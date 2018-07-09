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
#include "6502.h"
#include "NesSystem.h"
#include "debug.h"
#include "bitmap.h"

/*
bool Load6502Program( NesSystem& system, const string fileName, const uint16_t loadAddr, const uint16_t resetVector )
{
	ifstream programFile;
	programFile.open( fileName, ios::binary );
	std::string str( ( istreambuf_iterator< char >( programFile ) ), istreambuf_iterator< char >() );
	programFile.close();

	memset( system.memory, 0, Cpu6502::VirtualMemorySize );
	memcpy( system.memory + loadAddr, str.c_str(), str.size() );

	system.cpu.resetVector = resetVector;

	system.cpu.Reset();

	return true;
}


inline bool TestRegisters( const Cpu6502& cpu, const uint8_t A, const uint8_t X, const uint8_t Y, const uint8_t SP, const uint16_t PC, const uint8_t P )
{
	return ( cpu.A == A ) && ( cpu.X == X ) && ( cpu.Y == Y ) && ( cpu.SP == SP ) && ( cpu.PC == PC ) && ( cpu.P == P );
}


bool Easy6502Tests( NesSystem& system )
{
	Load6502Program( system, "Program1.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed1 = TestRegisters( cpu, 0x08, 0x00, 0x00, 0xFF, 0x0610, 0x30 );

	Load6502Program( system, "Program2.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed2 = TestRegisters( cpu, 0x84, 0xC1, 0x00, 0xFF, 0x0607, 0xB1 );

	Load6502Program( system, "Program3.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed3 = TestRegisters( cpu, 0x00, 0x03, 0x00, 0xFF, 0x060E, 0x33 );

	Load6502Program( system, "Program4.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed4 = TestRegisters( cpu, 0x01, 0x00, 0x00, 0xFF, 0x0609, 0xB0 );

	Load6502Program( system, "Program5.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed5 = TestRegisters( cpu, 0xCC, 0x00, 0x00, 0xFF, 0xCC02, 0xB0 );

	Load6502Program( system, "Program6.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed6 = TestRegisters( cpu, 0x0A, 0x01, 0x0A, 0xFF, 0x0612, 0x30 );

	Load6502Program( system, "Program7.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed7 = TestRegisters( cpu, 0x0A, 0x0A, 0x01, 0xFF, 0x0612, 0x30 );

	Load6502Program( system, "Program8.bin", 0x0600, 0x0600 );
	system.Run();

	const uint8_t memCheckValues[48] = {	0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
										0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
										0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

	bool memCheck = true;

	for ( uint16_t i = 0x00; i < 0x0F; ++i )
	{
		if ( cpu.memory[0x01f0 + i] != memCheckValues[i] )
		{
			memCheck = false;
			break;
		}
	}

	const bool passed8 = memCheck && TestRegisters( cpu, 0x00, 0x10, 0x20, 0xFF, 0x0619, 0x33 );

	Load6502Program( system, "Program9.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed9 = TestRegisters( cpu, 0x03, 0x00, 0x00, 0xFF, 0x060C, 0x30 );

	Load6502Program( system, "Program10.bin", 0x0600, 0x0600 );
	system.Run();
	const bool passed10 = TestRegisters( cpu, 0x00, 0x05, 0x00, 0xFD, 0x0613, 0x33 );

	return passed1 && passed2 && passed3 && passed4 && passed5 && passed6 && passed7 && passed8 && passed9 && passed10;
}


bool KlausTests( Cpu6502& cpu )
{
	Load6502Program( cpu, "6502_functional_test.bin", 0x0000, 0x0400 );

	cpu.Run();

	return ( cpu.A == 0x42 ) && ( cpu.X == 0x52 ) && ( cpu.Y == 0x4B ) && ( cpu.SP == 0xFF ) && ( cpu.PC == 0x09D0 ) && ( cpu.P == 0x30 );
}


float RunTests( Cpu6502& cpu )
{
	uint passedTests = Easy6502Tests( cpu ) ? 1 : 0;

	passedTests += KlausTests( cpu ) ? 1 : 0;

	return ( passedTests / 2.0f );
}
*/


RGBA Palette[0x40] =
{
	{ 0x75, 0x75, 0x75, 0xFF },	{ 0x27, 0x1B, 0x8F, 0xFF },	{ 0x00, 0x00, 0xAB, 0xFF },	{ 0x47, 0x00, 0x9F, 0xFF },
	{ 0x8F, 0x00, 0x77, 0xFF },	{ 0xAB, 0x00, 0x13, 0xFF },	{ 0xA7, 0x00, 0x00, 0xFF },	{ 0x7F, 0x0B, 0x00, 0xFF },
	{ 0x43, 0x2F, 0x00, 0xFF },	{ 0x00, 0x47, 0x00, 0xFF },	{ 0x00, 0x51, 0x00, 0xFF },	{ 0x00, 0x3F, 0x17, 0xFF },
	{ 0x1B, 0x3F, 0x5F, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },
	{ 0xBC, 0xBC, 0xBC, 0xFF },	{ 0x00, 0x73, 0xEF, 0xFF },	{ 0x23, 0x3B, 0xEF, 0xFF },	{ 0x83, 0x00, 0xF3, 0xFF },
	{ 0xBF, 0x00, 0xBF, 0xFF },	{ 0xE7, 0x00, 0x5B, 0xFF },	{ 0xDB, 0x2B, 0x00, 0xFF },	{ 0xCB, 0x4F, 0x0F, 0xFF },
	{ 0x8B, 0x73, 0x00, 0xFF },	{ 0x00, 0x97, 0x00, 0xFF },	{ 0x00, 0xAB, 0x00, 0xFF },	{ 0x00, 0x93, 0x3B, 0xFF },
	{ 0x00, 0x83, 0x8B, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },
	{ 0xFF, 0xFF, 0xFF, 0xFF },	{ 0x3F, 0xBF, 0xFF, 0xFF },	{ 0x5F, 0x97, 0xFF, 0xFF },	{ 0xA7, 0x8B, 0xFD, 0xFF },
	{ 0xF7, 0x7B, 0xFF, 0xFF },	{ 0xFF, 0x77, 0xB7, 0xFF },	{ 0xFF, 0x77, 0x63, 0xFF },	{ 0xFF, 0x9B, 0x3B, 0xFF },
	{ 0xF3, 0xBF, 0x3F, 0xFF },	{ 0x83, 0xD3, 0x13, 0xFF },	{ 0x4F, 0xDF, 0x4B, 0xFF },	{ 0x58, 0xF8, 0x98, 0xFF },
	{ 0x00, 0xEB, 0xDB, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },
	{ 0xFF, 0xFF, 0xFF, 0xFF },	{ 0xAB, 0xE7, 0xFF, 0xFF },	{ 0xC7, 0xD7, 0xFF, 0xFF },	{ 0xD7, 0xCB, 0xFF, 0xFF },
	{ 0xFF, 0xC7, 0xFF, 0xFF },	{ 0xFF, 0xC7, 0xDB, 0xFF },	{ 0xFF, 0xBF, 0xB3, 0xFF },	{ 0xFF, 0xDB, 0xAB, 0xFF },
	{ 0xFF, 0xE7, 0xA3, 0xFF },	{ 0xE3, 0xFF, 0xA3, 0xFF },	{ 0xAB, 0xF3, 0xBF, 0xFF },	{ 0xB3, 0xFF, 0xCF, 0xFF },
	{ 0x9F, 0xFF, 0xF3, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },	{ 0x00, 0x00, 0x00, 0xFF },
};


const uint8_t BitPlaneuint8_ts = 8;


inline const uint16_t CalcTileOffset( const uint16_t tileIx )
{
	const uint8_t tileuint8_ts = 2 * BitPlaneuint8_ts;
	const uint16_t tileOffset =  tileIx * tileuint8_ts;

	return tileOffset;
}


const uint8_t* FetchBitPlane0( const uint8_t*const tileMem )
{
	return &tileMem[0];
}


const uint8_t* FetchBitPlane1( const uint8_t*const tileMem )
{
	return &tileMem[BitPlaneuint8_ts];
}


uint8_t GetTilePixel( const uint8_t*const bitPlane0, const uint8_t*const bitPlane1, const uint8_t row, const uint8_t pixel )
{
	const uint8_t shift = ( 7 - pixel );
	const uint8_t bit0 = ( bitPlane0[row] >> shift ) & 0x01;
	const uint8_t bit1 = ( ( bitPlane1[row] >> shift ) << 1 ) & 0x02;
	const uint8_t colorIndex = ( bit0 | bit1 );

	return colorIndex;
}


void TileRowToPixels( const uint8_t*const bitPlane0, const uint8_t*const bitPlane1, const uint8_t row, const uint8_t*const palette, uint32_t pixels[8] )
{
	for( int pixelNum = 0; pixelNum < 8; ++pixelNum )
	{
		const uint8_t colorIndex = GetTilePixel( bitPlane0, bitPlane1, row, pixelNum );

		pixels[ pixelNum ] = palette[ colorIndex ];
	}
}


void DrawTile( NesSystem& system, NesCart& cart, Bitmap& image, const uint16_t nametableX, const uint16_t nametableY, const uint32_t tableIx, const int tileIx, RGBA colorPalette[], const uint32_t cornerX, const uint32_t cornerY )
{
	const uint8_t tileBytes = 16;

	const uint16_t baseAddr = cart.header.prgRomBanks * 0x4000 + tableIx * 0x1000;
	const uint16_t attribAddr = 0x23C0;
	const uint16_t paletteAddr = 0x3F00;

	const uint8_t attribWidth = 8;
	const uint8_t attribXoffset = (uint8_t)( nametableX / 4 );
	const uint8_t attribYoffset = (uint8_t)( nametableY / 4 );
	const uint8_t attribTable = system.ppu.vram[attribAddr + attribXoffset + attribYoffset * attribWidth];

	const uint8_t attribSectionRow = (uint8_t)floor( ( nametableX % 4 ) / 2.0f ); // 2 is to convert from nametable to attrib tile
	const uint8_t attribSectionCol = (uint8_t)floor( ( nametableY % 4 ) / 2.0f );
	const uint8_t attribShift = 2 * ( attribSectionRow + 2 * attribSectionCol );
	
	const uint8_t paleteIx = 4 * ( ( attribTable >> attribShift ) & 0x03 );

	const uint16_t chrRomBase = baseAddr + tileIx * tileBytes;

	for ( int y = 0; y < 8; ++y )
	{
		for ( int x = 0; x < 8; ++x )
		{
			const uint8_t chrRom0 = cart.rom[chrRomBase + y];
			const uint8_t chrRom1 = cart.rom[chrRomBase + ( tileBytes / 2 ) + y];

			const uint16_t xBitMask = ( 0x80 >> x );

			const uint8_t lowerBits = ( ( chrRom0 & xBitMask ) >> ( 7 - x ) ) | ( ( ( chrRom1 & xBitMask ) >> ( 7 - x ) ) << 1 );

			const uint32_t imageX = cornerX + x;
			const uint32_t imageY = cornerY + y;

			const uint8_t colorIx = system.ppu.vram[paletteAddr + paleteIx + lowerBits];

			Pixel pixelColor;

			Bitmap::CopyToPixel( Palette[colorIx], pixelColor );

			image.setPixel( imageX, 239 - imageY, pixelColor.raw );
		}
	}
}


void DrawSprite( NesSystem& system, NesCart& cart, Bitmap& image, const uint32_t tableIx, RGBA colorPalette[] )
{
	const uint8_t tileBytes = 16;

	const uint16_t totalSprites = 64;
	const uint16_t spritePaletteAddr = 0x3F10;

	for( uint16_t spriteNum = 0; spriteNum < totalSprites; ++spriteNum )
	{
		const uint8_t spriteY = system.ppu.primaryOAM[spriteNum * 4];
		const uint8_t tileId = system.ppu.primaryOAM[spriteNum * 4 + 1]; // TODO: implement 16x8
		const uint8_t attrib = system.ppu.primaryOAM[spriteNum * 4 + 2]; // TODO: finish attrib features
		const uint8_t spriteX = system.ppu.primaryOAM[spriteNum * 4 + 3];

		const uint8_t attribPal = 4 * ( attrib & 0x3 );
		const uint8_t priority = ( attrib & 0x20 ) >> 5;
		const uint8_t flippedHor = ( attrib & 0x40 ) >> 6;
		const uint8_t flippedVert = ( attrib & 0x80 ) >> 7;

		const uint16_t baseAddr = cart.header.prgRomBanks * 0x4000 + tableIx * 0x1000;
		const uint16_t chrRomBase = baseAddr + tileId * tileBytes;

		if ( spriteY < 8 || spriteY > 232 )
			continue;

		assert( priority == 0 );

		for ( int y = 0; y < 8; ++y )
		{
			for ( int x = 0; x < 8; ++x )
			{
				const uint8_t tileRow = flippedVert ? y : ( 7 - y );
				const uint8_t tileCol = flippedHor ? ( 7 - x ) : x;

				const uint8_t chrRom0 = cart.rom[chrRomBase + y];
				const uint8_t chrRom1 = cart.rom[chrRomBase + ( tileBytes / 2 ) + y];

				const uint16_t xBitMask = ( 0x80 >> x );

				const uint8_t lowerBits = ( ( chrRom0 & xBitMask ) >> ( 7 - x ) ) | ( ( ( chrRom1 & xBitMask ) >> ( 7 - x ) ) << 1 );

				if( lowerBits == 0x00 )
					continue;

				const uint8_t colorIx = system.ppu.vram[spritePaletteAddr + attribPal + lowerBits];

				Pixel pixelColor;

				Bitmap::CopyToPixel( Palette[colorIx], pixelColor );

				image.setPixel( spriteX + tileCol, 239 - spriteY + tileRow - 8, pixelColor.raw );
			}
		}
	}
}


int main()
{
	NesSystem system;

	Cpu6502 cpu;

	cpu.Reset();

	std::chrono::seconds sec( 1 );

	frameRate_t frame( 1 );
	frameRate_t frameDelta( 1 );
	chrono::milliseconds frameTime = chrono::duration_cast<chrono::milliseconds>( frame );

	masterCycles_t tick = chrono::duration_cast<masterCycles_t>( sec );

	masterCycles_t cyclesPerFrame = chrono::duration_cast<masterCycles_t>( frame );

	cpuCycle_t cyclesPerMaster= chrono::duration_cast<cpuCycle_t>( tick );
	ppuCycle_t ppucyclesPerMaster = chrono::duration_cast<cpuCycle_t>( tick );

	cout << tick.count() << endl;
	cout << cyclesPerMaster.count() << endl;
	cout << ppucyclesPerMaster.count() << endl;
	cout << cyclesPerFrame.count() << endl;

#if NES_MODE == 1
	NesCart cart;
	/*LoadNesFile( "nestest.nes", cart );
	system.LoadProgram( cart, 0xC000 );
	system.cpu.forceStopAddr = 0xC6BD;
	*/
	LoadNesFile( "Donkey Kong.nes", cart );
	system.LoadProgram( cart );

#if NES_MODE == 1
	RGBA colorIndex[] = { { 0x00_b, 0x00_b,0x00_b, 0xFF_b }, { 0x33_b, 0x33b_b, 0x33_b, 0xFF_b }, { 0x88_b, 0x88_b, 0x88_b, 0xFF_b }, { 0xBB_b, 0xBB_b, 0xBB_b, 0xFF_b } };
	/*
	Bitmap image( 128, 256, 0x00 );

	int tileIx = 0;

	const uint8_t tileuint8_ts = 16;

	for ( int tileIx = 0; tileIx < 512; ++tileIx )
	{
		const uint cornerX = static_cast< uint >( ( tileIx % 16 ) * 8 );
		const uint cornerY = static_cast< uint >( 8 * floor( tileIx / 16.0f ) );

		DrawTile( cart, image, 0x01, 0x62, colorIndex, cornerX, cornerY );
	}

	image.write( "chrRom.bmp" );
	*/
	Bitmap paletteDebug( 0x40, 1, 0x00 );
	for ( int colorIx = 0; colorIx < 0x40; ++colorIx )
	{
		Pixel pixel;

		Bitmap::CopyToPixel( Palette[colorIx], pixel );

		paletteDebug.setPixel( colorIx, 0, pixel.raw );
	}
	paletteDebug.write( "paletteDebug.bmp" );
#endif // #if NES_MODE == 1

	bool isRunning = true;


	while( isRunning )
	{
		if( ( frame.count() % 60 ) == 0 )
		{
			// print nametable

			Bitmap nametable( 256, 240, 0x00 );

			for ( int tileY = 0; tileY < 30; ++tileY )
			{
				for ( int tileX = 0; tileX < 32; ++tileX )
				{
					const uint32_t cornerX = static_cast< uint32_t >( tileX * 8 );
					const uint32_t cornerY = static_cast< uint32_t >( 8 * tileY );

					DrawTile( system, cart, nametable, tileX, tileY, 0x01, system.ppu.vram[ /*0x2083*/0x2000 + tileX + tileY * 32 ], Palette, cornerX, cornerY );
				}
			}

			DrawSprite( system, cart, nametable, 0x00, Palette );

			stringstream nametableName;
			stringstream palettesName;

			nametableName << "nametable" << frame.count() << ".bmp";
			palettesName << "palettes" << frame.count() << ".bmp";

			nametable.write( nametableName.str() );
			/*
			Bitmap framePalette( 0x10, 2, 0x00 );
			for ( int colorIx = 0; colorIx <= 0x0F; ++colorIx )
			{
				Pixel pixel;

				const uint16_t paletteAddr = 0x3F00;
				const uint16_t spritePaletteAddr = 0x3F10;

				Bitmap::CopyToPixel( Palette[ system.ppu.vram[ paletteAddr + colorIx ] ], pixel );
				framePalette.setPixel( colorIx, 0, pixel.raw );

				Bitmap::CopyToPixel( Palette[system.ppu.vram[spritePaletteAddr + colorIx]], pixel );
				framePalette.setPixel( colorIx, 1, pixel.raw );
			}
			framePalette.write( palettesName.str() );
			
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
		}

		masterCycles_t nextCycle = chrono::duration_cast<masterCycles_t>( frameRate_t( 1 ) );

		isRunning = system.Run( nextCycle );

		++frame;

//		if( (frame % 60).count() == 0 )
		{
//			cout << "CPU Cycles per Second:" << system.cpu.iterationCycle << endl;
//			system.cpu.iterationCycle = 0;
		}
	}
#else
	const float passedPercent = RunTests( cpu );

	cout << "Passed Tests %" << ( passedPercent * 100.0f ) << endl;
#endif

#if DEBUG_MEM
	/*
	uint colorIndex[] = { 0x000000FF, 0xFFFFFFFF,	0x880000FF, 0xAAFFEEFF,
		0xCC44CCFF, 0x00CC55FF, 0x0000AAFF, 0xEEEE77FF,
		0xDD8855FF, 0x664400FF, 0xFF7777FF, 0x333333FF,
		0x777777FF, 0xAAFF66FF, 0x0088FFFF, 0xBBBBBBFF };
	/*
	for ( std::map< uint16_t, uint8_t>::iterator it = memoryDebug.begin(); it != memoryDebug.end(); ++it )
	{
		const uint x = ( it->first - 0x200 ) % 32;
		const uint y = ( it->first - 0x200 ) / 32;

		image.setPixel( x, 31 - y, colorIndex[it->second % 16] );
	}
	*/	
#endif // #if DEBUG_MEM

	char end;
	cin >> end;

	return 0;
}

