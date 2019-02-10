#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <assert.h>
#include <map>
#include <sstream>
#include "common.h"
#include "mos6502.h"
#include "NesSystem.h"
#include "debug.h"
#include "bitmap.h"


NesSystem nesSystem;
frameRate_t frame( 1 );
NesCart cart;

typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_t;
timePoint_t previousTime;
bool lockFps = false;


int InitSystem()
{
	//LoadNesFile( "Games/nestest.nes", cart ); // Draws using pattern table 0
	//nesSystem.LoadProgram( cart, 0xC000 );
	//nesSystem.cpu.forceStopAddr = 0xC6BD;

	//LoadNesFile( "Games/Contra.nes", cart );
	//LoadNesFile( "Games/Super Mario Bros.nes", cart ); // starts, stalls on 0 sprite flag
	//LoadNesFile( "Games/Mario Bros.nes", cart ); // works, no collision (0 sprite?)
	//LoadNesFile( "Games/Balloon Fight.nes", cart ); // works, scrolls
	//LoadNesFile( "Games/Tennis.nes", cart ); // works
	//LoadNesFile( "Games/Ice Climber.nes", cart ); // starts, scrolls
	//LoadNesFile( "Games/Excitebike.nes", cart ); // works, no scroll, no sprites (0 sprite?)
	LoadNesFile( "Games/Donkey Kong.nes", cart ); // works, completed game
	nesSystem.LoadProgram( cart );
	nesSystem.cart = &cart;

	return 0;
}


void ShutdownSystem()
{
#if DEBUG_ADDR == 1
	nesSystem.cpu.logFile.close();
#endif // #if DEBUG_ADDR == 1
}


void CopyFrameBuffer( uint32_t destBuffer[], const size_t destSize )
{
	memcpy( destBuffer, nesSystem.frameBuffer, destSize );
}


void CopyNametable( uint32_t destBuffer[], const size_t destSize )
{
	memcpy( destBuffer, nesSystem.nameTableSheet, destSize );
}


int RunFrame()
{
	chrono::milliseconds frameTime = chrono::duration_cast<chrono::milliseconds>( frameRate_t( 1 ) );

	auto targetTime = previousTime + frameTime;
	auto currentTime = chrono::steady_clock::now();

	auto elapsed = targetTime - currentTime;
	auto dur = chrono::duration <double, nano>( elapsed ).count();

	if( lockFps && ( elapsed.count() > 0 ) )
	{
		std::this_thread::sleep_for( std::chrono::nanoseconds( elapsed ) );
	}

	previousTime = chrono::steady_clock::now();

	masterCycles_t cyclesPerFrame = chrono::duration_cast<masterCycles_t>( frameTime );

	masterCycles_t nextCycle = chrono::duration_cast<masterCycles_t>( frameRate_t( 1 ) );

	bool isRunning = isRunning = nesSystem.Run( nextCycle );

	++frame;

	if( nesSystem.headless )
	{
		return 0;
	}

	WtRect imageRect = { 0, 0, NesSystem::ScreenWidth, NesSystem::ScreenHeight };

	for ( uint32_t tileY = 0; tileY < PPU::NameTableHeightTiles; ++tileY )
	{
		for ( uint32_t tileX = 0; tileX < PPU::NameTableWidthTiles; ++tileX )
		{
			nesSystem.ppu.DrawTile( nesSystem.frameBuffer, imageRect, WtPoint{ tileX, tileY }, nesSystem.ppu.GetNameTableId(), nesSystem.ppu.GetBgPatternTableId(), true );

			imageRect.x += static_cast< uint32_t >( PPU::NameTableTilePixels );
		}

		imageRect.x = 0;
		imageRect.y += static_cast< uint32_t >( PPU::NameTableTilePixels );
	}

	nesSystem.ppu.DrawSprite( nesSystem.ppu.GetSpritePatternTableId() );

	bool debugNT = nesSystem.debugNT && ( ( (int)frame.count() % 60 ) == 0 );

	if( debugNT )
	{
		WtRect ntRects[4] = {	{ 0,						0,							2 * NesSystem::ScreenWidth,	2 * NesSystem::ScreenHeight },
								{ NesSystem::ScreenWidth,	0,							2 * NesSystem::ScreenWidth,	2 * NesSystem::ScreenHeight },
								{ 0,						NesSystem::ScreenHeight,	2 * NesSystem::ScreenWidth, 2 * NesSystem::ScreenHeight },
								{ NesSystem::ScreenWidth,	NesSystem::ScreenHeight,	2 * NesSystem::ScreenWidth, 2 * NesSystem::ScreenHeight }, };

		WtPoint ntCorners[4] = {	{0,							0 },
									{ NesSystem::ScreenWidth,	0 },
									{ 0,						NesSystem::ScreenHeight },
									{ NesSystem::ScreenWidth,	NesSystem::ScreenHeight },};

		for ( uint32_t ntId = 0; ntId < 4; ++ntId )
		{
			for ( uint32_t tileY = 0; tileY < PPU::NameTableHeightTiles; ++tileY )
			{
				for ( uint32_t tileX = 0; tileX < PPU::NameTableWidthTiles; ++tileX )
				{
					nesSystem.ppu.DrawTile( nesSystem.nameTableSheet, ntRects[ntId], WtPoint{ tileX, tileY }, ntId, nesSystem.ppu.GetBgPatternTableId(), false );

					ntRects[ntId].x += static_cast< uint32_t >( PPU::NameTableTilePixels );
				}

				ntRects[ntId].x = ntCorners[ntId].x;
				ntRects[ntId].y += static_cast< uint32_t >( PPU::NameTableTilePixels );
			}
		}
	}

	/*
	stringstream palettesName;
	palettesName << "Palettes/" << "plt_" << frame.count() << ".bmp";

	Bitmap framePalette( 0x10, 2, 0x00 );
	for ( int colorIx = 0; colorIx <= 0x0F; ++colorIx )
	{
		Pixel pixel;

		const uint16_t paletteAddr = 0x3F00;
		const uint16_t spritePaletteAddr = 0x3F10;

		Bitmap::CopyToPixel( Palette[nesSystem.ppu.vram[ paletteAddr + colorIx ] ], pixel, BITMAP_ABGR );
		framePalette.SetPixel( colorIx, 0, pixel.raw );

		Bitmap::CopyToPixel( Palette[nesSystem.ppu.vram[spritePaletteAddr + colorIx]], pixel, BITMAP_ABGR );
		framePalette.SetPixel( colorIx, 1, pixel.raw );
	}
	framePalette.Write( palettesName.str() );
			
	cout << "Frame: " << frame.count() << endl;

	const uint16_t attribAddr = 0x23C0;

	for ( int attribY = 0; attribY < 8; ++attribY )
	{
		for ( int attribX = 0; attribX < 8; ++attribX )
		{
			const uint16_t attribTable = system.ppu.vram[attribAddr + attribY*8 + attribX];
			const uint16_t tL = ( attribTable >> 6 ) & 0x03;
			const uint16_t tR = ( attribTable >> 4 ) & 0x03;
			const uint16_t bL = ( attribTable >> 2 ) & 0x03;
			const uint16_t bR = ( attribTable >> 0 ) & 0x03;

			cout << "[" << setfill( '0' ) << hex << setw( 2 ) << tL << " " << setw( 2 ) << tR << " " << setw( 2 ) << bL << " " << setw( 2 ) << bR << "]";
		}

		cout << endl;
	}

	cout << endl;
	*/

	return 0;
}



int main()
{
	InitSystem();
	nesSystem.headless = true;

	RunFrame();

	ShutdownSystem();
}