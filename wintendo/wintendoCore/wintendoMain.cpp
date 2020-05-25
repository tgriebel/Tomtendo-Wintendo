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
#include <wchar.h>
#include "common.h"
#include "mos6502.h"
#include "NesSystem.h"
#include "debug.h"
#include "bitmap.h"


wtSystem nesSystem;
wtCart cart;
frameRate_t frame( 1 );

typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_t;
timePoint_t previousTime;
bool lockFps = true;


int InitSystem( const wstring& filePath )
{
	LoadNesFile( filePath, cart);

	nesSystem.LoadProgram( cart );
	nesSystem.cart = &cart;
	nesSystem.fileName = filePath;

	return 0;
}


void ShutdownSystem()
{
#if DEBUG_ADDR == 1
	nesSystem.cpu.logFile.close();
#endif // #if DEBUG_ADDR == 1
}


void CopyImageBuffer( wtRawImage& destImageBuffer, wtImageTag tag )
{
	switch ( tag )
	{
		case wtImageTag::FRAME_BUFFER:
			destImageBuffer = nesSystem.frameBuffer;
		break;

		case wtImageTag::NAMETABLE:
			destImageBuffer = nesSystem.nameTableSheet;
		break;

		case wtImageTag::PALETTE:
			destImageBuffer = nesSystem.paletteDebug;
		break;

		case wtImageTag::PATTERN_TABLE_0:
			destImageBuffer = nesSystem.patternTable0Debug;
		break;

		case wtImageTag::PATTERN_TABLE_1:
			destImageBuffer = nesSystem.patternTable1Debug;
		break;

		default:
			assert(0); // TODO: Error message
		break;
	}
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

	masterCycles_t nextCycle = chrono::duration_cast<masterCycles_t>( ++frame );

	bool isRunning = isRunning = nesSystem.Run( nextCycle );

	if( nesSystem.headless )
	{
		return isRunning;
	}

	if( !isRunning )
	{
		return false;
	}

	wtRect imageRect = { 0, 0, wtSystem::ScreenWidth, wtSystem::ScreenHeight };

	static uint8_t line = 0;

	for ( uint32_t scanY = 0; scanY < wtSystem::ScreenHeight; ++scanY )
	{
		if( ( scanY % 2 ) == line ) // lazy scanline effect
		{
			//nesSystem.ppu.DrawBlankScanline( nesSystem.frameBuffer, imageRect, scanY );
			continue;
		}

		//nesSystem.ppu.DrawScanline( nesSystem.frameBuffer, imageRect, NesSystem::ScreenWidth, scanY );
	}

	line ^= 1;

	//nesSystem.ppu.DrawSprites( nesSystem.ppu.GetSpritePatternTableId() );

	bool debugNT = nesSystem.debugNT && ( ( (int)frame.count() % 60 ) == 0 );

	if( debugNT )
	{
		wtRect ntRects[4] = {	{ 0,						0,							2 * wtSystem::ScreenWidth,	2 * wtSystem::ScreenHeight },
								{ wtSystem::ScreenWidth,	0,							2 * wtSystem::ScreenWidth,	2 * wtSystem::ScreenHeight },
								{ 0,						wtSystem::ScreenHeight,	2 * wtSystem::ScreenWidth, 2 * wtSystem::ScreenHeight },
								{ wtSystem::ScreenWidth,	wtSystem::ScreenHeight,	2 * wtSystem::ScreenWidth, 2 * wtSystem::ScreenHeight }, };

		wtPoint ntCorners[4] = {	{0,							0 },
									{ wtSystem::ScreenWidth,	0 },
									{ 0,						wtSystem::ScreenHeight },
									{ wtSystem::ScreenWidth,	wtSystem::ScreenHeight },};

		for ( uint32_t ntId = 0; ntId < 4; ++ntId )
		{
			for ( int32_t tileY = 0; tileY < (int)PPU::NameTableHeightTiles; ++tileY )
			{
				for ( int32_t tileX = 0; tileX < (int)PPU::NameTableWidthTiles; ++tileX )
				{
					nesSystem.ppu.DrawTile( nesSystem.nameTableSheet, ntRects[ntId], wtPoint{ tileX, tileY }, ntId, nesSystem.ppu.GetBgPatternTableId() );

					ntRects[ntId].x += static_cast< uint32_t >( PPU::TilePixels );
				}

				ntRects[ntId].x = ntCorners[ntId].x;
				ntRects[ntId].y += static_cast< uint32_t >( PPU::TilePixels );
			}
		}
	}

	for ( int32_t tileY = 0; tileY < 16; ++tileY )
	{
		for ( int32_t tileX = 0; tileX < 16; ++tileX )
		{
			nesSystem.ppu.DrawTile( nesSystem.patternTable0Debug, wtRect{ (int32_t)PPU::TilePixels * tileX, (int32_t)PPU::TilePixels * tileY, PPU::PatternTableWidth, PPU::PatternTableHeight }, (int32_t)( tileX + 16 * tileY ), 0 );
			nesSystem.ppu.DrawTile( nesSystem.patternTable1Debug, wtRect{ (int32_t)PPU::TilePixels * tileX, (int32_t)PPU::TilePixels * tileY, PPU::PatternTableWidth, PPU::PatternTableHeight }, (int32_t)( tileX + 16 * tileY ), 1 );
		}
	}

	nesSystem.ppu.DrawDebugPalette( nesSystem.paletteDebug );

	static bool debugAttribs = false;
		
	if( debugAttribs )
	{
		ofstream attribFile;

		stringstream attribName;
		attribName << "Palettes/" << "attrib_" << frame.count() << ".txt";

		attribFile.open( attribName.str() );

		attribFile << "Frame: " << frame.count() << endl;

		const uint16_t attribAddr = 0x23C0;

		for ( int attribY = 0; attribY < 8; ++attribY )
		{
			for ( int attribX = 0; attribX < 8; ++attribX )
			{
				const uint16_t attribTable = nesSystem.ppu.vram[attribAddr + attribY*8 + attribX];
				const uint16_t tL = ( attribTable >> 6 ) & 0x03;
				const uint16_t tR = ( attribTable >> 4 ) & 0x03;
				const uint16_t bL = ( attribTable >> 2 ) & 0x03;
				const uint16_t bR = ( attribTable >> 0 ) & 0x03;

				attribFile << "[" << setfill( '0' ) << hex << setw( 2 ) << tL << " " << setw( 2 ) << tR << " " << setw( 2 ) << bL << " " << setw( 2 ) << bR << "]";
			}

			attribFile << endl;
		}

		attribFile << endl;

		attribFile.close();
	}

	return isRunning;
}



int main()
{
	InitSystem( L"Games/Contra.nes" );
	nesSystem.headless = true;

	RunFrame();

	ShutdownSystem();
}