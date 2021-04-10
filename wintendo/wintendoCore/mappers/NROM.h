#pragma once

#include "common.h"
#include "NesSystem.h"

class NROM : public wtMapper
{
private:
	uint8_t*	prgBanks[2];
	uint8_t*	chrBank;
public:
	NROM( const uint32_t _mapperId )
	{
		mapperId = _mapperId;
		prgBanks[0] = nullptr;
		prgBanks[1] = nullptr;
		chrBank = nullptr;
	}

	uint8_t OnLoadCpu() override
	{
		const uint8_t bank1 = ( system->cart->GetPrgBankCount() == 1 ) ? 0 : 1;
		prgBanks[ 0 ] = system->cart->GetPrgRomBank( 0 );
		prgBanks[ 1 ] = system->cart->GetPrgRomBank( bank1 );
		return 0;
	};

	uint8_t OnLoadPpu() override
	{
		chrBank = system->cart->GetChrRomBank( 0 );
		return 0;
	};

	uint8_t	ReadRom( const uint16_t addr ) override
	{
		const uint8_t bank = ( addr >> 14 ) & 0x01;
		const uint16_t offset = ( addr & ( wtSystem::BankSize - 1 ) );

		return prgBanks[ bank ][ offset ];
	}

	uint8_t	ReadChrRom( const uint16_t addr ) override
	{
		return chrBank[ addr ];
	}
};