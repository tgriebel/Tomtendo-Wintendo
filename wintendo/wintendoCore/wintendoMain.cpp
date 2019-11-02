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


NesSystem nesSystem;
frameRate_t frame( 1 );
NesCart cart;

typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_t;
timePoint_t previousTime;
bool lockFps = false;


int InitSystem( const char* filePath )
{
	//LoadNesFile( "Games/nestest.nes", cart ); // Draws using pattern table 0
	//nesSystem.LoadProgram( cart, 0xC000 );
	//nesSystem.cpu.forceStopAddr = 0xC6BD;

	//LoadNesFile("Tests/01-basics.nes", cart);
	//LoadNesFile("Tests/02-implied.nes", cart);
	//LoadNesFile("Tests/03-immediate.nes", cart); // FAILED?
	//LoadNesFile("Tests/04-zero_page.nes", cart); // FAILED
	//LoadNesFile("Tests/05-zp_xy.nes", cart); // FAILED
	//LoadNesFile("Tests/06-absolute.nes", cart); // FAILED
	//LoadNesFile("Tests/07-abs_xy.nes", cart); // FAILED
	//LoadNesFile("Tests/08-ind_x.nes", cart); // FAILED
	//LoadNesFile("Tests/09-ind_y.nes", cart); // FAILED
	//LoadNesFile("Tests/10-branches.nes", cart);
	//LoadNesFile("Tests/11-stack.nes", cart);
	//LoadNesFile("Tests/12-jmp_jsr.nes", cart);
	//LoadNesFile("Tests/13-rts.nes", cart);
	//LoadNesFile("Tests/14-rti.nes", cart);
	//LoadNesFile("Tests/15-brk.nes", cart); // FAILED
	//LoadNesFile("Tests/16-special.nes", cart); // FAILED
	//LoadNesFile("Tests/all_instrs.nes", cart);
	//LoadNesFile("Tests/nestest.nes", cart);

	//LoadNesFile("PpuTests/color_test.nes", cart);	
	//LoadNesFile("PpuTests/test_ppu_read_buffer.nes", cart); // FAILED
	//LoadNesFile("PpuTests/vram_access.nes", cart); // FAILED
	
	//LoadNesFile( "Games/Metroid.nes", cart ); // 
	//LoadNesFile("Games/Duck Tales.nes", cart); // Needs mapper
	//LoadNesFile("Games/Megaman.nes", cart); // Needs mapper
	//LoadNesFile("Games/Castlevania.nes", cart); // Needs mapper
	//LoadNesFile( "Games/Contra.nes", cart ); // Needs mapper
	//LoadNesFile( "Games/Super Mario Bros.nes", cart ); // works
	//LoadNesFile( "Games/Mario Bros.nes", cart ); // works
	//LoadNesFile( "Games/Balloon Fight.nes", cart ); // works
	//LoadNesFile( "Games/Tennis.nes", cart ); // works
	//LoadNesFile( "Games/Ice Climber.nes", cart ); // works, minor graphical issues with ice blocks, wasn't a problem before addr reg rework
	//LoadNesFile( "Games/Excitebike.nes", cart ); // works
	//LoadNesFile( "Games/Donkey Kong.nes", cart ); // works
	LoadNesFile( filePath, cart);

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


void CopyPalette( uint32_t destBuffer[], const size_t destSize )
{
	memcpy( destBuffer, nesSystem.paletteDebug, destSize );
}


void CopyPatternTable0( uint32_t destBuffer[], const size_t destSize )
{
	memcpy( destBuffer, nesSystem.patternTable0Debug, destSize );
}


void CopyPatternTable1( uint32_t destBuffer[], const size_t destSize )
{
	memcpy( destBuffer, nesSystem.patternTable1Debug, destSize );
}


void SetGameName( const char* name )
{
	string fileName;
	nesSystem.fileName.clear();
	nesSystem.fileName.insert( 0, name );
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
		return 0;
	}

	WtRect imageRect = { 0, 0, NesSystem::ScreenWidth, NesSystem::ScreenHeight };

	static uint8_t line = 0;

	for ( uint32_t scanY = 0; scanY < NesSystem::ScreenHeight; ++scanY )
	{
		if( ( scanY % 2 ) == line ) // lazy scanline effect
		{
		//	nesSystem.ppu.DrawBlankScanline( nesSystem.frameBuffer, imageRect, scanY );
			continue;
		}

	//	nesSystem.ppu.DrawScanline( nesSystem.frameBuffer, imageRect, NesSystem::ScreenWidth, scanY );
	}

	line ^= 1;

	//nesSystem.ppu.DrawSprites( nesSystem.ppu.GetSpritePatternTableId() );

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
			for ( int32_t tileY = 0; tileY < (int)PPU::NameTableHeightTiles; ++tileY )
			{
				for ( int32_t tileX = 0; tileX < (int)PPU::NameTableWidthTiles; ++tileX )
				{
					nesSystem.ppu.DrawTile( nesSystem.nameTableSheet, ntRects[ntId], WtPoint{ tileX, tileY }, ntId, nesSystem.ppu.GetBgPatternTableId() );

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
			nesSystem.ppu.DrawTile( nesSystem.patternTable0Debug, WtRect{ 8 * tileX, 8 * tileY, 128, 128 }, (int32_t)( tileX + 16 * tileY ), 0 );
			nesSystem.ppu.DrawTile( nesSystem.patternTable1Debug, WtRect{ 8 * tileX, 8 * tileY, 128, 128 }, (int32_t)( tileX + 16 * tileY ), 1 );
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

	return 0;
}



int main()
{
	InitSystem( "Games/Contra.nes" );
	nesSystem.headless = true;

	RunFrame();

	ShutdownSystem();
}