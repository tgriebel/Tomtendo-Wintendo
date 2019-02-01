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


const RGBA Palette[64] =
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


const uint8_t BitPlaneBytes = 8;

NesSystem nesSystem;
frameRate_t frame( 1 );
NesCart cart;

typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_t;
timePoint_t previousTime;
bool lockFps = true;


inline void FrameBufferWritePixel( NesSystem& system, const uint32_t x, const uint32_t y, const Pixel pixel )
{
	system.frameBuffer[ x + y * NesSystem::ScreenWidth ] = pixel.raw;
}


inline const uint16_t CalcTileOffset( const uint16_t tileIx )
{
	const uint8_t tileBytes = 2 * BitPlaneBytes;
	const uint16_t tileOffset =  tileIx * tileBytes;

	return tileBytes;
}


const uint8_t* FetchBitPlane0( const uint8_t*const tileMem )
{
	return &tileMem[0];
}


const uint8_t* FetchBitPlane1( const uint8_t*const tileMem )
{
	return &tileMem[BitPlaneBytes];
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

// TODO: system should own palette
void DrawTile( NesSystem& system, uint32_t imageBuffer[], const WtRect& imageRect, const WtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId )
{
	const uint8_t tileBytes = 16;

	const uint32_t ntBaseAddr = ( PPU::NameTable0BaseAddr + ntId * PPU::NameTableAttribMemorySize );
	const uint32_t tileIx = system.ppu.vram[ntBaseAddr + nametableTile.x + nametableTile.y * PPU::NameTableWidthTiles];

	const uint16_t baseAddr = system.cart->header.prgRomBanks * NesSystem::BankSize + ptrnTableId * NesSystem::ChrRomSize;
	const uint16_t attribAddr = ntBaseAddr + PPU::NametableMemorySize;
	const uint16_t paletteAddr = PPU::PaletteBaseAddr;

	const uint8_t attribWidth = 8;
	const uint8_t attribXoffset = (uint8_t)( nametableTile.x / 4 );
	const uint8_t attribYoffset = (uint8_t)( nametableTile.y / 4 );
	const uint8_t attribTable = system.ppu.vram[attribAddr + attribXoffset + attribYoffset * attribWidth];

	const uint8_t attribSectionRow = (uint8_t)floor( ( nametableTile.x % 4 ) / 2.0f ); // 2 is to convert from nametable to attrib tile
	const uint8_t attribSectionCol = (uint8_t)floor( ( nametableTile.y % 4 ) / 2.0f );
	const uint8_t attribShift = 2 * ( attribSectionRow + 2 * attribSectionCol );
	
	const uint8_t paleteIx = 4 * ( ( attribTable >> attribShift ) & 0x03 );

	const uint16_t chrRomBase = baseAddr + tileIx * tileBytes;

	for ( int y = 0; y < 8; ++y )
	{
		for ( int x = 0; x < 8; ++x )
		{
			const uint8_t chrRom0 = system.cart->rom[chrRomBase + y];
			const uint8_t chrRom1 = system.cart->rom[chrRomBase + ( tileBytes / 2 ) + y];

			const uint16_t xBitMask = ( 0x80 >> x );

			const uint8_t lowerBits = ( ( chrRom0 & xBitMask ) >> ( 7 - x ) ) | ( ( ( chrRom1 & xBitMask ) >> ( 7 - x ) ) << 1 );

			const uint32_t imageX = imageRect.x + x;
			const uint32_t imageY = imageRect.y + y;

			const uint8_t colorIx = system.ppu.vram[paletteAddr + paleteIx + lowerBits];

			Pixel pixelColor;

			Bitmap::CopyToPixel( system.palette[colorIx], pixelColor, BITMAP_BGRA );

			imageBuffer[ imageX + imageY * imageRect.width ] = pixelColor.raw;
		}
	}
}


void DrawSprite( NesSystem& system, const uint32_t tableId )
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

		const uint16_t baseAddr = system.cart->header.prgRomBanks * 0x4000 + tableId * 0x1000;
		const uint16_t chrRomBase = baseAddr + tileId * tileBytes;

		if ( spriteY < 8 || spriteY > 232 )
			continue;

		assert( priority == 0 );

		for ( int y = 0; y < 8; ++y )
		{
			for ( int x = 0; x < 8; ++x )
			{
				const uint8_t tileRow = flippedVert ? ( 7 - y ) : y;
				const uint8_t tileCol = flippedHor ? ( 7 - x ) : x;

				const uint8_t chrRom0 = system.cart->rom[chrRomBase + y];
				const uint8_t chrRom1 = system.cart->rom[chrRomBase + ( tileBytes / 2 ) + y];

				const uint16_t xBitMask = ( 0x80 >> x );

				const uint8_t lowerBits = ( ( chrRom0 & xBitMask ) >> ( 7 - x ) ) | ( ( ( chrRom1 & xBitMask ) >> ( 7 - x ) ) << 1 );

				if( lowerBits == 0x00 )
					continue;

				//if( spriteZeroHit )
				{
				//	system.ppu.regStatus.current.sem.spriteHit = true;
				}

				const uint8_t colorIx = system.ppu.vram[spritePaletteAddr + attribPal + lowerBits];

				Pixel pixelColor;

				Bitmap::CopyToPixel( system.palette[colorIx], pixelColor, BITMAP_BGRA );
				FrameBufferWritePixel( system, spriteX + tileCol, spriteY + tileRow, pixelColor );
			}
		}
	}
}




int InitSystem()
{
	//LoadNesFile( "nestest.nes", cart );
	//system.LoadProgram( cart, 0xC000 );
	//system.cpu.forceStopAddr = 0xC6BD;

	//LoadNesFile( "Games/Super Mario Bros.nes", cart );
	//LoadNesFile( "Games/Mario Bros.nes", cart ); // works, no collision (0 sprite?)
	//LoadNesFile( "Games/Balloon Fight.nes", cart ); // grey screen
	//LoadNesFile( "Games/Tennis.nes", cart ); // works
	//LoadNesFile( "Games/Ice Climber.nes", cart ); // grey screen
	//LoadNesFile( "Games/nestest.nes", cart );
	//LoadNesFile( "Games/Excitebike.nes", cart ); // works, no scroll
	LoadNesFile( "Games/Donkey Kong.nes", cart ); // works, completed game
	nesSystem.LoadProgram( cart );
	nesSystem.palette = &Palette[0];
	nesSystem.cart = &cart;

	return 0;
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

	if( lockFps && (  elapsed.count() > 0 ) )
		std::this_thread::sleep_for( std::chrono::nanoseconds( elapsed ) );

	previousTime = chrono::steady_clock::now();

	masterCycles_t cyclesPerFrame = chrono::duration_cast<masterCycles_t>( frameTime );

	RGBA colorIndex[] = { { 0x00_b, 0x00_b,0x00_b, 0xFF_b }, { 0x33_b, 0x33b_b, 0x33_b, 0xFF_b }, { 0x88_b, 0x88_b, 0x88_b, 0xFF_b }, { 0xBB_b, 0xBB_b, 0xBB_b, 0xFF_b } };

	bool isRunning = true;

	WtRect imageRect = { 0, 0, NesSystem::ScreenWidth, NesSystem::ScreenHeight };

	for ( uint32_t tileY = 0; tileY < 30; ++tileY )
	{
		for ( uint32_t tileX = 0; tileX < 32; ++tileX )
		{
			DrawTile( nesSystem, nesSystem.frameBuffer, imageRect, WtPoint{ tileX, tileY }, 0x00, 0x01 );

			imageRect.x += static_cast< uint32_t >( PPU::NameTableTilePixels );
		}

		imageRect.x = 0;
		imageRect.y += static_cast< uint32_t >( PPU::NameTableTilePixels );
	}

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
				DrawTile( nesSystem, nesSystem.nameTableSheet, ntRects[ntId], WtPoint{ tileX, tileY }, ntId, 0x01 );

				ntRects[ntId].x += static_cast< uint32_t >( PPU::NameTableTilePixels );
			}

			ntRects[ntId].x = ntCorners[ntId].x;
			ntRects[ntId].y += static_cast< uint32_t >( PPU::NameTableTilePixels );
		}
	}

	DrawSprite( nesSystem, 0x00 );

	stringstream nametableName;

	nametableName << "Render Dumps/" << "nt_" << frame.count() << ".bmp";

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
		
	masterCycles_t nextCycle = chrono::duration_cast<masterCycles_t>( frameRate_t( 1 ) );

	isRunning = nesSystem.Run( nextCycle );

	++frame;

	return 0;
}



int main()
{
	RunFrame();
}