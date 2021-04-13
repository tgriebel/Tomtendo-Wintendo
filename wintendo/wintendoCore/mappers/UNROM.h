#pragma once

#include "common.h"
#include "NesSystem.h"

class UNROM : public wtMapper
{
private:
	uint8_t		bank;
	uint8_t		chrRam[ PPU::PatternTableMemorySize ];
	uint8_t*	prgBanks[ 2 ];
	uint8_t*	chrBank;
public:
	UNROM( const uint32_t _mapperId )
	{
		mapperId = _mapperId;
		bank = 0;
		prgBanks[ 0 ] = nullptr;
		prgBanks[ 1 ] = nullptr;
		chrBank = nullptr;
	}

	uint8_t OnLoadCpu() override
	{
		bank = 0;
		const uint8_t lastBank = ( system->cart->h.prgRomBanks - 1 );
		prgBanks[ 0 ] = system->cart->GetPrgRomBank( bank );
		prgBanks[ 1 ] = system->cart->GetPrgRomBank( lastBank );
		return 0;
	};

	uint8_t OnLoadPpu() override
	{
		if( system->cart->HasChrRam() ) {
			chrBank = chrRam;
		} else {
			chrBank = system->cart->GetChrRomBank( 0 );
		}
		memset( chrRam, 0, sizeof( PPU::PatternTableMemorySize ) );
		return 0;
	};

	uint8_t	ReadRom( const uint16_t addr ) const override
	{
		const uint8_t bank = ( addr >> 14 ) & 0x01;
		const uint16_t offset = ( addr & ( wtSystem::BankSize - 1 ) );

		return prgBanks[ bank ][ offset ];
	}

	uint8_t	ReadChrRom( const uint16_t addr ) const override
	{
		return chrBank[ addr ];
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
		prgBanks[ 0 ] = system->cart->GetPrgRomBank( bank );
		return 0;
	};

	bool InWriteWindow( const uint16_t addr, const uint16_t offset ) const override
	{
		const uint16_t address = ( addr + offset );
		return InRange( address, wtSystem::ExpansionRomBase, wtSystem::Bank1End );
	}

	void Serialize( Serializer& serializer ) override
	{
		serializer.Next8b( bank );

		if( system->cart->HasChrRam() ) {
			serializer.NextArray( chrRam, PPU::PatternTableMemorySize );
		}

		if( serializer.GetMode() == serializeMode_t::LOAD )
		{
			prgBanks[ 0 ] = system->cart->GetPrgRomBank( bank );
			if ( !system->cart->HasChrRam() ) {
				chrBank = system->cart->GetChrRomBank( 0 );
			}
		}
	}
};