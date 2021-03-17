#pragma once

#include "common.h"
#include "NesSystem.h"

class UNROM : public wtMapper
{
private:
	uint8_t bank;
	uint8_t lastBank;
	uint8_t	chrRam[ PPU::PatternTableMemorySize ];
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
		lastBank = ( system->cart->h.prgRomBanks - 1 );
		return 0; 
	};

	uint8_t OnLoadPpu() override
	{
		memset( chrRam, 0, sizeof( PPU::PatternTableMemorySize ) );
		return 0;
	};

	uint8_t	ReadRom( const uint16_t addr ) override
	{
		if ( InRange( addr, wtSystem::Bank0, wtSystem::Bank0End ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank0 );
			return system->cart->GetPrgRomBank( bank )[ bankAddr ];
		}
		else if ( InRange( addr, wtSystem::Bank1, wtSystem::Bank1End ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank1 );
			return system->cart->GetPrgRomBank( lastBank )[ bankAddr ];
		}
		// assert( 0 );
		return 0;
	}

	uint8_t	ReadChrRom( const uint16_t addr ) override
	{
		if ( InRange( addr, 0x0000, 0x1FFF ) )	{
			if( system->cart->HasChrRam() ) {
				return chrRam[ addr ];
			} else {
				return system->cart->GetChrRomBank( 0 )[ addr ];
			}
		}
		assert( 0 );
		return 0;
	}

	uint8_t	 WriteChrRam( const uint16_t addr, const uint8_t value ) override
	{
		if ( InRange( addr, 0x0000, 0x1FFF ) && system->cart->HasChrRam() ) {
			chrRam[ addr ] = value;
			return 1;
		}
		return 0;
	}

	uint8_t Write( const uint16_t addr, const uint8_t value ) override
	{
		bank = ( value & 0x07 );
		return 0;
	};

	bool InWriteWindow( const uint16_t addr, const uint16_t offset ) override
	{
		const uint16_t address = ( addr + offset );
		return ( system->cart->GetMapperId() == 2 ) && InRange( address, wtSystem::ExpansionRomBase, wtSystem::Bank1End );
	}

	void Serialize( Serializer& serializer ) override
	{
		serializer.Next8b( bank );
		serializer.Next8b( lastBank );

		if( system->cart->HasChrRam() ) {
			serializer.NextArray( chrRam, PPU::PatternTableMemorySize );
		}
	}
};