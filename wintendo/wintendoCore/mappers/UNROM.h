#pragma once

#include "common.h"
#include "NesSystem.h"

class UNROM : public wtMapper
{
private:
	uint8_t bank;
	uint8_t lastBank;
public:
	UNROM( const uint32_t _mapperId )
	{
		mapperId = _mapperId;
		bank = 0;
		lastBank = 0;
	}

	uint8_t OnLoadCpu() override
	{
		bank = 0;
		lastBank = ( system->cart.header.prgRomBanks - 1 );
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
		if ( InRange( addr, wtSystem::Bank0, wtSystem::Bank0End ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank0 );
			return system->cart.GetPrgRomBank( bank )[ bankAddr ];
		}
		else if ( InRange( addr, wtSystem::Bank1, wtSystem::Bank1End ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank1 );
			return system->cart.GetPrgRomBank( lastBank )[ bankAddr ];
		}
		assert( 0 );
		return 0;
	}

	uint8_t Write( const uint16_t addr, const uint16_t offset, const uint8_t value ) override
	{
		bank = ( value & 0x07 );
		return 0;
	};

	bool InWriteWindow( const uint16_t addr, const uint16_t offset ) override
	{
		const uint16_t address = ( addr + offset );
		return ( system->cart.GetMapperId() == 2 ) && InRange( address, 0x8000, 0xFFFF );
	}

	void Serialize( Serializer& serializer, const serializeMode_t mode ) override
	{
		serializer.Next8b( bank,		mode );
		serializer.Next8b( lastBank,	mode );
	}
};