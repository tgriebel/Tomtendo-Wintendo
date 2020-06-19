#pragma once

#include "common.h"
#include "NesSystem.h"

class UNROM : public wtMapper
{
	uint8_t OnLoadCpu()
	{
		const size_t lastBank = ( system->cart.header.prgRomBanks - 1 );
		memcpy( &system->memory[wtSystem::Bank0], system->cart.rom, wtSystem::BankSize );
		memcpy( &system->memory[wtSystem::Bank1], &system->cart.rom[lastBank * wtSystem::BankSize], wtSystem::BankSize );

		return 0; 
	};
	uint8_t OnLoadPpu() { return 0; };
	uint8_t Write( const uint16_t addr, const uint16_t offset, const uint8_t value )
	{
		size_t bank = ( value & 0x07 );
		memcpy( &system->memory[wtSystem::Bank0], &system->cart.rom[bank * wtSystem::BankSize], wtSystem::BankSize );

		return 0;
	};

	bool InWriteWindow( const uint16_t addr, const uint16_t offset )
	{
		const uint16_t address = ( addr + offset );
		return ( system->cart.GetMapperId() == 2 ) && InRange( address, 0x8000, 0xFFFF );
	}
};