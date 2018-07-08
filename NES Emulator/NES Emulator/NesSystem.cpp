#include "stdafx.h"
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "common.h"
#include "NesSystem.h"
#include "6502.h"


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

	cpu.nmiVector = Combine( memory[NmiVectorAddr], memory[NmiVectorAddr + 1] );
	cpu.irqVector = Combine( memory[IrqVectorAddr], memory[IrqVectorAddr + 1] );

	cpu.interruptTriggered = false;
	cpu.resetTriggered = false;
	cpu.system = this;
	cpu.Reset();
}