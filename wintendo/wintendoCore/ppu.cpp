#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <sstream>
#include <assert.h>
#include <map>
#include <bitset>
#include "common.h"
#include "debug.h"
#include "mos6502.h"
#include "NesSystem.h"
#include "bitmap.h"


const RGBA DefaultPalette[64] =
{
	{ 0x75, 0x75, 0x75, 0xFF },{ 0x27, 0x1B, 0x8F, 0xFF },{ 0x00, 0x00, 0xAB, 0xFF },{ 0x47, 0x00, 0x9F, 0xFF },
	{ 0x8F, 0x00, 0x77, 0xFF },{ 0xAB, 0x00, 0x13, 0xFF },{ 0xA7, 0x00, 0x00, 0xFF },{ 0x7F, 0x0B, 0x00, 0xFF },
	{ 0x43, 0x2F, 0x00, 0xFF },{ 0x00, 0x47, 0x00, 0xFF },{ 0x00, 0x51, 0x00, 0xFF },{ 0x00, 0x3F, 0x17, 0xFF },
	{ 0x1B, 0x3F, 0x5F, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },
	{ 0xBC, 0xBC, 0xBC, 0xFF },{ 0x00, 0x73, 0xEF, 0xFF },{ 0x23, 0x3B, 0xEF, 0xFF },{ 0x83, 0x00, 0xF3, 0xFF },
	{ 0xBF, 0x00, 0xBF, 0xFF },{ 0xE7, 0x00, 0x5B, 0xFF },{ 0xDB, 0x2B, 0x00, 0xFF },{ 0xCB, 0x4F, 0x0F, 0xFF },
	{ 0x8B, 0x73, 0x00, 0xFF },{ 0x00, 0x97, 0x00, 0xFF },{ 0x00, 0xAB, 0x00, 0xFF },{ 0x00, 0x93, 0x3B, 0xFF },
	{ 0x00, 0x83, 0x8B, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },
	{ 0xFF, 0xFF, 0xFF, 0xFF },{ 0x3F, 0xBF, 0xFF, 0xFF },{ 0x5F, 0x97, 0xFF, 0xFF },{ 0xA7, 0x8B, 0xFD, 0xFF },
	{ 0xF7, 0x7B, 0xFF, 0xFF },{ 0xFF, 0x77, 0xB7, 0xFF },{ 0xFF, 0x77, 0x63, 0xFF },{ 0xFF, 0x9B, 0x3B, 0xFF },
	{ 0xF3, 0xBF, 0x3F, 0xFF },{ 0x83, 0xD3, 0x13, 0xFF },{ 0x4F, 0xDF, 0x4B, 0xFF },{ 0x58, 0xF8, 0x98, 0xFF },
	{ 0x00, 0xEB, 0xDB, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },
	{ 0xFF, 0xFF, 0xFF, 0xFF },{ 0xAB, 0xE7, 0xFF, 0xFF },{ 0xC7, 0xD7, 0xFF, 0xFF },{ 0xD7, 0xCB, 0xFF, 0xFF },
	{ 0xFF, 0xC7, 0xFF, 0xFF },{ 0xFF, 0xC7, 0xDB, 0xFF },{ 0xFF, 0xBF, 0xB3, 0xFF },{ 0xFF, 0xDB, 0xAB, 0xFF },
	{ 0xFF, 0xE7, 0xA3, 0xFF },{ 0xE3, 0xFF, 0xA3, 0xFF },{ 0xAB, 0xF3, 0xBF, 0xFF },{ 0xB3, 0xFF, 0xCF, 0xFF },
	{ 0x9F, 0xFF, 0xF3, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },
};


uint8_t& PPU::Reg( uint16_t address, uint8_t value )
{
	const uint16_t regNum = ( address % 8 ); // Mirror: 2008-4000

	PpuRegWriteFunc regFunc = PpuRegWriteMap[regNum];

	return ( this->*regFunc )( value );
}


uint8_t& PPU::Reg( uint16_t address )
{
	const uint16_t regNum = ( address % 8 ); // Mirror: 2008-4000

	PpuRegReadFunc regFunc = PpuRegReadMap[regNum];

	return ( this->*regFunc )( );
}


void PPU::GenerateNMI()
{
	system->cpu.interruptTriggered = true;
}

inline void PPU::EnablePrinting()
{
#if DEBUG_ADDR == 1
	system->cpu.printToOutput = true;
#endif // #if DEBUG_ADDR == 1
}


void PPU::GenerateDMA()
{
	system->cpu.oamInProcess = true;
}


uint8_t PPU::DMA( const uint16_t address )
{
	//	assert( address == 0 ); // need to handle other case
	memcpy( primaryOAM, &system->GetMemory( Combine( 0x00, static_cast<uint8_t>( address ) ) ), 256 );

	return 0;
}


uint8_t& PPU::PPUCTRL( const uint8_t value )
{
	regCtrl.raw = value;
	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	return regCtrl.raw;
}


uint8_t& PPU::PPUCTRL()
{
	return regCtrl.raw;
}


uint8_t& PPU::PPUMASK( const uint8_t value )
{
	regMask.raw = value;
	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	return regMask.raw;
}


uint8_t& PPU::PPUMASK()
{
	return regMask.raw;
}


uint8_t& PPU::PPUSTATUS( const uint8_t value )
{
	// TODO: need to redesign to not return reference
	assert( value == 0 );

	regStatus.latched = regStatus.current;
	regStatus.latched.sem.vBlank = 0;
	regStatus.latched.sem.lastReadLsb = 0;
	regStatus.hasLatch = true;
	registers[PPUREG_ADDR] = 0;
	registers[PPUREG_DATA] = 0;
	vramAddr = 0;
	
	scrollPosition[0] = 0;
	scrollPosition[1] = 0;

	return regStatus.current.raw;
}


uint8_t& PPU::PPUSTATUS()
{
	regStatus.latched = regStatus.current;
	regStatus.latched.sem.vBlank = 0;
	regStatus.latched.sem.lastReadLsb = 0;
	regStatus.hasLatch = true;
	registers[PPUREG_ADDR] = 0;
	registers[PPUREG_DATA] = 0;
	vramAddr = 0;
	scrollReg = 0;

	scrollPosition[0] = 0;
	scrollPosition[1] = 0;

	return regStatus.current.raw;
}


uint8_t& PPU::OAMADDR( const uint8_t value )
{
	assert( value == 0x00 ); // TODO: implement other modes

	registers[PPUREG_OAMADDR] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	return registers[PPUREG_OAMADDR];
}


uint8_t& PPU::OAMADDR()
{
	return registers[PPUREG_OAMADDR];
}


uint8_t& PPU::OAMDATA( const uint8_t value )
{
	assert( 0 );

	registers[PPUREG_OAMDATA] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	return registers[PPUREG_OAMDATA];
}


uint8_t& PPU::OAMDATA()
{
	return registers[PPUREG_OAMDATA];
}


uint8_t& PPU::PPUSCROLL( const uint8_t value )
{
	registers[PPUREG_SCROLL] = value;

	scrollPosPending[scrollReg] = value;
	scrollReg ^= 0x01;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	return registers[PPUREG_SCROLL];
}


uint8_t& PPU::PPUSCROLL()
{
	return registers[PPUREG_SCROLL];
}


uint8_t& PPU::PPUADDR( const uint8_t value )
{
	registers[PPUREG_ADDR] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	vramAddr = ( vramAddr << 8 ) | ( value );

	return registers[PPUREG_ADDR];
}


uint8_t& PPU::PPUADDR()
{
	return registers[PPUREG_ADDR];
}


uint8_t& PPU::PPUDATA( const uint8_t value )
{
	if ( value == 0x62 )
	{
		//	EnablePrinting();
	}
	registers[PPUREG_DATA] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	vramWritePending = true;

	return registers[PPUREG_DATA];
}


uint8_t& PPU::PPUDATA()
{
	return registers[PPUREG_DATA];
}


uint8_t& PPU::OAMDMA( const uint8_t value )
{
	GenerateDMA();
	DMA( value );

	return registers[PPUREG_OAMDMA];
}


uint8_t& PPU::OAMDMA()
{
	GenerateDMA();
	return registers[PPUREG_OAMDMA];
}


void PPU::RenderScanline()
{
	/*
	Memory fetch phase 1 thru 128
	---------------------------- -
	1. Name table byte
	2. Attribute table byte
	3. Pattern table bitmap #0
	4. Pattern table bitmap #1
	*/
}


uint8_t PPU::GetBgPatternTableId()
{
	return regCtrl.sem.bgTableId;
}


uint8_t PPU::GetSpritePatternTableId()
{
	return regCtrl.sem.spriteTableId;
}


uint8_t PPU::GetNameTableId()
{
	return regCtrl.sem.nameTableId;
}


uint16_t PPU::MirrorVramAddr( uint16_t addr )
{
	const iNesHeader::ControlsBits0 controlBits0 = system->cart->header.controlBits0;
	const iNesHeader::ControlsBits1 controlBits1 = system->cart->header.controlBits1;

	uint32_t mirrorMode = controlBits0.fourScreenMirror ? 2 : controlBits0.mirror;

	// Major mirror
	addr %= 0x4000;

	// Nametable Mirrors
	if ( addr >= 0x3000 && addr < 0x3F00 )
	{
		addr -= 0x1000;
	}

	// Nametable Mirroring Modes
	if ( mirrorMode == 0 ) // Vertical
	{
		if ( ( addr >= 0x2400 && addr < 0x2800 ) || ( addr >= 0x2C00 && addr < 0x3000 ) )
		{
			return ( addr - NameTableAttribMemorySize );
		}
	}
	else if ( mirrorMode == 1 ) // Horizontal
	{
		if ( addr >= 0x2800 && addr < 0x3000 )
		{
			return ( addr - 2 * NameTableAttribMemorySize );
		}
	}
	else if ( mirrorMode == 2 ) // Four screen
	{
		if ( addr >= 0x2000 && addr < 0x3000 )
		{
			return 0x2000 + ( addr % NameTableAttribMemorySize );
		}
	}

	// Palette Mirrors
	if ( addr >= 0x3F20 && addr < 0x4000 )
	{
		addr = PaletteBaseAddr + ( ( addr & 0x00FF ) % 0x20 );
	}

	if ( ( addr == 0x3F10 ) || ( addr == 0x3F14 ) || ( addr == 0x3F18 ) || ( addr == 0x3F1C ) )
	{
		return ( addr - 0x0010 );
	}

	return addr;
}


uint8_t PPU::GetNtTileForPoint( const uint32_t ntId, WtPoint point, uint32_t& newNtId, WtPoint& newPoint )
{
	WtPoint scrollPoint;
	newNtId = ntId;

	point.x += scrollPosition[0];
	point.y += scrollPosition[1];

	newPoint.x = ( point.x % 256 );
	newPoint.y = ( point.y % 240 );

	point.x >>= 3;
	point.y >>= 3;

	scrollPoint.x = ( point.x % NameTableWidthTiles );
	scrollPoint.y = ( point.y % NameTableHeightTiles );

	if( scrollPoint.y < point.y )
	{
		if( ntId == 0x00 || ntId == 0x01 )
		{
			newNtId = ntId + 0x02;
		}
		else if ( ntId == 0x02 || ntId == 0x03 )
		{
			newNtId = ntId - 0x02;
		}
	}

	if ( scrollPoint.x < point.x )
	{
		if ( ntId == 0x00 || ntId == 0x02 )
		{
			newNtId = ntId + 0x01;
		}
		else if ( ntId == 0x01 || ntId == 0x03 )
		{
			newNtId = ntId - 0x01;
		}
	}

	return GetNtTile( newNtId, scrollPoint );
}


uint8_t PPU::GetNtTile( const uint32_t ntId, const WtPoint& tileCoord )
{
	const uint32_t ntBaseAddr = ( NameTable0BaseAddr + ntId * NameTableAttribMemorySize );
	
	return ReadVram( ntBaseAddr + tileCoord.x + tileCoord.y * NameTableWidthTiles );
}


uint8_t PPU::GetArribute( const uint32_t ntId, const WtPoint& point )
{
	const uint32_t ntBaseAddr		= NameTable0BaseAddr + ntId * NameTableAttribMemorySize;
	const uint32_t attribBaseAddr	= ntBaseAddr + NametableMemorySize;

	const uint8_t attribXoffset = point.x >> 5;
	const uint8_t attribYoffset = ( point.y >> 5 ) << 3;

	const uint16_t attributeAddr = attribBaseAddr + attribXoffset + attribYoffset;

	return ReadVram( attributeAddr );
}


uint8_t PPU::GetTilePaletteId( const uint32_t attribTable, const WtPoint& point )
{
	WtPoint attributeCorner;

	// The lower 3bits (0-3) specify the position within a single attrib table
	// But each attribute contains 4 nametable blocks so only every 2 nametable 
	// Blocks change the attribute therefore only bit 2 matters

	// ,---+---.
	// | . | . |
	// +---+---+
	// | . | . |
	// `---+---.

	// Byte layout, 2 bits each
	// [ Btm R | Btm L | Top R | Top L ]

	attributeCorner.x = ( point.x >> 3 ) & 0x02; // TODO: fine scroll
	attributeCorner.y = ( point.y >> 3 ) & 0x02;

	const uint8_t attribShift = attributeCorner.x + ( attributeCorner.y << 1 ); // x + y * w

	return ( ( attribTable >> attribShift ) & 0x03 ) << 2;
}


uint8_t PPU::GetChrRom( const uint32_t tileId, const uint8_t plane, const uint8_t ptrnTableId, const WtPoint point )
{
	const uint8_t tileBytes		= 16;
	const uint16_t baseAddr		= system->cart->header.prgRomBanks * NesSystem::BankSize + ptrnTableId * NesSystem::ChrRomSize;
	const uint16_t chrRomBase	= baseAddr + tileId * tileBytes;

	return system->cart->rom[chrRomBase + point.y + 8 * ( plane & 0x01 )];
}


uint8_t PPU::GetChrRomPalette( const uint8_t plane0, const uint8_t plane1, const WtPoint point )
{
	const uint16_t xBitMask = ( 0x80 >> point.x );

	const uint8_t lowerBits = ( ( plane0 & xBitMask ) >> ( 7 - point.x ) ) | ( ( ( plane1 & xBitMask ) >> ( 7 - point.x ) ) << 1 );

	return ( lowerBits & 0x03 );
}


uint8_t PPU::ReadVram( const uint16_t addr )
{
	return vram[MirrorVramAddr( /*vramAddr +*/ addr )];
}


void PPU::WriteVram()
{
	if ( vramWritePending )
	{
		vram[MirrorVramAddr( vramAddr )] = registers[PPUREG_DATA];
		vramAddr += regCtrl.sem.vramInc ? 32 : 1;

		vramWritePending = false;
	}
}


void PPU::FrameBufferWritePixel( const uint32_t x, const uint32_t y, const Pixel pixel )
{
	system->frameBuffer[x + y * NesSystem::ScreenWidth] = pixel.raw;
}


void PPU::DrawBlankScanline( uint32_t imageBuffer[], const WtRect& imageRect, const uint8_t scanY )
{
	for ( int x = 0; x < NesSystem::ScreenWidth; ++x )
	{
		Pixel pixelColor;
		Bitmap::CopyToPixel( RGBA{ 0, 0, 0, 255 }, pixelColor, BITMAP_BGRA );

		const uint32_t imageX = imageRect.x + x;
		const uint32_t imageY = imageRect.y + scanY;
		const uint32_t pixelIx = imageX + imageY * imageRect.width;

		imageBuffer[pixelIx] = pixelColor.raw;
	}
}


uint8_t PPU::DrawPixel( uint32_t imageBuffer[], const WtRect& imageRect, const uint8_t ntId, const uint8_t ptrnTableId, const WtPoint point )
{
	WtPoint nametableTile;
	WtPoint chrRomPoint;
	WtPoint scrollPoint;
	uint32_t scrollNtId;

	const uint32_t imageX = imageRect.x + point.x;
	const uint32_t imageY = imageRect.y + point.y;
	const uint32_t imageIx = imageX + imageY * imageRect.width;

	if( !regMask.sem.showBg )
	{
		Pixel pixelColor;

		const uint8_t colorIx = ReadVram(PPU::PaletteBaseAddr);

		Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

		imageBuffer[imageIx] = pixelColor.raw;

		return 0;
	}

	// Get color index
	nametableTile.x = ( point.x >> 3 );
	nametableTile.y = ( point.y >> 3 );

	const uint32_t tileIx = GetNtTileForPoint( ntId, point, scrollNtId, scrollPoint );

	const uint8_t attribute = GetArribute( scrollNtId, scrollPoint );
	const uint8_t paletteId = GetTilePaletteId( attribute, scrollPoint );

	chrRomPoint.x = ( point.x + scrollPosition[0] ) & 0x07;
	chrRomPoint.y = ( point.y + scrollPosition[1] ) & 0x07;

	const uint8_t chrRom0 = GetChrRom( tileIx, 0, ptrnTableId, chrRomPoint );
	const uint8_t chrRom1 = GetChrRom( tileIx, 1, ptrnTableId, chrRomPoint );

	const uint8_t chrRomColor = GetChrRomPalette( chrRom0, chrRom1, chrRomPoint );
	uint16_t finalPalette = paletteId | chrRomColor;

	finalPalette += PPU::PaletteBaseAddr;

	const uint8_t colorIx = ( chrRomColor == 0 ) ? ReadVram( PPU::PaletteBaseAddr ) : ReadVram( finalPalette );

	// Frame Buffer
	//Pixel oldPixelColor;
	Pixel pixelColor;

	Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

	//oldPixelColor.raw = imageBuffer[pixelIx];

	// Goofing around with frame blends, gives a fun motion blur
	// pixelColor.vec[0] = ( pixelColor.vec[0] >> 1 ) + ( oldPixelColor.vec[0] >> 1 );
	// pixelColor.vec[1] = ( pixelColor.vec[1] >> 1 ) + ( oldPixelColor.vec[1] >> 1 );
	// pixelColor.vec[2] = ( pixelColor.vec[2] >> 1 ) + ( oldPixelColor.vec[2] >> 1 );

	imageBuffer[imageIx] = pixelColor.raw;

	return chrRomColor;
}


void PPU::DrawScanline( uint32_t imageBuffer[], const WtRect& imageRect, const uint32_t lineWidth, const uint8_t scanY )
{
	WtPoint point;

	point.y = scanY;

	const uint8_t ntId = GetNameTableId();
	const uint8_t ptrnTableId = GetBgPatternTableId();

	for ( uint32_t x = 0; x < lineWidth; ++x )
	{
		point.x = x;
		DrawPixel( imageBuffer, imageRect, ntId, ptrnTableId, point );
	}
}


void PPU::DrawTile( uint32_t imageBuffer[], const WtRect& imageRect, const WtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId )
{
	for ( uint32_t y = 0; y < PPU::NameTableTilePixels; ++y )
	{
		for ( uint32_t x = 0; x < PPU::NameTableTilePixels; ++x )
		{
			WtPoint point;
			WtPoint chrRomPoint;

			point.x = 8 * nametableTile.x + x;
			point.y = 8 * nametableTile.y + y;

			const uint32_t tileIx = GetNtTile( ntId, nametableTile );

			const uint8_t attribute	= GetArribute( ntId, point );
			const uint8_t paletteId	= GetTilePaletteId( attribute, point );

			chrRomPoint.x = x;
			chrRomPoint.y = y;

			const uint8_t chrRom0 = GetChrRom( tileIx, 0, ptrnTableId, chrRomPoint );
			const uint8_t chrRom1 = GetChrRom( tileIx, 1, ptrnTableId, chrRomPoint );

			const uint16_t chrRomColor = GetChrRomPalette( chrRom0, chrRom1, chrRomPoint );
			const uint8_t finalPalette = paletteId | chrRomColor;

			const uint32_t imageX = imageRect.x + x;
			const uint32_t imageY = imageRect.y + y;

			const uint8_t colorIx = /*chrRomColor == 0 ? ReadVram( PPU::PaletteBaseAddr ) :*/ ReadVram( PPU::PaletteBaseAddr + finalPalette );

			Pixel pixelColor;

			Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

			imageBuffer[imageX + imageY * imageRect.width] = pixelColor.raw;
		}
	}
}


PpuSpriteAttrib PPU::GetSpriteData( const uint8_t spriteId, const uint8_t oam[] )
{
	PpuSpriteAttrib attribs;

	attribs.y				= oam[spriteId * 4];
	attribs.tileId			= oam[spriteId * 4 + 1]; // TODO: implement 16x8
	attribs.x				= oam[spriteId * 4 + 3];

	const uint8_t attrib	= oam[spriteId * 4 + 2]; // TODO: finish attrib features

	attribs.palette				= ( attrib & 0x03 ) << 2;
	attribs.priority			= ( attrib & 0x20 ) >> 5;
	attribs.flippedHorizontal	= ( attrib & 0x40 ) >> 6;
	attribs.flippedVertical		= ( attrib & 0x80 ) >> 7;

	return attribs;
}


void PPU::DrawSpritePixel( uint32_t imageBuffer[], const WtRect& imageRect, const PpuSpriteAttrib attribs, const WtPoint& point, const uint8_t bgPixel, bool sprite0 )
{
	if( !regMask.sem.showSprt )
	{
		return;
	}

	Pixel pixelColor;

	WtPoint spritePt;

	spritePt.x = point.x - attribs.x;
	spritePt.y = point.y - attribs.y;

	spritePt.y = attribs.flippedVertical	? ( 7 - spritePt.y ) : spritePt.y;
	spritePt.x = attribs.flippedHorizontal	? ( 7 - spritePt.x ) : spritePt.x;

	const uint8_t chrRom0 = GetChrRom( attribs.tileId, 0, 0, spritePt );
	const uint8_t chrRom1 = GetChrRom( attribs.tileId, 1, 0, spritePt );

	const uint8_t finalPalette = GetChrRomPalette( chrRom0, chrRom1, spritePt );

	const uint32_t imageX = imageRect.x + point.x;
	const uint32_t imageY = imageRect.y + point.y;

	const uint8_t colorIx = ReadVram( SpritePaletteAddr + attribs.palette + finalPalette );

	// Sprite 0 Hit happens regardless of priority but still draws normal
	if ( sprite0 )
	{
		if ( /*( bgPixel != 0 ) &&*/ ( finalPalette != 0 ) && ( point.x != 255 ) && ( regMask.sem.showBg ) )
		{
			// SMB bug: supposed to render NT0 until this is hit
			regStatus.current.sem.spriteHit = true;
		}
	}

	if ( finalPalette == 0x00 )
		return;

	if ( ( ( attribs.priority == 1 ) && ( bgPixel != 0 ) && ( finalPalette != 0 ) ) )
	{
		return;
	}

	Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

	const uint32_t imageIndex = imageX + imageY * imageRect.width;

	if ( sprite0 )
	{
		imageBuffer[imageIndex] = 0x0000FFFF;
	}
	else
	{
		imageBuffer[imageIndex] = pixelColor.raw;
	}
}


void PPU::DrawSprites( const uint32_t tableId )
{
	static WtRect imageRect = { 0, 0, NesSystem::ScreenWidth, NesSystem::ScreenHeight };

	for ( uint8_t spriteNum = 0; spriteNum < TotalSprites; ++spriteNum )
	{
		PpuSpriteAttrib attribs = GetSpriteData( spriteNum, primaryOAM );

		if ( attribs.y < 8 || attribs.y > 232 )
			continue;

		//assert( attribs.priority == 0 );

		for ( uint32_t y = 0; y < 8; ++y )
		{
			for ( uint32_t x = 0; x < 8; ++x )
			{
				uint8_t bgPixel = 0;

				WtPoint point = { x + attribs.x, y + attribs.y };

				DrawSpritePixel( system->frameBuffer, imageRect, attribs, point, bgPixel, false );
			}
		}
	}
}


void PPU::DrawDebugPalette( uint32_t imageBuffer[] )
{
	for ( uint16_t colorIx = 0; colorIx < 16; ++colorIx )
	{
		Pixel pixel;

		uint32_t color = ReadVram( PaletteBaseAddr + colorIx );
		Bitmap::CopyToPixel( palette[color], pixel, BITMAP_BGRA );
		imageBuffer[colorIx] = pixel.raw;

		color = ReadVram( SpritePaletteAddr + colorIx );
		Bitmap::CopyToPixel( palette[color], pixel, BITMAP_BGRA );
		imageBuffer[16 + colorIx] = pixel.raw;
	}
}


void PPU::LoadSecondaryOAM()
{
	if( !loadingSecondingOAM )
	{
		return;
	}

	uint8_t destSpriteNum = 0;

	sprite0InList = false;
	memset( &secondaryOAM, 0xFF, 32 );

	for ( uint8_t spriteNum = 0; spriteNum < 64; ++spriteNum )
	{
		uint8_t y = 1 + primaryOAM[spriteNum * 4];

		if ( ( beamPosition.y < ( y + 8u ) ) && ( beamPosition.y >= y ) )
		{
			secondaryOAM[destSpriteNum * 4]		= y;
			secondaryOAM[destSpriteNum * 4 + 1] = primaryOAM[spriteNum * 4 + 1];
			secondaryOAM[destSpriteNum * 4 + 2] = primaryOAM[spriteNum * 4 + 2];
			secondaryOAM[destSpriteNum * 4 + 3] = primaryOAM[spriteNum * 4 + 3];

			if( spriteNum == 0 )
			{
				sprite0InList = true;
			}

			destSpriteNum++;
		}

		if ( destSpriteNum >= 8 )
		{
			break;
		}
	}

	loadingSecondingOAM = false;
}


const ppuCycle_t PPU::Exec()
{
	// Function advances 1 - 8 cycles at a time. The logic is built on this constaint.
	static WtRect imageRect = { 0, 0, NesSystem::ScreenWidth, NesSystem::ScreenHeight };

	ppuCycle_t execCycles = ppuCycle_t( 0 );

	// Cycle timing
	const uint64_t cycleCount = cycle.count() % 341;

	if ( regStatus.hasLatch )
	{
		regStatus.current = regStatus.latched;
		regStatus.latched.raw = 0;
		regStatus.hasLatch = false;
	}

	WriteVram();

	scrollPosition[0] = scrollPosPending[0];
	scrollPosition[1] = scrollPosPending[1];

	// Scanline Work
	// Scanlines take multiple cycles
	if ( cycleCount == 0 )
	{
		if ( currentScanline < 0 )
		{
			regStatus.current.sem.vBlank = 0;
			regStatus.latched.raw = 0;
			regStatus.hasLatch = false;

			//scrollPosition[0] = scrollPosPending[0];
			//scrollPosition[1] = scrollPosPending[1];

		//	regStatus.current.sem.spriteHit = false;// Clear at dot 1
		}
		else if ( currentScanline < 240 )
		{
		}
		else if ( currentScanline == 240 )
		{
		}
		else if ( currentScanline == 241 )
		{
			// must only exec once!
			// set vblank flag at second tick, cycle=1
			regStatus.current.sem.vBlank = 1;
			//EnablePrinting();
			// Gen NMI
			if ( regCtrl.sem.nmiVblank )
				GenerateNMI();
		}
		else if ( currentScanline < 260 )
		{
		}
		else
		{
		}
	}

	if ( cycleCount == 0 )
	{
		// Idle cycle
		execCycles++;
		scanelineCycle++;

		sprite0InList = false;
		loadingSecondingOAM = true;
		LoadSecondaryOAM();

//		OAMDATA( 0xFF );
	}
	else if ( cycleCount <= 256 )
	{
		// Scanline render
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );
	
		uint8_t bgPixel = DrawPixel( system->frameBuffer, imageRect, GetNameTableId(), GetBgPatternTableId(), beamPosition );
		
		for ( uint8_t spriteNum = 0; spriteNum < 8; ++spriteNum )
		{
			PpuSpriteAttrib attribs = GetSpriteData( spriteNum, secondaryOAM );

			// FIXME: this check is too agressive
			if ( ( beamPosition.x < ( attribs.x + 8u ) ) && ( beamPosition.x >= attribs.x ) )
			{
				if ( !( attribs.y < 8 || attribs.y > 232 ) )
				{	
					DrawSpritePixel( system->frameBuffer, imageRect, attribs, beamPosition, bgPixel, ( spriteNum == 0 ) && sprite0InList );
				}
			}
		}
		
		beamPosition.x++;
	}
	else if ( cycleCount <= 320 )
	{
		// Prefetch the 8 sprites on next scanline
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );
	}
	else if ( cycleCount <= 336 )
	{
		// Prefetch first two tiles on next scanline
		execCycles++;
		scanelineCycle++;
	}
	else if ( cycleCount < 340 ) // [337 - 340], +4 cycles
	{
		// Unused fetch
		execCycles++;
		scanelineCycle++;
	}
	else
	{
		if( currentScanline >= 260 )
		{
			currentScanline = -1;

			regStatus.current.sem.spriteHit = false;// Clear at dot 1

			//scrollPosition[0] = scrollPosPending[0];
			//scrollPosition[1] = scrollPosPending[1];

			beamPosition.x = 0;
			beamPosition.y = 0;
		}
		else
		{
			currentScanline++;
			beamPosition.x = 0;

			if( currentScanline <= 240 )
			{
				beamPosition.y++;
			}
		}

		execCycles++;
		scanelineCycle = ppuCycle_t( 0 );
	}

	return execCycles;
}


bool PPU::Step( const ppuCycle_t& nextCycle )
{
	/*
	Start of vertical blanking : Set NMI_occurred in PPU to true.
	End of vertical blanking, sometime in pre - render scanline : Set NMI_occurred to false.
	Read PPUSTATUS : Return old status of NMI_occurred in bit 7, then set NMI_occurred to false.
	Write to PPUCTRL : Set NMI_output to bit 7.
	The PPU pulls / NMI low if and only if both NMI_occurred and NMI_output are true.By toggling NMI_output( PPUCTRL.7 ) during vertical blank without reading PPUSTATUS, a program can cause / NMI to be pulled low multiple times, causing multiple NMIs to be generated.
	*/

	while ( cycle < nextCycle )
	{
		cycle += Exec();
	}

	return true;
}
