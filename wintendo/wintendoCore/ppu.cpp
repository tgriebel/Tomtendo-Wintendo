#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <bitset>
#include "common.h"
#include "debug.h"
#include "mos6502.h"
#include "NesSystem.h"


void PPU::WriteReg( const uint16_t addr, const uint8_t value )
{
	const uint16_t regNum = ( addr & 0x000F ); // Mirror: 2008-4000
	switch ( regNum )
	{
		case 0: PPUCTRL( value );	break;
		case 1: PPUMASK( value );	break;
		case 3:	OAMADDR( value );	break;
		case 4:	OAMDATA( value );	break;
		case 5:	PPUSCROLL( value );	break;
		case 6: PPUADDR( value );	break;
		case 7:	PPUDATA( value );	break;
		default: break;
	}
}


uint8_t PPU::ReadReg( uint16_t addr )
{
	const uint16_t regNum = ( addr & 0x000F ); // Mirror: 0x2008-0x4000
	switch ( regNum )
	{
		case 2:	return PPUSTATUS();
		case 7:	return PPUDATA();
		default: break;
	}
	return 0;
}


void PPU::DMA( const uint16_t address )
{
	for( uint32_t i = 0; i < wtSystem::PageSize; ++i )
	{
		primaryOAM[i] = system->ReadMemory( i + wtSystem::PageSize * static_cast<uint8_t>( address ) );
	}
}


void PPU::PPUCTRL( const uint8_t value )
{
	regCtrl.byte = value;
	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	regT.sem.ntId = static_cast<uint8_t>( regCtrl.sem.ntId );
}


void PPU::PPUMASK( const uint8_t value )
{
	regMask.byte = value;
	regStatus.current.sem.lastReadLsb = ( value & 0x1F );
}


uint8_t PPU::PPUSTATUS()
{
	regStatus.latched = regStatus.current;
	regStatus.latched.sem.vBlank = 0;
	//regStatus.latched.sem.lastReadLsb = 0;
	regStatus.hasLatch = true;
	registers[PPUREG_ADDR] = 0;
	registers[PPUREG_DATA] = 0;

	regW = 0;

	return regStatus.current.byte;
}


void PPU::OAMADDR( const uint8_t value )
{
	assert( value == 0x00 ); // TODO: implement other modes

	registers[PPUREG_OAMADDR] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

}


void PPU::OAMDATA( const uint8_t value )
{
	assert( 0 );

	registers[PPUREG_OAMDATA] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );
}


void PPU::PPUSCROLL( const uint8_t value )
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
}


void PPU::PPUADDR( const uint8_t value )
{
	registers[PPUREG_ADDR] = value;

	regStatus.current.sem.lastReadLsb = ( value & 0x1F );

	if( regW == 0 )
	{
		regT.byte2x = ( regT.byte2x & 0x00FF) | ( ( value & 0x3F ) << 8 );
	}
	else
	{
		regT.byte2x = ( regT.byte2x & 0xFF00 ) | value;
		regV = regT;
	}

	regW ^= 1;
}


void PPU::PPUDATA( const uint8_t value )
{
	if( DataportEnabled() )
	{
		registers[PPUREG_DATA] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		vramAccessed = true;
		vramWritePending = true;
	}
}


uint8_t PPU::PPUDATA()
{
	uint8_t value = 0;
	if ( regV.byte2x < 0x3EFF ) {
		value = ppuReadBuffer;
	} else if ( regV.byte2x >= 0x3F00 ) {
		value = ReadVram( regV.byte2x );
	}

	// TODO: when reading palettes, store mirrored NT data
	// https://wiki.nesdev.com/w/index.php/PPU_programmer_reference#The_PPUDATA_read_buffer_.28post-fetch.29
	ppuReadBuffer = ReadVram( regV.byte2x );
	vramAccessed = true;

	return value;
}


void PPU::Begin()
{

}


void PPU::End()
{

}


void PPU::RegisterSystem( wtSystem* sys )
{
	system = sys;
}


void PPU::IssueDMA( const uint8_t value )
{
	system->RequestDMA();
	DMA( value );
}


void PPU::IncRenderAddr()
{
	if( DataportEnabled() )
	{
		// This is synonymous with a coarseX or coarseY increment
		const bool isLargeIncrement = static_cast<bool>( regCtrl.sem.vramInc );
		regV.byte2x += isLargeIncrement ? 0x20 : 0x01;
	}
}


FORCE_INLINE void PPU::BgPipelineFetch( const uint64_t cycleCountAdjust )
{
	// 1. Name table byte
	if ( cycleCountAdjust == 2 )
	{
		const uint16_t address = NameTable0BaseAddr | ( regV.byte2x & 0x0FFF );
		plLatches.tileId = ReadVram( address );
	}
	// 2. Attribute table byte
	else if ( cycleCountAdjust == 4 )
	{
		wtPoint coarsePt;
		coarsePt.x = regV.sem.coarseX;
		coarsePt.y = regV.sem.coarseY;

		const uint16_t address = AttribTable0BaseAddr | ( regV.sem.ntId << 10 ) | ( ( regV.sem.coarseY << 1 ) & 0xF8 ) | ( ( regV.sem.coarseX >> 2 ) & 0x07 );
		const uint32_t attribute = ReadVram( address );

		assert( address >= 0x2000 );

		plLatches.attribId = GetTilePaletteId( attribute, coarsePt );
	}
	// 3. Pattern table tile #0
	else if ( cycleCountAdjust == 6 )
	{
		plLatches.chrRom0 = GetChrRom8x8( plLatches.tileId, 0, GetBgPatternTableId(), regV.sem.fineY );
	}
	// 4. Pattern table tile #1
	else if( cycleCountAdjust == 0 )
	{
		plLatches.chrRom1 = GetChrRom8x8( plLatches.tileId, 1, GetBgPatternTableId(), regV.sem.fineY );
		//---------------------
		// Finish pipeline work
		//---------------------
		plLatches.flags = 0x1;
		AdvanceXScroll();

		plShifts[curShift] = plLatches;

		chrShifts[0] = ( chrShifts[0] & 0xFF00 ) | plLatches.chrRom0;
		chrShifts[1] = ( chrShifts[1] & 0xFF00 ) | plLatches.chrRom1;

		palLatch[0] = ( plShifts[curShift].attribId >> 2 ) & 0x01;
		palLatch[1] = ( ( plShifts[curShift].attribId >> 2 ) & 0x2 ) >> 1;

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


uint16_t PPU::StaticMirrorVram( uint16_t addr, uint32_t mirrorMode )
{
	// Major mirror
	addr %= 0x4000;

	// Nametable Mirrors
	if ( addr >= 0x3000 && addr < 0x3F00 )
	{
		addr -= 0x1000;
	}

	// Nametable Mirroring Modes
	if ( mirrorMode == MIRROR_MODE_HORIZONTAL )
	{
		if ( addr >= 0x2400 && addr < 0x2800 ) {
			return ( addr - 0x2400 + PPU::NameTable0BaseAddr );
		} else if ( addr >= 0x2800 && addr < 0x2C00 ) {
			return ( addr - 0x2800 + PPU::NameTable1BaseAddr );
		} else if ( addr >= 0x2C00 && addr < 0x3000 ) {
			return ( addr - 0x2C00 + PPU::NameTable1BaseAddr );
		}
	}
	else if ( mirrorMode == MIRROR_MODE_VERTICAL )
	{
		if ( addr >= 0x2400 && addr < 0x2800 ) {
			return ( addr - 0x2400 + PPU::NameTable1BaseAddr );
		} else if ( addr >= 0x2800 && addr < 0x2C00 ) {
			return ( addr - 0x2800 + PPU::NameTable0BaseAddr );
		} else if ( addr >= 0x2C00 && addr < 0x3000 ) {
			return ( addr - 0x2C00 + PPU::NameTable1BaseAddr );
		}
	}
	else if ( mirrorMode == MIRROR_MODE_FOURSCREEN )
	{
		if ( addr >= PPU::NameTable0BaseAddr && addr < 0x3000 )
		{
			return PPU::NameTable0BaseAddr + ( addr % PPU::NameTableAttribMemorySize );
		}
	}

	// Palette Mirrors
	if ( addr >= 0x3F20 && addr < 0x4000 )
	{
		addr = PPU::PaletteBaseAddr + ( ( addr & 0x00FF ) % 0x20 );
	}

	if ( ( addr == 0x3F10 ) || ( addr == 0x3F14 ) || ( addr == 0x3F18 ) || ( addr == 0x3F1C ) )
	{
		return ( addr - 0x0010 );
	}

	return addr;
}


void PPU::GenerateMirrorMap()
{
#if MIRROR_OPTIMIZATION
	for ( uint16_t mode = 0; mode < MIRROR_MODE_COUNT; ++mode )
	{
		for ( uint32_t addr = 0; addr < PPU::VirtualMemorySize; ++addr )
		{
			MirrorMap[mode][addr] = StaticMirrorVram( addr, mode );
		}
	}
#endif
}

FORCE_INLINE uint16_t PPU::MirrorVram( uint16_t addr )
{
	uint32_t mirrorMode = system->GetMirrorMode();

#if MIRROR_OPTIMIZATION
	return MirrorMap[mirrorMode][addr];
#else
	return StaticMirrorVram( addr, mirrorMode );
#endif
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
	const uint16_t chrRomAddr= ( ptrnTableId << 12 ) | ( tileId << 4 ) | ( plane << 3 ) | row;
	return ReadVram( chrRomAddr );
}


uint8_t PPU::GetChrRom8x16( const uint32_t tileId, const uint8_t plane, const uint8_t row, const uint8_t isUpper )
{
	const uint16_t baseAddr		= ( ( tileId & 0x01 ) << 12 ) | ( ( ( tileId & ~0x01 ) | isUpper ) << 4 );
	const uint16_t chrRomAddr	= baseAddr | ( plane << 3 ) | row;

	return ReadVram( chrRomAddr );
}


uint8_t PPU::GetChrRomBank8x8( const uint32_t tileId, const uint8_t plane, const uint8_t bankId, const uint8_t row )
{
	const uint8_t tileBytes = 16;
	const uint16_t chrRomBase = tileId * tileBytes;
	const uint16_t bankAddr = bankId * wtSystem::ChrRomSize;
	const uint8_t* bank = system->cart->GetChrRomBank( bankId );
	return bank[ chrRomBase + row + 8 * ( plane & 0x01 ) ];
}


uint8_t PPU::GetChrRomPalette( const uint8_t plane0, const uint8_t plane1, const uint8_t col )
{
	const uint8_t planeBit0 = ( plane0 >> ( 7 - col ) ) & 0x01;
	const uint8_t planeBit1 = ( ( plane1 >> ( 7 - col ) ) & 0x01 ) << 1;

	return ( planeBit0 | planeBit1 );
}


bool PPU::IsMemoryMapped( const uint16_t addr ) const
{
	return false;
}


FORCE_INLINE uint8_t PPU::ReadVram( const uint16_t addr )
{
	const uint16_t adjustedAddr = MirrorVram( addr );
	assert( adjustedAddr < VirtualMemorySize );

	if( InRange( adjustedAddr, 0x0000, 0x1FFF ) ) {
		return system->cart->mapper->ReadChrRom( adjustedAddr );
	} else if ( InRange( adjustedAddr, 0x2000, 0x3EFF ) ) {
		return nt[ adjustedAddr - 0x2000 ];
	} else if ( InRange( adjustedAddr, 0x3F00, 0x3F0F ) ) {
		return imgPal[ adjustedAddr - 0x3F00 ];
	} else if ( InRange( adjustedAddr, 0x3F10, 0x3F1F ) ) {
		return sprPal[ adjustedAddr - 0x3F10 ];
	} else {
		assert( 0 );
		return 0;
	}
}


void PPU::WriteVram()
{
	if ( vramWritePending && DataportEnabled()  )
	{
		const uint16_t adjustedAddr = MirrorVram( regV.byte2x );
		// assert( adjustedAddr < PhysicalMemorySize );

		if ( InRange( adjustedAddr, 0x0000, 0x1FFF ) ) {
			system->cart->mapper->WriteChrRam( adjustedAddr, registers[ PPUREG_DATA ] );
		} else if ( InRange( adjustedAddr, 0x2000, 0x3EFF ) ) {
			nt[ adjustedAddr - 0x2000 ] = registers[ PPUREG_DATA ];
		} else if ( InRange( adjustedAddr, 0x3F00, 0x3F0F ) ) {
			imgPal[ adjustedAddr - 0x3F00 ] = registers[ PPUREG_DATA ];
		} else if ( InRange( adjustedAddr, 0x3F10, 0x3F1F ) ) {
			sprPal[ adjustedAddr - 0x3F10 ] = registers[ PPUREG_DATA ];
		} else {
			assert( 0 );
		}

		debugVramWriteCounter[adjustedAddr]++;
	}

	vramWritePending = false;
}


void PPU::DrawBlankScanline( wtDisplayImage& imageBuffer, const wtRect& imageRect, const uint8_t scanY )
{
	for ( int x = 0; x < ScreenWidth; ++x )
	{
		Pixel pixelColor;
		pixelColor.vec[0] = 0;
		pixelColor.vec[1] = 0;
		pixelColor.vec[2] = 0;
		pixelColor.vec[3] = 0xFF;

		const uint32_t imageX = imageRect.x + x;
		const uint32_t imageY = imageRect.y + scanY;
		const uint32_t pixelIx = imageX + imageY * imageRect.width;

		imageBuffer.Set( pixelIx, pixelColor );
	}
}


uint8_t PPU::BgPipelineDecodePalette()
{
	uint8_t chrRomColor;

	const uint8_t chrRom0 = static_cast<uint8_t>( chrShifts[0] >> 8 );
	const uint8_t chrRom1 = static_cast<uint8_t>( chrShifts[1] >> 8 );

	uint8_t paletteId = GetChrRomPalette( palShifts[0], palShifts[1], static_cast< uint8_t >( regX ) );
	paletteId <<= 2;

	chrRomColor = GetChrRomPalette( chrRom0, chrRom1, static_cast< uint8_t >( regX ) );

	return ( paletteId | chrRomColor );
}


void PPU::DrawTile( wtNameTableImage& imageBuffer, const wtRect& imageRect, const wtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId )
{
	for ( uint32_t y = 0; y < PPU::TilePixels; ++y )
	{
		for ( uint32_t x = 0; x < PPU::TilePixels; ++x )
		{
			wtPoint chrRomPoint;

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
			pixelColor.rgba = palette[colorIx];
			imageBuffer.Set( imageX + imageY * imageRect.width, pixelColor );
		}
	}
}


void PPU::DrawChrRomTile( wtRawImageInterface* imageBuffer, const wtRect& imageRect, const RGBA dbgPalette[4], const uint32_t tileId, const uint32_t tableId, const bool cartBank, const bool is8x16, const bool isUpper )
{
	// TODO: fork into two functions--one for cart banks, the other for mapped banks
	for ( uint32_t y = 0; y < PPU::TilePixels; ++y )
	{
		for ( uint32_t x = 0; x < PPU::TilePixels; ++x )
		{
			wtPoint chrRomPoint;

			chrRomPoint.x = x;
			chrRomPoint.y = y;

			uint8_t chrRom0;
			uint8_t chrRom1;

			if( cartBank )
			{
				assert( !is8x16 );
				chrRom0 = GetChrRomBank8x8( tileId, 0, tableId, chrRomPoint.y );
				chrRom1 = GetChrRomBank8x8( tileId, 1, tableId, chrRomPoint.y );
			}
			else
			{
				if ( is8x16 )
				{
					chrRom0 = GetChrRom8x16( tileId, 0, chrRomPoint.y, isUpper );
					chrRom1 = GetChrRom8x16( tileId, 1, chrRomPoint.y, isUpper );
				}
				else
				{
					chrRom0 = GetChrRom8x8( tileId, 0, tableId, chrRomPoint.y );
					chrRom1 = GetChrRom8x8( tileId, 1, tableId, chrRomPoint.y );
				}
			}

			const uint16_t chrRomColor = GetChrRomPalette( chrRom0, chrRom1, chrRomPoint.x );

			const uint32_t imageX = imageRect.x + x;
			const uint32_t imageY = imageRect.y + y;

			Pixel pixelColor;
			pixelColor.rgba = dbgPalette[chrRomColor];
			imageBuffer->Set( imageX + imageY * imageRect.width, pixelColor );
		}
	}
}


spriteAttrib_t PPU::GetSpriteData( const uint8_t spriteId, const uint8_t oam[] )
{
	spriteAttrib_t attribs;
	const uint8_t attrib		= oam[spriteId * 4 + 2]; // TODO: finish attrib features
	attribs.tileId				= oam[spriteId * 4 + 1];
	attribs.x					= oam[spriteId * 4 + 3];
	attribs.y					= 1 + oam[spriteId * 4];
	attribs.palette				= ( attrib & 0x03 ) << 2;
	attribs.priority			= ( attrib & 0x20 ) >> 5;
	attribs.flippedHorizontal	= ( attrib & 0x40 ) >> 6;
	attribs.flippedVertical		= ( attrib & 0x80 ) >> 7;
	attribs.sprite0				= ( spriteId == 0 );
	attribs.oamIndex			= spriteId;

	return attribs;
}


bool PPU::DrawSpritePixel( wtDisplayImage& fb, const spriteAttrib_t attribs, const ppuImageIx_t& beam, const uint8_t bgPixel )
{
	Pixel pixelColor;

	wtPoint spritePt;

	spritePt.x = beam.point.x - attribs.x;
	spritePt.y = beam.point.y - attribs.y;
	spritePt.x = attribs.flippedHorizontal	? ( 7 - spritePt.x ) : spritePt.x;

	uint8_t chrRom0 = 0;
	uint8_t chrRom1 = 0; 

	if ( regCtrl.sem.sprite8x16Mode )
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

		chrRom0 = GetChrRom8x8( attribs.tileId, 0, GetSpritePatternTableId(), spritePt.y );
		chrRom1 = GetChrRom8x8( attribs.tileId, 1, GetSpritePatternTableId(), spritePt.y );
	}

	const uint8_t finalPalette = GetChrRomPalette( chrRom0, chrRom1, spritePt.x );

	const uint8_t colorIx = ReadVram( SpritePaletteAddr + attribs.palette + finalPalette );

	// Sprite 0 Hit happens regardless of priority but still draws normal
	if ( attribs.sprite0 )
	{
		if ( ( bgPixel != 0 ) && ( finalPalette != 0 ) && ( regMask.sem.showBg ) )
		{
			regStatus.current.sem.spriteHit = true;
		}
	}

	if ( finalPalette == 0x00 ) {
		return false;
	}

	if ( ( attribs.priority == 1 ) && ( bgPixel != 0 ) ) {
		return true;
	}

	if( !system->GetConfig()->ppu.showSprite ) {
		return true;
	}

	pixelColor.rgba = palette[colorIx];

	uint8_t spriteHeight = regCtrl.sem.sprite8x16Mode ? 16 : 8;
	if ( system->MouseInRegion( { attribs.x, attribs.y, attribs.x + 8, attribs.y + spriteHeight } ) )
	{
		pixelColor.rawABGR = ~pixelColor.rawABGR;
		pixelColor.rgba.alpha = 0xFF;
		
		dbgInfo.spritePicked = attribs;
		fb.Set( beam.index, pixelColor );
	}
	else
	{
		fb.Set( beam.index, pixelColor );
	}

	return true;
}


void PPU::DrawDebugPatternTables( wtPatternTableImage& imageBuffer, const RGBA dbgPalette[4], const uint32_t tableID, const bool isCartbank )
{
	for ( int32_t tileY = 0; tileY < 16; ++tileY ) {
		for ( int32_t tileX = 0; tileX < 16; ++tileX ) {
			DrawChrRomTile( &imageBuffer, wtRect{ (int32_t)PPU::TilePixels * tileX, (int32_t)PPU::TilePixels * tileY, PPU::PatternTableWidth, PPU::PatternTableHeight }, dbgPalette, (int32_t)( tileX + 16 * tileY ), tableID, isCartbank );
		}
	}
}


void PPU::DrawDebugObject( wtRawImageInterface* imageBuffer, const RGBA dbgPalette[ 4 ], const spriteAttrib_t& attrib )
{
	imageBuffer->Clear();
	const int32_t tileHeight = attrib.is8x16 ? 2 : 1;
	DrawChrRomTile( imageBuffer, wtRect{ 0, 0, (int32_t)PPU::TilePixels, PPU::TilePixels }, dbgPalette, attrib.tileId, attrib.tableId, false, attrib.is8x16, false );
	if( attrib.is8x16 ) {
		DrawChrRomTile( imageBuffer, wtRect{ 0, (int32_t)PPU::TilePixels, (int32_t)PPU::TilePixels, PPU::TilePixels }, dbgPalette, attrib.tileId, attrib.tableId, false, attrib.is8x16, true );
	}
}


void PPU::DrawDebugNametable( wtNameTableImage& imageBuffer )
{
	wtRect ntRects[4] = {		{ 0,			0,					2 * ScreenWidth,	2 * ScreenHeight },
								{ ScreenWidth,	0,					2 * ScreenWidth,	2 * ScreenHeight },
								{ 0,			ScreenHeight,		2 * ScreenWidth,	2 * ScreenHeight },
								{ ScreenWidth,	ScreenHeight,		2 * ScreenWidth,	2 * ScreenHeight }, };

	wtPoint ntCorners[4] = {	{ 0,			0 },
								{ ScreenWidth,	0 },
								{ 0,			ScreenHeight },
								{ ScreenWidth,	ScreenHeight }, };

	for ( uint32_t ntId = 0; ntId < 4; ++ntId )
	{
		for ( int32_t tileY = 0; tileY < (int)PPU::NameTableHeightTiles; ++tileY )
		{
			for ( int32_t tileX = 0; tileX < (int)PPU::NameTableWidthTiles; ++tileX )
			{
				DrawTile( imageBuffer, ntRects[ntId], wtPoint{ tileX, tileY }, ntId, GetBgPatternTableId() );

				ntRects[ntId].x += static_cast<uint32_t>( PPU::TilePixels );
			}

			ntRects[ntId].x = ntCorners[ntId].x;
			ntRects[ntId].y += static_cast<uint32_t>( PPU::TilePixels );
		}
	}
}


void PPU::DrawDebugPalette( wtPaletteImage& imageBuffer )
{
	for ( uint16_t colorIx = 0; colorIx < 16; ++colorIx )
	{
		Pixel pixel;

		uint32_t color = ReadVram( PaletteBaseAddr + colorIx );
		pixel.rgba = palette[color];
		imageBuffer.Set( colorIx, pixel );

		color = ReadVram( SpritePaletteAddr + colorIx );
		pixel.rgba = palette[color];
		imageBuffer.Set( 16 + colorIx, pixel );
	}
}


FORCE_INLINE void PPU::LoadSecondaryOAM()
{
	uint8_t destSpriteNum = 0;
	memset( &secondaryOAM, 0xFF, system->GetConfig()->ppu.spriteLimit );

	for ( uint8_t spriteNum = 0; spriteNum < 64; ++spriteNum )
	{
		const uint8_t y = 1 + primaryOAM[spriteNum * 4];
		const bool isLargeSpriteMode = static_cast<bool>( regCtrl.sem.sprite8x16Mode );
		const uint32_t spriteHeight = isLargeSpriteMode ? 16 : 8;

		if ( ( beam.point.y >= static_cast<int32_t>( y + spriteHeight ) ) || ( beam.point.y < static_cast<int32_t>( y ) ) ) {
			continue;
		}

		if ( ( y < 1 ) || (y > 232 ) ) {
			continue;
		}

		secondaryOAM[destSpriteNum] = GetSpriteData( spriteNum, primaryOAM );
		secondaryOAM[ destSpriteNum ].secondaryOamIndex = destSpriteNum;
		secondaryOAM[ destSpriteNum ].is8x16 = isLargeSpriteMode;
		secondaryOAM[ destSpriteNum ].tableId = GetSpritePatternTableId();
		destSpriteNum++;

		if ( destSpriteNum >= system->GetConfig()->ppu.spriteLimit ) {
			regStatus.current.sem.spriteOverflow = true; //  // TODO: accuracy
			break;
		}
	}

	secondaryOamSpriteCnt = destSpriteNum;
}


void PPU::AdvanceXScroll()
{
	if ( !RenderEnabled() )
		return;
	
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


void PPU::AdvanceYScroll()
{
	if ( regV.sem.fineY < 7 )
	{
		regV.byte2x += 0x1000; // regV.sem.fineY + 1 with overflow
	}
	else
	{
		regV.sem.fineY = 0;
		if ( regV.sem.coarseY == 29 ) // > 240 vertical pixel
		{
			regV.sem.coarseY = 0;
			regV.sem.ntId ^= 0x2;
		}
		else
		{
			regV.sem.coarseY++;
		}
	}
}


void PPU::BgPipelineShiftRegisters()
{
	chrShifts[0] <<= 1;
	chrShifts[1] <<= 1;
	palShifts[0] <<= 1;
	palShifts[1] <<= 1;
	palShifts[0] |= palLatch[0] & 0x1;
	palShifts[1] |= palLatch[1] & 0x1;
}


void PPU::BgPipelineDebugPrefetchFetchTiles()
{
#if 0
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
#endif
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
	return regStatus.current.sem.vBlank;
}


bool PPU::DataportEnabled()
{
	return ( InVBlank() || !RenderEnabled() );
}


uint32_t PPU::GetScanline() const
{
	return currentScanline;
}


ppuCycle_t PPU::GetCycle() const
{
	return cycle;
}


void PPU::Render()
{
	const uint32_t imageIx = beam.index;

	uint8_t bgPixel = 0;

	bool bgMask = ( !regMask.sem.bgLeft && ( beam.point.x < 8 ) );
	bgMask = bgMask || !regMask.sem.showBg;
	bgMask = bgMask || !system->GetConfig()->ppu.showBG;

	wtDisplayImage* fb = system->GetBackbuffer();

	if ( bgMask )
	{
		Pixel pixelColor;
		const uint8_t colorIx = ReadVram( PPU::PaletteBaseAddr );

		pixelColor.rgba = palette[ colorIx ];
		fb->Set( imageIx, pixelColor );	
	}
	else
	{
		bgPixel = BgPipelineDecodePalette();

		uint16_t finalPalette = bgPixel + PPU::PaletteBaseAddr;

		const uint8_t colorIx = ( ( bgPixel & 0x03 ) == 0 ) ? ReadVram( PPU::PaletteBaseAddr ) : ReadVram( finalPalette );

		// Frame Buffer
		Pixel pixelColor;
		pixelColor.rgba = palette[ colorIx ];
		fb->Set( imageIx, pixelColor );
	}

	uint8_t spriteCount = secondaryOamSpriteCnt;
	if ( !regMask.sem.sprtLeft && ( beam.point.x < 8 ) ) {
		spriteCount = 0;
	}

	for ( uint8_t spriteIndex = 0; spriteIndex < spriteCount; ++spriteIndex )
	{
		spriteAttrib_t& attribs = secondaryOAM[ spriteIndex ];
		if ( ( beam.point.x >= ( attribs.x + 8 ) ) || ( beam.point.x < attribs.x ) ) {
			continue;
		}

		if ( !regMask.sem.showSprt ) {
			continue;
		}

		if ( DrawSpritePixel( *fb, attribs, beam, bgPixel & 0x03 ) ) {
			break;
		}
	}

	++beam.index;
}


ppuCycle_t PPU::Exec()
{
	ppuCycle_t execCycles = ppuCycle_t( 0 );

	// Cycle timing
	const uint64_t cycleCount = cycle.count() % ScanlineCycles;

	if ( regStatus.hasLatch )
	{
		regStatus.current = regStatus.latched;
		regStatus.latched.byte = 0;
		regStatus.hasLatch = false;
	}

	WriteVram();

	if( vramAccessed )
	{
		IncRenderAddr();
		vramAccessed = false;
	}

	///////////////////////////////////////////////////////////////////////////
	//                                                                       //
	//                            Scanline Work                              //
	//                                                                       //
	///////////////////////////////////////////////////////////////////////////
	// Scanlines take multiple cycles
	if ( currentScanline == 241 )
	{
		// must only exec once!
		if( cycleCount == 1 )
		{
			inVBlank = true;
			regStatus.current.sem.vBlank = 1;

			system->ToggleFrame(); // frame is complete
			beam.index = 0;

			const bool isVblank = static_cast<bool>( regCtrl.sem.nmiVblank );
			if ( isVblank )
			{
				system->RequestNMI();
			}
		}
		
		if ( cycleCount >= 340 ) {
			++currentScanline;
		}

		++execCycles;
		return execCycles;
	}
	if ( ( currentScanline >= POSTRENDER_SCANLINE ) && ( currentScanline < PRERENDER_SCANLINE ) )
	{
		if ( cycleCount >= 340 ) {
			++currentScanline;
		}

		++execCycles;
		return execCycles;
	}
	else if ( currentScanline == PRERENDER_SCANLINE )
	{
		if ( cycleCount == 1 )
		{
			regStatus.current.sem.spriteOverflow = false;
			regStatus.current.sem.spriteHit = false;

			inVBlank = false;
			regStatus.current.sem.vBlank = 0;
			regStatus.latched.byte = 0;
			regStatus.hasLatch = false;

			system->SaveFrameState();
		}
		else if ( cycleCount >= 280 && cycleCount <= 304 )
		{
			if ( RenderEnabled() )
			{
				regV.sem.fineY = regT.sem.fineY;
				regV.sem.coarseY = regT.sem.coarseY;
				regV.sem.ntId = ( regV.sem.ntId & 0x1 ) | ( regT.sem.ntId & 0x2 );
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////
	//                                                                       //
	//                               Cycle Work                              //
	//                                                                       //
	///////////////////////////////////////////////////////////////////////////
	if ( cycleCount == 0 )
	{
		// Idle cycle
		++execCycles;
	}
	else if ( cycleCount <= 256 )
	{
		if ( currentScanline < POSTRENDER_SCANLINE )
		{
			if ( cycleCount == 1 ) {
				LoadSecondaryOAM();
			}

			Render();
				
			BgPipelineShiftRegisters();
			BgPipelineFetch( cycleCount & 0x07 );
		}

		++execCycles;
	}
	else if ( cycleCount == 257 )
	{
		if ( RenderEnabled() )
		{
			AdvanceYScroll(); // technically done on 256

			regV.sem.coarseX = regT.sem.coarseX;
			regV.sem.ntId = ( regV.sem.ntId & 0x2 ) | ( regT.sem.ntId & 0x1 );
		}

		++execCycles;
	}
	else if ( cycleCount <= 259 ) // [258 - 259]
	{
		execCycles += 2;
	}
	else if ( cycleCount == 260 )
	{
		if( RenderEnabled() ) {
			system->cart->mapper->Clock(); // TODO: How big of a hack is this?
		}
		++execCycles;
	}
	else if ( cycleCount <= 320 ) // [261 - 320]
	{
		// Garbage fetches, 8 fetches
		execCycles += 3;
	}
	else if ( cycleCount <= 336 ) // [321 - 336]
	{
		if( RenderEnabled() ) {
			BgPipelineShiftRegisters();
			BgPipelineFetch( cycleCount & 0x07 ); // Prefetch first two tiles on next scanline
			BgPipelineDebugPrefetchFetchTiles();
		}

		++execCycles;
	}
	else if ( cycleCount <= 339 ) // [337 - 339]
	{
		// 2 unused fetches
		++execCycles;
	}
	else if ( cycleCount == 340 )
	{
		currentScanline = ( currentScanline + 1 ) % PRERENDER_SCANLINE;

		++execCycles;
	}

	return execCycles;
}


bool PPU::Step( const ppuCycle_t& nextCycle )
{
	/*
	https://wiki.nesdev.com/w/index.php/NMI
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