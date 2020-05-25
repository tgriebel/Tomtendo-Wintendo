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


void PPU::GenerateDMA()
{
	system->cpu.oamInProcess = true;
}


uint8_t PPU::DMA( const uint16_t address )
{
	const void* memoryAddress = &system->GetMemory( wtSystem::PageSize * static_cast<uint8_t>( address ) );
	memcpy( primaryOAM, memoryAddress, wtSystem::PageSize );
	return 0;
}


uint8_t& PPU::PPUCTRL( const uint8_t value )
{
	regCtrl.raw = value;
	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	regT.sem.ntId = static_cast<uint8_t>( regCtrl.sem.ntId );

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
	if( DataportEnabled() )
	{
		registers[PPUREG_DATA] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		vramAccessed = true;
		vramWritePending = true;
	}

	return registers[PPUREG_DATA];
}


uint8_t& PPU::PPUDATA()
{
	// ppuReadBuffer[1] is the internal read buffer but ppuReadBuffer[0] is used to return a 'safe' writable byte
	// This is due to the current (wonky) memory design
	ppuReadBuffer[0] = ppuReadBuffer[1];

	if ( DataportEnabled() )
	{
		ppuReadBuffer[1] = ReadVram( regV.raw );

		if( regV.raw >= PaletteBaseAddr )
		{
			ppuReadBuffer[0] = ppuReadBuffer[1];
		}

		vramAccessed = true;
	}

	return ppuReadBuffer[0];
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


void PPU::IncRenderAddr()
{
	if( DataportEnabled() )
	{
		const bool isLargeIncrement = static_cast<bool>( regCtrl.sem.vramInc );
		regV.raw += isLargeIncrement ? 0x20 : 0x01;
	}
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

	if ( !BgDataFetchEnabled() )
	{
		return;
	}

	uint32_t cycleCountAdjust = ( scanlineCycle - 1 ) % 8;
	if ( cycleCountAdjust == 1 )
	{
		plLatches.tileId = ReadVram( NameTable0BaseAddr | ( regV.raw & 0x0FFF ) );
	}
	else if ( cycleCountAdjust == 3 )
	{
		// The problem might be an issue with coarseX being off +/-
		wtPoint coarsePt;
		coarsePt.x = regV.sem.coarseX;
		coarsePt.y = regV.sem.coarseY;

		//const uint32_t attribute = GetArribute( regV0.sem.ntId, coarsePt );
		const uint32_t attribute = ReadVram( AttribTable0BaseAddr | ( regV.sem.ntId << 10 ) | ( ( regV.sem.coarseY << 1 ) & 0xF8 ) | ( ( regV.sem.coarseX >> 2 ) & 0x07 ) );		

		plLatches.attribId = GetTilePaletteId( attribute, coarsePt );
	}
	else if ( cycleCountAdjust == 5 )
	{
		plLatches.chrRom0 = GetChrRom8x8( plLatches.tileId, 0, GetBgPatternTableId(), regV.sem.fineY );
	}
	else if( cycleCountAdjust == 7 )
	{
		plLatches.chrRom1 = GetChrRom8x8( plLatches.tileId, 1, GetBgPatternTableId(), regV.sem.fineY );
		plLatches.flags = 0x1;
		AdvanceXScroll( scanlineCycle );

		plShifts[curShift] = plLatches;

		chrShifts[0] = ( chrShifts[0] & 0xFF00 ) | plLatches.chrRom0;
		chrShifts[1] = ( chrShifts[1] & 0xFF00 ) | plLatches.chrRom1;

		palLatch[0] = ( plShifts[curShift].attribId >> 2 ) & 0x01;
		palLatch[1] = ( ( plShifts[curShift].attribId >> 2 ) & 0x2 ) >> 1;

		palShifts[0] = 0;
		palShifts[1] = 0;

		for ( uint8_t bit = 0; bit < 8; ++bit )
		{
			palShifts[0] <<= 1;
			palShifts[1] <<= 1;
			palShifts[0] |= ( plShifts[curShift^1].attribId >> 2 ) & 0x1;
			palShifts[1] |= ( ( plShifts[curShift^1].attribId >> 2 ) & 0x2 ) >> 1;
		}

		curShift ^= 0x1;
	}
}


uint8_t PPU::GetBgPatternTableId()
{
	return static_cast<uint8_t>( regCtrl.sem.bgTableId );
}


uint8_t PPU::GetSpritePatternTableId()
{
	return static_cast<uint8_t>( regCtrl.sem.spriteTableId );
}


uint8_t PPU::GetNameTableId()
{
	return regV.sem.ntId;
}


uint16_t PPU::MirrorVram( uint16_t addr )
{
	const wtRomHeader::ControlsBits0 controlBits0 = system->cart->header.controlBits0;
	const wtRomHeader::ControlsBits1 controlBits1 = system->cart->header.controlBits1;

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


uint8_t PPU::GetNtTile( const uint32_t ntId, const wtPoint& tileCoord )
{
	const uint32_t ntBaseAddr = ( NameTable0BaseAddr + ntId * NameTableAttribMemorySize );
	
	return ReadVram( ntBaseAddr + tileCoord.x + tileCoord.y * NameTableWidthTiles );
}


uint8_t PPU::GetArribute( const uint32_t ntId, const wtPoint& tileCoord )
{
	const uint32_t ntBaseAddr		= NameTable0BaseAddr + ntId * NameTableAttribMemorySize;
	const uint32_t attribBaseAddr	= ntBaseAddr + NametableMemorySize;

	const uint8_t attribXoffset = tileCoord.x >> 2;
	const uint8_t attribYoffset = ( tileCoord.y >> 2 ) << 3;

	const uint16_t attributeAddr = attribBaseAddr + attribXoffset + attribYoffset;

	return ReadVram( attributeAddr );
}


uint8_t PPU::GetTilePaletteId( const uint32_t attribTable, const wtPoint& tileCoord )
{
	wtPoint attributeCorner;

	// The lower 2bits (0-3) specify the position within a single attrib table
	// Each attribute contains 4 nametable blocks
	// Every two nametable blocks change the attribute meaning only bit 2 matters

	// ,---+---.
	// | . | . |
	// +---+---+
	// | . | . |
	// `---+---.

	// Byte layout, 2 bits each
	// [ Btm R | Btm L | Top R | Top L ]

	attributeCorner.x = tileCoord.x & 0x02;
	attributeCorner.y = tileCoord.y & 0x02;

	const uint8_t attribShift = attributeCorner.x + ( attributeCorner.y << 1 ); // x + y * w

	return ( ( attribTable >> attribShift ) & 0x03 ) << 2;
}


uint8_t PPU::GetChrRom8x8( const uint32_t tileId, const uint8_t plane, const uint8_t ptrnTableId, const uint8_t row )
{
	const uint8_t tileBytes		= 16;
	const uint16_t baseAddr		= ptrnTableId * wtSystem::ChrRomSize;
	const uint16_t chrRomBase	= baseAddr + tileId * tileBytes;

	return ReadVram( chrRomBase + row + 8 * ( plane & 0x01 ) );
}


uint8_t PPU::GetChrRom8x16( const uint32_t tileId, const uint8_t plane, const uint8_t row, const bool isUpper )
{
	const uint8_t tileBytes = 16;
	const uint16_t baseAddr = ( tileId & 0x01 ) * wtSystem::ChrRomSize;
	const uint16_t chrRomBase = baseAddr + ( ( tileId & ~0x01 ) + isUpper ) * tileBytes;

	return ReadVram( chrRomBase + row + 8 * ( plane & 0x01 ) );
}


uint8_t PPU::GetChrRomPalette( const uint8_t plane0, const uint8_t plane1, const uint8_t col )
{
	const uint16_t xBitMask = ( 0x80 >> col );

	const uint8_t lowerBits = ( ( plane0 & xBitMask ) >> ( 7 - col ) ) | ( ( ( plane1 & xBitMask ) >> ( 7 - col ) ) << 1 );

	return ( lowerBits & 0x03 );
}

static int chrRamAddr = 0;

uint8_t PPU::ReadVram( const uint16_t addr )
{
	// TODO: Has CHR-RAM check
	bool isUnrom = system->cart->header.controlBits0.mapperNumberLower == 2;
	if ( addr >= 0x2000 )
	{
		const uint16_t adjustedAddr = MirrorVram( addr );
		assert( adjustedAddr < VirtualMemorySize );
		return vram[adjustedAddr];
	}
	else if ( isUnrom && ( addr < 0x2000 ) )
	{
		const uint16_t adjustedAddr = MirrorVram( addr );
		assert( adjustedAddr < VirtualMemorySize );
		return vram[adjustedAddr];
	}
	else
	{
		const uint16_t baseAddr = system->cart->header.prgRomBanks * wtSystem::BankSize;

		return system->cart->rom[baseAddr + addr];
	}
}


void PPU::WriteVram()
{
	if ( vramWritePending )
	{
		if ( DataportEnabled() )
		{
			const uint16_t adjustedAddr = MirrorVram( regV.raw );

			if ( adjustedAddr >= 0x2000 )
			{
				vram[adjustedAddr] = registers[PPUREG_DATA];
				debugVramWriteCounter[adjustedAddr]++;
			}

			static uint32_t addr = 0x1000;
			// TODO: Has CHR-RAM check
			bool isUnrom = system->cart->header.controlBits0.mapperNumberLower == 2;
			if ( isUnrom && ( adjustedAddr < 0x2000 ) )
			{
				vram[adjustedAddr] = registers[PPUREG_DATA];

				debugVramWriteCounter[adjustedAddr]++;
			}
		}

		vramWritePending = false;
	}
}


void PPU::FrameBufferWritePixel( const uint32_t x, const uint32_t y, const Pixel pixel )
{
	system->frameBuffer.SetPixel( x, y, pixel );
}


void PPU::DrawBlankScanline( wtRawImage& imageBuffer, const wtRect& imageRect, const uint8_t scanY )
{
	for ( int x = 0; x < wtSystem::ScreenWidth; ++x )
	{
		Pixel pixelColor;
		Bitmap::CopyToPixel( RGBA{ 0, 0, 0, 255 }, pixelColor, BITMAP_BGRA );

		const uint32_t imageX = imageRect.x + x;
		const uint32_t imageY = imageRect.y + scanY;
		const uint32_t pixelIx = imageX + imageY * imageRect.width;

		imageBuffer.Set( pixelIx, pixelColor );
	}
}


void PPU::DrawPixel( wtRawImage& imageBuffer, const wtRect& imageRect )
{

}


uint8_t PPU::BgPipelineDecodePalette()
{
	uint8_t chrRomColor;

	uint8_t paletteId = plShifts[curShift].attribId;
	uint8_t chrRom0 = static_cast< uint8_t >( chrShifts[0] >> 8 );
	uint8_t chrRom1 = static_cast< uint8_t >( chrShifts[1] >> 8 );

	paletteId = GetChrRomPalette( palShifts[0], palShifts[1], static_cast< uint8_t >( regX ) );
	paletteId <<= 2;

	static uint8_t shiftRegX = 0;

	chrRomColor = GetChrRomPalette( chrRom0, chrRom1, static_cast< uint8_t >( regX ) );

	return ( paletteId | chrRomColor );
}


void PPU::DrawTile( wtRawImage& imageBuffer, const wtRect& imageRect, const wtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId )
{
	for ( uint32_t y = 0; y < PPU::TilePixels; ++y )
	{
		for ( uint32_t x = 0; x < PPU::TilePixels; ++x )
		{
			wtPoint point;
			wtPoint chrRomPoint;

			point.x = 8 * nametableTile.x + x;
			point.y = 8 * nametableTile.y + y;

			const uint32_t tileIx = GetNtTile( ntId, nametableTile );

			const uint8_t attribute	= GetArribute( ntId, nametableTile );
			const uint8_t paletteId	= GetTilePaletteId( attribute, nametableTile );

			chrRomPoint.x = x;
			chrRomPoint.y = y;

			const uint8_t chrRom0 = GetChrRom8x8( tileIx, 0, ptrnTableId, chrRomPoint.y );
			const uint8_t chrRom1 = GetChrRom8x8( tileIx, 1, ptrnTableId, chrRomPoint.y );

			const uint16_t chrRomColor = GetChrRomPalette( chrRom0, chrRom1, chrRomPoint.x );
			const uint8_t finalPalette = paletteId | chrRomColor;

			const uint32_t imageX = imageRect.x + x;
			const uint32_t imageY = imageRect.y + y;

			const uint8_t colorIx = chrRomColor == 0 ? ReadVram( PPU::PaletteBaseAddr ) : ReadVram( PPU::PaletteBaseAddr + finalPalette );

			Pixel pixelColor;

			Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

			imageBuffer.Set( imageX + imageY * imageRect.width, pixelColor );
		}
	}
}


void PPU::DrawTile( wtRawImage& imageBuffer, const wtRect& imageRect, const uint32_t tileId, const uint32_t ptrnTableId )
{
	for ( uint32_t y = 0; y < PPU::TilePixels; ++y )
	{
		for ( uint32_t x = 0; x < PPU::TilePixels; ++x )
		{
			wtPoint chrRomPoint;

			chrRomPoint.x = x;
			chrRomPoint.y = y;

			static uint32_t toggleTable = 0;

			const uint8_t chrRom0 = toggleTable == 1 ? 0 : GetChrRom8x8( tileId, 0, ptrnTableId, chrRomPoint.y );
			const uint8_t chrRom1 = toggleTable == 2 ? 0 : GetChrRom8x8( tileId, 1, ptrnTableId, chrRomPoint.y );

			const uint16_t chrRomColor = GetChrRomPalette( chrRom0, chrRom1, chrRomPoint.x );

			const uint32_t imageX = imageRect.x + x;
			const uint32_t imageY = imageRect.y + y;

			const uint8_t colorIx = ReadVram( PPU::PaletteBaseAddr + chrRomColor );

			Pixel pixelColor;

			Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

			imageBuffer.Set( imageX + imageY * imageRect.width, pixelColor );
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


void PPU::DrawSpritePixel( wtRawImage& imageBuffer, const wtRect& imageRect, const PpuSpriteAttrib attribs, const wtPoint& point, const uint8_t bgPixel, bool sprite0 )
{
	if( !regMask.sem.showSprt )
	{
		return;
	}

	Pixel pixelColor;

	wtPoint spritePt;

	spritePt.x = point.x - attribs.x;
	spritePt.y = point.y - attribs.y;
	spritePt.x = attribs.flippedHorizontal	? ( 7 - spritePt.x ) : spritePt.x;

	uint8_t chrRom0 = 0;
	uint8_t chrRom1 = 0; 

	if ( regCtrl.sem.spriteSize == SpriteMode::SPRITE_MODE_8x16 )
	{
		bool isUpper = ( spritePt.y >= 8 );
		uint8_t row = ( spritePt.y % 8 );

		if ( attribs.flippedVertical )
		{
			row = ( 7 - row );
			isUpper = !isUpper;
		}

		chrRom0 = GetChrRom8x16( attribs.tileId, 0, row, isUpper );
		chrRom1 = GetChrRom8x16( attribs.tileId, 1, row, isUpper );
	}
	else
	{
		spritePt.y = attribs.flippedVertical ? ( 7 - spritePt.y ) : spritePt.y;

		chrRom0 = GetChrRom8x8( attribs.tileId, 0, 0, spritePt.y );
		chrRom1 = GetChrRom8x8( attribs.tileId, 1, 0, spritePt.y );
	}

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

	if ( ( attribs.priority == 1 ) && ( bgPixel != 0 ) )
	{
		return;
	}

	Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

	const uint32_t imageIndex = imageX + imageY * imageRect.width;

	imageBuffer.Set( imageIndex, pixelColor );
}


void PPU::DrawSprites( const uint32_t tableId )
{
	static wtRect imageRect = { 0, 0, wtSystem::ScreenWidth, wtSystem::ScreenHeight };

	for ( uint8_t spriteNum = 0; spriteNum < TotalSprites; ++spriteNum )
	{
		PpuSpriteAttrib attribs = GetSpriteData( spriteNum, primaryOAM );

		if ( attribs.y < 8 || attribs.y > 232 )
			continue;

		for ( int32_t y = 0; y < 8; ++y )
		{
			for ( int32_t x = 0; x < 8; ++x )
			{
				uint8_t bgPixel = 0;

				wtPoint point = { x + attribs.x, y + attribs.y };

				DrawSpritePixel( system->frameBuffer, imageRect, attribs, point, bgPixel, false );
			}
		}
	}
}


void PPU::DrawDebugPalette( wtRawImage& imageBuffer )
{
	for ( uint16_t colorIx = 0; colorIx < 16; ++colorIx )
	{
		Pixel pixel;

		uint32_t color = ReadVram( PaletteBaseAddr + colorIx );
		Bitmap::CopyToPixel( palette[color], pixel, BITMAP_BGRA );
		imageBuffer.Set( colorIx, pixel );

		color = ReadVram( SpritePaletteAddr + colorIx );
		Bitmap::CopyToPixel( palette[color], pixel, BITMAP_BGRA );
		imageBuffer.Set( 16 + colorIx, pixel );
	}
}


void PPU::LoadSecondaryOAM()
{
	if( !loadingSecondaryOAM )
	{
		return;
	}

	uint8_t destSpriteNum = 0;

	sprite0InList = false;
	memset( &secondaryOAM, 0xFF, OamSize );

	for ( uint8_t spriteNum = 0; spriteNum < 64; ++spriteNum )
	{
		uint8_t y = 1 + primaryOAM[spriteNum * 4];
		const bool isLargeSpriteMode = static_cast<bool>( regCtrl.sem.spriteSize );
		uint32_t spriteHeight = isLargeSpriteMode ? 16 : 8;

		if ( ( beamPosition.y < static_cast<int32_t>( y + spriteHeight ) ) && ( beamPosition.y >= static_cast<int32_t>( y ) ) )
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

		if ( destSpriteNum >= spriteLimit )
		{
			break;
		}
	}

	loadingSecondaryOAM = false;
}


void PPU::AdvanceXScroll( const uint64_t cycleCount )
{
	if ( RenderEnabled() )
	{
		if ( regV.sem.coarseX == 0x001F )
		{
			regV.sem.coarseX = 0;
			regV.sem.ntId ^= 0x1;
		}
		else
		{
			regV.sem.coarseX++;
		}
	}
}


void PPU::AdvanceYScroll( const uint64_t cycleCount )
{
	if ( RenderEnabled() )
	{
		if ( regV.sem.fineY < 7 )
		{
			regV.sem.fineY++;
		}
		else
		{
			regV.sem.fineY = 0;

			if ( regV.sem.coarseY == 29 ) // > 240 vertical pixel
			{
				regV.sem.coarseY = 0;
				regV.sem.ntId ^= 0x2;
			}
			else if ( regV.sem.coarseY == 31 ) // This is legal to set, but causes odd behavior
			{
				regV.sem.coarseY = 0;
			}
			else
			{
				regV.sem.coarseY++;
			}
		}
	}
}


void PPU::BgPipelineShiftRegisters()
{
	if ( !BgDataFetchEnabled() )
	{
		return;
	}

	chrShifts[0] <<= 1;
	chrShifts[1] <<= 1;
	palShifts[0] <<= 1;
	palShifts[1] <<= 1;
	palShifts[0] |= palLatch[0] & 0x1;
	palShifts[1] |= palLatch[1] & 0x1;
}


void PPU::BgPipelineDebugPrefetchFetchTiles()
{
	if ( !BgDataFetchEnabled() )
	{
		return;
	}

	if ( debugPrefetchTiles )
	{
		plShifts[0].tileId = 1;
		plShifts[0].attribId = 2;
		plShifts[0].chrRom0 = GetChrRom8x8( plShifts[0].tileId, 0, GetBgPatternTableId(), regV.sem.fineY );
		plShifts[0].chrRom1 = GetChrRom8x8( plShifts[0].tileId, 1, GetBgPatternTableId(), regV.sem.fineY );

		plShifts[1].tileId = 2;
		plShifts[1].attribId = 4;
		plShifts[1].chrRom0 = GetChrRom8x8( plShifts[1].tileId, 0, GetBgPatternTableId(), regV.sem.fineY );
		plShifts[1].chrRom1 = GetChrRom8x8( plShifts[1].tileId, 1, GetBgPatternTableId(), regV.sem.fineY );

		chrShifts[0] = ( plShifts[0].chrRom0 << 8 ) | plShifts[1].chrRom0;
		chrShifts[1] = ( plShifts[0].chrRom1 << 8 ) | plShifts[1].chrRom1;
	}
}


bool PPU::BgDataFetchEnabled()
{
	return ( ( currentScanline < POSTRENDER_SCANLINE ) || ( currentScanline == PRERENDER_SCANLINE ) );
}


bool PPU::RenderEnabled()
{
	return ( regMask.sem.showBg || regMask.sem.showSprt ) && BgDataFetchEnabled();
}


bool PPU::InVBlank()
{
	return inVBlank;
}


bool PPU::DataportEnabled()
{
	return ( InVBlank() || !RenderEnabled() );
}


const ppuCycle_t PPU::Exec()
{
	// Function advances 1 - 8 cycles at a time. The logic is built on this constaint.
	static wtRect imageRect = { 0, 0, wtSystem::ScreenWidth, wtSystem::ScreenHeight };

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

	if( vramAccessed )
	{
		IncRenderAddr();
		vramAccessed = false;
	}

	// Scanline Work
	// Scanlines take multiple cycles
	if ( currentScanline == POSTRENDER_SCANLINE )
	{
		execCycles		+= ppuCycle_t( 1 );
		scanelineCycle	+= ppuCycle_t( 1 );

		if ( scanelineCycle.count() >= ScanlineCycles )
		{
			++currentScanline;
		}

		return execCycles;
	}
	else if ( currentScanline == PRERENDER_SCANLINE )
	{
		if ( cycleCount == 1 )
		{
			regStatus.current.sem.spriteHit = false;

			inVBlank = false;
			regStatus.current.sem.vBlank = 0;
			regStatus.latched.raw = 0;
			regStatus.hasLatch = false;
		}
		else if( cycleCount >= 280 && cycleCount <= 304 )
		{
			if ( RenderEnabled() )
			{
				regV.sem.fineY = regT.sem.fineY;
				regV.sem.coarseY = regT.sem.coarseY;
				regV.sem.ntId = ( regV.sem.ntId & 0x1 ) | ( regT.sem.ntId & 0x2 );
			}
		}
	}
	else if ( currentScanline == 241 )
	{
		// must only exec once!
		if( cycleCount == 1 )
		{
			inVBlank = true;
			regStatus.current.sem.vBlank = 1;

			const bool isVblank = static_cast<bool>( regCtrl.sem.nmiVblank );
			if ( isVblank )
			{
				GenerateNMI();
			}
		}
	}

	if ( cycleCount == 0 )
	{
		// Idle cycle
		execCycles++;
		scanelineCycle++;

		sprite0InList = false;
		loadingSecondaryOAM = true;
		LoadSecondaryOAM();
//		OAMDATA( 0xFF );
	}
	else if ( cycleCount <= 256 )
	{
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );

		if( currentScanline < POSTRENDER_SCANLINE )
		{
			wtPoint screenPt;
			screenPt.x = beamPosition.x & 0xF8;
			screenPt.y = beamPosition.y & 0xF8;

			// Scanline render
			imageRect.x = beamPosition.x;
			imageRect.y = beamPosition.y;

			// TODO: don't use globals directly in this function
			const uint32_t imageX = imageRect.x;
			const uint32_t imageY = imageRect.y;
			const uint32_t imageIx = imageX + imageY * imageRect.width;

			uint8_t bgPixel = 0;

			if ( !regMask.sem.showBg )
			{
				Pixel pixelColor;

				const uint8_t colorIx = ReadVram( PPU::PaletteBaseAddr );

				Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

				system->frameBuffer.Set( imageIx, pixelColor );
			}
			else
			{
				bgPixel = BgPipelineDecodePalette();

				uint16_t finalPalette = bgPixel + PPU::PaletteBaseAddr;

				const uint8_t colorIx = ( ( bgPixel & 0x03 ) == 0 ) ? ReadVram( PPU::PaletteBaseAddr ) : ReadVram( finalPalette );

				// Frame Buffer
				Pixel pixelColor;
				Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

				system->frameBuffer.Set( imageIx, pixelColor );
			}

			for ( uint8_t spriteNum = 0; spriteNum < spriteLimit; ++spriteNum )
			{
				PpuSpriteAttrib attribs = GetSpriteData( spriteNum, secondaryOAM );

				if ( ( beamPosition.x < ( attribs.x + 8 ) ) && ( beamPosition.x >= attribs.x ) )
				{
					if ( !( attribs.y < 8 || attribs.y > 232 ) )
					{
						DrawSpritePixel( system->frameBuffer, imageRect, attribs, beamPosition, bgPixel & 0x03, ( spriteNum == 0 ) && sprite0InList );
					}
				}
			}
		}

		BgPipelineShiftRegisters();
		BgPipelineFetch( cycleCount );

		beamPosition.x++;
	}
	else if ( cycleCount == 257 )
	{
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );

		BgPipelineFetch( cycleCount );

		AdvanceYScroll( cycleCount );

		if ( RenderEnabled() )
		{
			regV.sem.coarseX = regT.sem.coarseX;
			regV.sem.ntId = ( regV.sem.ntId & 0x2 ) | ( regT.sem.ntId & 0x1 );
		}
	}
	else if ( cycleCount <= 320 ) // [258 - 320]
	{
		// Prefetch the 8 sprites on next scanline
		execCycles += ppuCycle_t( 1 );
		scanelineCycle += ppuCycle_t( 1 );

	//	BgPipelineFetch( cycleCount ); // Garbage fetches, 8 fetches
	}
	else if ( cycleCount <= 336 ) // [321 - 336]
	{
		execCycles++;
		scanelineCycle++;

		BgPipelineShiftRegisters();

		BgPipelineFetch( cycleCount ); // Prefetch first two tiles on next scanline
		BgPipelineDebugPrefetchFetchTiles();
	}
	else if ( cycleCount < 340 ) // [337 - 340], +4 cycles
	{
		BgPipelineFetch( cycleCount );

		// 2 unused fetches
		execCycles++;
		scanelineCycle++;
	}
	else
	{
		if( currentScanline == PRERENDER_SCANLINE )
		{
			currentScanline = 0;

			beamPosition.x = 0;
			beamPosition.y = 0;
		}
		else
		{
			currentScanline++;
			beamPosition.x = 0;
			beamPosition.y++;
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
