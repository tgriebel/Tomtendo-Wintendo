#include "stdafx.h"
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "common.h"
#include "NesSystem.h"
#include "6502.h"


inline void NesSystem::SetMemory( const half address, byte value )
{
#if DEBUG_MODE == 1
	if ( ( address == 0x4014 || address == 0x2003 ) || ( ( address >= 0x2000 ) && ( address <= 0x2008 ) ) )
	{
		std::cout << "DMA/PPU Registers Address Hit" << std::endl;
	}
#endif // #if DEBUG_MODE == 1

	// TODO: Implement mirroring, etc
	memory[MirrorAddress( address )] = value;
}


void NesSystem::LoadProgram( const NesCart& cart, const uint resetVectorManual )
{
	memset( memory, 0, VirtualMemorySize );

	memcpy( memory + Bank0, cart.rom, BankSize );

	assert( cart.header.prgRomBanks <= 2 );

	if ( cart.header.prgRomBanks == 1 )
	{
		memcpy( memory + Bank1, cart.rom, BankSize );
	}
	else
	{
		memcpy( memory + Bank1, cart.rom + Bank1, BankSize );
	}

	if ( resetVectorManual == 0x10000 )
	{
		cpu.resetVector = ( memory[ResetVectorAddr + 1] << 8 ) | memory[ResetVectorAddr];

	}
	else
	{
		cpu.resetVector = static_cast< half >( resetVectorManual & 0xFFFF );
	}

	cpu.interruptTriggered = false;
	cpu.resetTriggered = false;
	cpu.system = this;
	cpu.Reset();
}