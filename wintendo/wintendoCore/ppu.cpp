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

	scrollPosition[scrollReg] = value;
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

	if ( addr >= 0x3000 && addr < 0x3F00 )
	{
		addr -= 0x1000;
	}

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

	return addr;
}


uint8_t PPU::ReadMem( const uint16_t addr )
{
	return vram[MirrorVramAddr( addr )];
}


uint8_t PPU::GetNtTileForPoint( const uint32_t ntId, WtPoint point )
{
	WtPoint scrollPoint;
	uint32_t newNtId = ntId;

	point.x += scrollPosition[0];
	point.y += scrollPosition[1];
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
	
	return ReadMem( ntBaseAddr + tileCoord.x + tileCoord.y * NameTableWidthTiles );
}


uint8_t PPU::GetArribute( const uint32_t ntId, const WtPoint& tileCoord )
{
	const uint32_t ntBaseAddr = ( NameTable0BaseAddr + ntId * NameTableAttribMemorySize );

	const uint8_t attribXoffset = (uint8_t)( tileCoord.x / NtTilesPerAttribute );
	const uint8_t attribYoffset = (uint8_t)( tileCoord.y / NtTilesPerAttribute );

	return ReadMem( ntBaseAddr + NametableMemorySize + attribXoffset + attribYoffset * AttribTableWidthTiles );
}


uint8_t PPU::GetTilePaletteId( const uint32_t ntId, const WtPoint& tileCoord )
{
	const uint8_t attribTable = GetArribute( ntId, tileCoord );

	const uint8_t attribSectionRow = (uint8_t)floor( ( tileCoord.x % NtTilesPerAttribute ) / 2.0f ); // 2 is to convert from nametable to attrib tile
	const uint8_t attribSectionCol = (uint8_t)floor( ( tileCoord.y % NtTilesPerAttribute ) / 2.0f );
	const uint8_t attribShift = 2 * ( attribSectionRow + 2 * attribSectionCol );

	return 4 * ( ( attribTable >> attribShift ) & 0x03 );
}


void PPU::WriteVram()
{
	if ( vramWritePending )
	{
		//std::cout << "Writing: " << std::hex << vramAddr << " " << std::hex << (uint32_t) registers[PPUREG_DATA] << std::endl;
		vram[MirrorVramAddr( vramAddr )] = registers[PPUREG_DATA];
		vramAddr += regCtrl.sem.vramInc ? 32 : 1;

		vramWritePending = false;
	}
}


void PPU::FrameBufferWritePixel( const uint32_t x, const uint32_t y, const Pixel pixel )
{
	system->frameBuffer[x + y * NesSystem::ScreenWidth] = pixel.raw;
}


void PPU::DrawTile( uint32_t imageBuffer[], const WtRect& imageRect, const WtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId, bool scroll )
{
	const uint8_t tileBytes = 16;

	const uint16_t baseAddr = system->cart->header.prgRomBanks * NesSystem::BankSize + ptrnTableId * NesSystem::ChrRomSize;

	for ( int y = 0; y < PPU::NameTableTilePixels; ++y )
	{
		for ( int x = 0; x < PPU::NameTableTilePixels; ++x )
		{
			WtPoint point;

			point.x = 8 * nametableTile.x + x;
			point.y = 8 * nametableTile.y + y;

			// FIXME: Needs to be per row due to scrolls, split this off into new function and keep this for NT debug
			const uint32_t tileIx = scroll ? GetNtTileForPoint( ntId, point ) : GetNtTile( ntId, nametableTile );

			const uint16_t chrRomBase = baseAddr + tileIx * tileBytes;

			const uint8_t paleteId = GetTilePaletteId( ntId, nametableTile ); // TODO: Have Attrib struct and a single function for attribs

			const uint8_t scrollOffsetX = scrollPosition[0] & 0x07;
			const uint8_t scrollOffsetY = scrollPosition[1] & 0x07;

			const uint8_t chrRomCol = scroll ? ( x + scrollOffsetX ) % PPU::NameTableTilePixels : x;
			const uint8_t chrRomRow = scroll ? ( y + scrollOffsetY ) % PPU::NameTableTilePixels : y;

			const uint8_t chrRom0 = system->cart->rom[chrRomBase + chrRomRow];
			const uint8_t chrRom1 = system->cart->rom[chrRomBase + ( tileBytes / 2 ) + chrRomRow];

			const uint16_t xBitMask = ( 0x80 >> chrRomCol );

			// TODO: Test x scroll
			const uint8_t lowerBits = ( ( chrRom0 & xBitMask ) >> ( 7 - chrRomCol ) ) | ( ( ( chrRom1 & xBitMask ) >> ( 7 - chrRomCol ) ) << 1 );

			const uint32_t imageX = imageRect.x + x;
			const uint32_t imageY = imageRect.y + y;

			const uint8_t colorIx = vram[PPU::PaletteBaseAddr + paleteId + lowerBits];

			Pixel pixelColor;

			Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );

			imageBuffer[imageX + imageY * imageRect.width] = pixelColor.raw;
		}
	}
}


void PPU::DrawSprite( const uint32_t tableId )
{
	const uint8_t tileBytes = 16;

	const uint16_t totalSprites = 64;
	const uint16_t spritePaletteAddr = 0x3F10;

	for ( uint16_t spriteNum = 0; spriteNum < totalSprites; ++spriteNum )
	{
		const uint8_t spriteY = primaryOAM[spriteNum * 4];
		const uint8_t tileId = primaryOAM[spriteNum * 4 + 1]; // TODO: implement 16x8
		const uint8_t attrib = primaryOAM[spriteNum * 4 + 2]; // TODO: finish attrib features
		const uint8_t spriteX = primaryOAM[spriteNum * 4 + 3];

		const uint8_t attribPal = 4 * ( attrib & 0x3 );
		const uint8_t priority = ( attrib & 0x20 ) >> 5;
		const uint8_t flippedHor = ( attrib & 0x40 ) >> 6;
		const uint8_t flippedVert = ( attrib & 0x80 ) >> 7;

		const uint16_t baseAddr = system->cart->header.prgRomBanks * 0x4000 + tableId * 0x1000;
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

				const uint8_t chrRom0 = system->cart->rom[chrRomBase + y];
				const uint8_t chrRom1 = system->cart->rom[chrRomBase + ( tileBytes / 2 ) + y];

				const uint16_t xBitMask = ( 0x80 >> x );

				const uint8_t lowerBits = ( ( chrRom0 & xBitMask ) >> ( 7 - x ) ) | ( ( ( chrRom1 & xBitMask ) >> ( 7 - x ) ) << 1 );

				if ( lowerBits == 0x00 )
					continue;

				//if( spriteZeroHit )
				{
					system->ppu.regStatus.current.sem.spriteHit = true;
				}

				const uint8_t colorIx = vram[spritePaletteAddr + attribPal + lowerBits];

				Pixel pixelColor;

				Bitmap::CopyToPixel( palette[colorIx], pixelColor, BITMAP_BGRA );
				FrameBufferWritePixel( spriteX + tileCol, spriteY + tileRow, pixelColor );
			}
		}
	}
}


const ppuCycle_t PPU::Exec()
{
	ppuCycle_t cycles = ppuCycle_t( 0 );

	// Cycle timing
	const uint64_t cycleCount = scanelineCycle.count();

	if ( regStatus.hasLatch )
	{
		regStatus.current = regStatus.latched;
		regStatus.latched.raw = 0;
		regStatus.hasLatch = false;
	}

	WriteVram();

	// Scanline Work
	// Scanlines take multiple cycles
	if ( cycleCount == 0 )
	{
		if ( currentScanline < 0 )
		{
			regStatus.current.sem.vBlank = 0;
			regStatus.latched.raw = 0;
			regStatus.hasLatch = false;
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
		cycles++;
		scanelineCycle++;
	}
	else if ( cycleCount <= 256 )
	{
		// Scanline render
		cycles += ppuCycle_t( 8 );
		scanelineCycle += ppuCycle_t( 8 );
	}
	else if ( cycleCount <= 320 )
	{
		// Prefetch the 8 sprites on next scanline
		cycles += ppuCycle_t( 8 );
		scanelineCycle += ppuCycle_t( 8 );
	}
	else if ( cycleCount <= 336 )
	{
		// Prefetch first two tiles on next scanline
		cycles++;
		scanelineCycle++;
	}
	else if ( cycleCount <= 340 )
	{
		// Unused fetch
		cycles += ppuCycle_t( 8 );
		scanelineCycle += ppuCycle_t( 8 );
	}
	else
	{
		if ( currentScanline >= 260 )
		{
			currentScanline = -1;
		}
		else
		{
			++currentScanline;
		}

		scanelineCycle = scanelineCycle.zero();
	}

	return cycles;
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
