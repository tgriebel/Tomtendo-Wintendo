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

	regT.sem.ntId = regCtrl.sem.ntId;

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

	regW = 0;

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

	if( regW == 0 )
	{
		regX = ( value & 0x07 );
		regT.sem.coarseX = value >> 3;

		nextTilePixels = regX;
	}
	else
	{
		regT.sem.fineY = value & 0x07;
		regT.sem.coarseY = ( value & 0xF8 ) >> 3;
	}
	
	regW ^= 0x01;

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

	if( regW == 0 )
	{
		regT.raw = ( regT.raw & 0x00FF) | ( ( value & 0x3F ) << 8 );
	}
	else
	{
		regT.raw = ( regT.raw & 0xFF00 ) | value;
		regV = regT;
		//regV0 = regT;
	}

	regW ^= 1;

	return registers[PPUREG_ADDR];
}


uint8_t& PPU::PPUADDR()
{
	return registers[PPUREG_ADDR];
}


uint8_t& PPU::PPUDATA( const uint8_t value )
{
	registers[PPUREG_DATA] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	vramWritePending = true;

	return registers[PPUREG_DATA];
}


uint8_t& PPU::PPUDATA()
{
	registers[PPUREG_DATA] = ReadVram( regV0.raw );

	regV0.raw += regCtrl.sem.vramInc ? 32 : 1;

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


void PPU::BgPipelineFetch( const uint64_t scanlineCycle )
{
	/*
	Memory fetch phase 1 thru 128
	---------------------------- -
	1. Name table byte
	2. Attribute table byte
	3. Pattern table bitmap #0
	4. Pattern table bitmap #1
	*/

	if ( !fetchMode )
		return;

	uint32_t cycleCountAdjust = ( scanlineCycle - 1 ) % 8;
	if ( cycleCountAdjust == 0 )
	{
		if ( plLatches.flags == 0x1 )
		{
			plShifts[curShift] = plLatches;
			plLatches.flags = 0;
			curShift ^= 0x1;
		}

		plLatches.tileId = ReadVram( NameTable0BaseAddr | ( regV0.raw & 0x0FFF ) );
	}
	else if ( cycleCountAdjust == 2 )
	{
		plLatches.attribId = ReadVram( AttribTable0BaseAddr | ( regV0.sem.ntId << 10 ) | ( ( regV0.sem.coarseY << 1 ) & 0xF8 ) | ( regV0.sem.coarseX >> 2 ) );
	}
	else if ( cycleCountAdjust == 4 )
	{
		plLatches.chrRom0 = GetChrRom( plLatches.tileId, 0, GetBgPatternTableId(), regV0.sem.fineY );
	}
	else if ( cycleCountAdjust == 6 )
	{
		plLatches.chrRom1 = GetChrRom( plLatches.tileId, 1, GetBgPatternTableId(), regV0.sem.fineY );
		plLatches.flags = 0x1;
	}
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
	return regCtrl.sem.ntId;
}


uint16_t PPU::MirrorVram( uint16_t addr )
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

	attributeCorner.x = ( point.x >> 3 ) & 0x02;
	attributeCorner.y = ( point.y >> 3 ) & 0x02;

	const uint8_t attribShift = attributeCorner.x + ( attributeCorner.y << 1 ); // x + y * w

	return ( ( attribTable >> attribShift ) & 0x03 ) << 2;
}


uint8_t PPU::GetChrRom( const uint32_t tileId, const uint8_t plane, const uint8_t ptrnTableId, const uint8_t row )
{
	const uint8_t tileBytes		= 16;
	const uint16_t baseAddr		= ptrnTableId * NesSystem::ChrRomSize;
	const uint16_t chrRomBase	= baseAddr + tileId * tileBytes;

	return ReadVram( chrRomBase + row + 8 * ( plane & 0x01 ) );
}


uint8_t PPU::GetChrRomPalette( const uint8_t plane0, const uint8_t plane1, const uint8_t col )
{
	const uint16_t xBitMask = ( 0x80 >> col );

	const uint8_t lowerBits = ( ( plane0 & xBitMask ) >> ( 7 - col ) ) | ( ( ( plane1 & xBitMask ) >> ( 7 - col ) ) << 1 );

	return ( lowerBits & 0x03 );
}


uint8_t PPU::ReadVram( const uint16_t addr )
{
	if( addr < 0x2000 )
	{
		const uint16_t baseAddr = system->cart->header.prgRomBanks * NesSystem::BankSize;

		return system->cart->rom[baseAddr + addr];
	}
	else
	{
		return vram[MirrorVram( addr )];
	}
}


void PPU::WriteVram()
{
	if ( vramWritePending )
	{
		vram[MirrorVram( /*regV0.raw*/regV.raw )] = registers[PPUREG_DATA];
		regV.raw += regCtrl.sem.vramInc ? 32 : 1;
		//regV0.raw += regCtrl.sem.vramInc ? 32 : 1;

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


uint8_t PPU::DrawPixel( uint32_t imageBuffer[], const WtRect& imageRect, const uint8_t ntId, const uint8_t ptrnTableId, const WtPoint courseOffset, const uint8_t tileX )
{
	WtPoint chrRomPoint;

	const uint32_t imageX = imageRect.x;
	const uint32_t imageY = imageRect.y;
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
	chrRomPoint.x = ( nextTilePixels ) & 0x07;
	chrRomPoint.y = regV0.sem.fineY;

	uint32_t tileIx;
	uint8_t attribute;
	uint8_t chrRom0;
	uint8_t chrRom1;

	if( fetchMode )
	{
		tileIx = plShifts[curShift].tileId;
		attribute = plShifts[curShift].attribId;
		chrRom0 = plShifts[curShift].chrRom0;
		chrRom1 = plShifts[curShift].chrRom1;
	}
	else
	{
		tileIx = ReadVram( NameTable0BaseAddr | ( regV0.raw & 0x0FFF ) );
		attribute = ReadVram( AttribTable0BaseAddr | ( regV0.sem.ntId << 10 ) | ( ( regV0.sem.coarseY << 1 ) & 0xF8 ) | ( regV0.sem.coarseX >> 2 ) );
		chrRom0 = GetChrRom( tileIx, 0, ptrnTableId, chrRomPoint.y );
		chrRom1 = GetChrRom( tileIx, 1, ptrnTableId, chrRomPoint.y );
	}

	const uint8_t paletteId = GetTilePaletteId( attribute, courseOffset );

	const uint8_t chrRomColor = GetChrRomPalette( chrRom0, chrRom1, chrRomPoint.x );
	uint16_t finalPalette = paletteId | chrRomColor;

	finalPalette += PPU::PaletteBaseAddr;

	const uint8_t colorIx = ( chrRomColor == 0 ) ? ReadVram( PPU::PaletteBaseAddr ) : ReadVram( finalPalette );

	// Frame Buffer
	Pixel pixelColor;
	Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

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
		DrawPixel( imageBuffer, imageRect, ntId, ptrnTableId, point, x % 8 );
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

			const uint8_t chrRom0 = GetChrRom( tileIx, 0, ptrnTableId, chrRomPoint.y );
			const uint8_t chrRom1 = GetChrRom( tileIx, 1, ptrnTableId, chrRomPoint.y );

			const uint16_t chrRomColor = GetChrRomPalette( chrRom0, chrRom1, chrRomPoint.x );
			const uint8_t finalPalette = paletteId | chrRomColor;

			const uint32_t imageX = imageRect.x + x;
			const uint32_t imageY = imageRect.y + y;

			const uint8_t colorIx = chrRomColor == 0 ? ReadVram( PPU::PaletteBaseAddr ) : ReadVram( PPU::PaletteBaseAddr + finalPalette );

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

	const uint8_t chrRom0 = GetChrRom( attribs.tileId, 0, 0, spritePt.y );
	const uint8_t chrRom1 = GetChrRom( attribs.tileId, 1, 0, spritePt.y );

	const uint8_t finalPalette = GetChrRomPalette( chrRom0, chrRom1, spritePt.x );

	const uint32_t imageX = imageRect.x;
	const uint32_t imageY = imageRect.y;

	const uint8_t colorIx = ReadVram( SpritePaletteAddr + attribs.palette + finalPalette );

	// Sprite 0 Hit happens regardless of priority but still draws normal
	if ( sprite0 )
	{
		if ( ( bgPixel != 0 ) && ( finalPalette != 0 ) && ( point.x != 255 ) && ( regMask.sem.showBg ) )
		{
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

	imageBuffer[imageIndex] = pixelColor.raw;
}


void PPU::DrawSprites( const uint32_t tableId )
{
	static WtRect imageRect = { 0, 0, NesSystem::ScreenWidth, NesSystem::ScreenHeight };

	for ( uint8_t spriteNum = 0; spriteNum < TotalSprites; ++spriteNum )
	{
		PpuSpriteAttrib attribs = GetSpriteData( spriteNum, primaryOAM );

		if ( attribs.y < 8 || attribs.y > 232 )
			continue;

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


void PPU::AdvanceXScroll( const uint64_t cycleCount )
{
	if ( ( nextTilePixels ) >= 8 )
	{
		if ( regV0.sem.coarseX == 0x001F )
		{
			regV0.sem.coarseX = 0;
			regV0.sem.ntId ^= 0x1;
		}
		else
		{
			regV0.sem.coarseX++;
		}

		nextTilePixels = 0;
	}
}


void PPU::AdvanceYScroll( const uint64_t cycleCount )
{
	if ( RenderEnabled() )
	{
		if ( regV0.sem.fineY < 7 )
		{
			regV0.sem.fineY++;
		}
		else
		{
			regV0.sem.fineY = 0;

			if ( regV0.sem.coarseY == 29 ) // > 240 vertical pixel
			{
				regV0.sem.coarseY = 0;
				regV0.sem.ntId ^= 0x2;
			}
			else if ( regV0.sem.coarseY == 31 ) // This is legal to set, but causes odd behavior
			{
				regV0.sem.coarseY = 0;
			}
			else
			{
				regV0.sem.coarseY++;
			}
		}
	}
}


const ppuCycle_t PPU::Exec()
{
	// Function advances 1 - 8 cycles at a time. The logic is built on this constaint.
	static WtRect imageRect = { 0, 0, NesSystem::ScreenWidth, NesSystem::ScreenHeight };

	fetchMode = false;

	ppuCycle_t execCycles = ppuCycle_t( 0 );

	// Cycle timing
	const uint64_t cycleCount = cycle.count() % ScanlineCycles;

	if ( regStatus.hasLatch )
	{
		regStatus.current = regStatus.latched;
		regStatus.latched.raw = 0;
		regStatus.hasLatch = false;
	}

	WriteVram();

	if ( currentScanline == 240 )
	{
		++currentScanline;

		execCycles		+= ppuCycle_t( ScanlineCycles );
		scanelineCycle	+= ppuCycle_t( ScanlineCycles );

		return execCycles;
	}

	if ( currentScanline < 0 )
	{
		if ( ( cycleCount >= 281 ) && ( cycleCount <= 305 ) )
		{
			if ( RenderEnabled() )
			{
			//	regV0.sem.fineY = regT.sem.fineY;
			//	regV0.sem.coarseY = regT.sem.coarseY;
			//	regV0.sem.ntId = ( regV0.sem.ntId & 0x1 ) | ( regT.sem.ntId & 0x2 );
			}
		}
	}

	// Scanline Work
	// Scanlines take multiple cycles

	if ( cycleCount == 0 )
	{
		if ( currentScanline < 0 )
		{
			regStatus.current.sem.vBlank = 0;
			regStatus.latched.raw = 0;
			regStatus.hasLatch = false;

			regV0.sem.fineY = regT.sem.fineY;
			regV0.sem.coarseY = regT.sem.coarseY;
			regV0.sem.ntId = ( regV0.sem.ntId & 0x1 ) | ( regT.sem.ntId & 0x2 );
		}
		else if ( currentScanline == 241 )
		{
			// must only exec once!
			// set vblank flag at second tick, cycle=1
			regStatus.current.sem.vBlank = 1;
			//EnablePrinting();
			// Gen NMI
			if ( regCtrl.sem.nmiVblank )
			{
				GenerateNMI();
			}
		}

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
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );

		WtPoint screenPt;
		screenPt.x = ( regV0.sem.coarseX << 3 );
		screenPt.y = ( regV0.sem.coarseY << 3 );

		BgPipelineFetch( cycleCount );
		AdvanceXScroll( cycleCount );

		// Scanline render
		imageRect.x = beamPosition.x;
		imageRect.y = beamPosition.y;

		uint8_t bgPixel = DrawPixel( system->frameBuffer, imageRect, regV0.sem.ntId/*GetNameTableId()*/, GetBgPatternTableId(), screenPt, 0 );
		
		for ( uint8_t spriteNum = 0; spriteNum < 8; ++spriteNum )
		{
			PpuSpriteAttrib attribs = GetSpriteData( spriteNum, secondaryOAM );

			if ( ( beamPosition.x < ( attribs.x + 8u ) ) && ( beamPosition.x >= attribs.x ) )
			{
				if ( !( attribs.y < 8 || attribs.y > 232 ) )
				{
					DrawSpritePixel( system->frameBuffer, imageRect, attribs, beamPosition, bgPixel, ( spriteNum == 0 ) && sprite0InList );
				}
			}
		}

		beamPosition.x++;
		nextTilePixels++;
	}
	else if ( cycleCount == 257 )
	{
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );

		AdvanceYScroll( cycleCount );
	}
	else if ( cycleCount == 258 )
	{
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );

		if ( RenderEnabled() )
		{
			regV0.sem.coarseX = regT.sem.coarseX;
			regV0.sem.ntId = ( regV0.sem.ntId & 0x2 ) | ( regT.sem.ntId & 0x1 );
		}
	}
	else if ( cycleCount <= 320 )
	{
		// Prefetch the 8 sprites on next scanline
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );

		BgPipelineFetch( cycleCount ); // Garbage fetches
	}
	else if ( cycleCount <= 336 )
	{
		execCycles++;
		scanelineCycle++;

		BgPipelineFetch( cycleCount ); // Prefetch first two tiles on next scanline
	}
	else if ( cycleCount < 340 ) // [337 - 340], +4 cycles
	{
		// 2 unused fetches
		execCycles++;
		scanelineCycle++;
	}
	else
	{
		if( currentScanline >= 260 )
		{
			currentScanline = -1;

			regStatus.current.sem.spriteHit = false;// Clear at dot 1

			nextTilePixels = regX;
			beamPosition.x = 0;
			beamPosition.y = 0;
		}
		else
		{
			currentScanline++;
			beamPosition.x = 0;
			beamPosition.x = 0;
			nextTilePixels = regX;

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
