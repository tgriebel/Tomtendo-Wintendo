#pragma once

#include "common.h"
#include "NesSystem.h"

class NROM : public wtMapper
{
	uint8_t OnLoadCpu()
	{
		const size_t lastBank = ( system->cart.header.prgRomBanks - 1 );
		memcpy( &system->memory[wtSystem::Bank0], system->cart.rom, wtSystem::BankSize );
		memcpy( &system->memory[wtSystem::Bank1], &system->cart.rom[lastBank * wtSystem::BankSize], wtSystem::BankSize );

		return 0;
	};

	uint8_t OnLoadPpu() { return 0; };
	uint8_t Write( const uint16_t addr, const uint16_t offset, const uint8_t value ) { return 0; };

	bool InWriteWindow( const uint16_t addr, const uint16_t offset )
	{
		return false;
	}
};