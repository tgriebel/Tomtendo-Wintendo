#pragma once

#include "common.h"
#include "NesSystem.h"

class NROM : public wtMapper
{
public:
	NROM( const uint32_t _mapperId )
	{
		mapperId = _mapperId;
	}

	uint8_t OnLoadCpu() override
	{
		return 0;
	};

	uint8_t OnLoadPpu() override
	{
		const uint16_t chrRomStart = system->cart.header.prgRomBanks * KB_16;
		memcpy( system->ppu.vram, &system->cart.rom[chrRomStart], PPU::PatternTableMemorySize );
		return 0;
	};

	uint8_t	ReadRom( const uint16_t addr ) override
	{
		if( InRange( addr, wtSystem::Bank0, 0xFFFF ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank0 );
			return system->cart.rom[ bankAddr ];
		}
		assert( 0 );
		return 0;
	}

	uint8_t Write( const uint16_t addr, const uint16_t offset, const uint8_t value ) override { return 0; };

	bool InWriteWindow( const uint16_t addr, const uint16_t offset ) override
	{
		return false;
	}
};