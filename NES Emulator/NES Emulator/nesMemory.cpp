#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <sstream>
#include <assert.h>
#include <map>
#include <bitset>
#include "common.h"
#include "debug.h"
#include "6502.h"
#include "NesSystem.h"


uint8_t& Cpu6502::Read( const InstrParams& params )
{
	uint32_t address = 0x00;

	const AddrFunction getAddr = params.getAddr;

	uint8_t& M = ( this->*getAddr )( params, address );

	return M;
}


void Cpu6502::Write( const InstrParams& params, const uint8_t value )
{
	assert( params.getAddr != &Cpu6502::Immediate );

	uint32_t address = 0x00;

	const AddrFunction getAddr = params.getAddr;

	uint8_t& M = ( this->*getAddr )( params, address );

	// TODO: Check is RAM, otherwise do another GetMem, get will do the routing logic
	// Might need to have a different wrapper that passes the work to different handlers
	// That could help with RAM, I/O regs, and mappers
	// Wait until working on mappers to design
	// Addressing functions will call a lower level RAM function only
	// TODO: Need a memory manager class

	if ( system->IsPpuRegister( address ) )
	{
		system->ppu.Reg( address, value );
	}
	else if ( system->IsDMA( address ) )
	{
		system->ppu.OAMDMA( value );
	}
	else
	{
		//uint8_t& writeLoc = system->GetMemory( address );
		//assert( M == writeLoc );
		//assert( &M == &writeLoc );
		M = value;
	}

#if DEBUG_MEM == 1
	system->memoryDebug[address] = value;
#endif // #if DEBUG_MEM == 1
}
