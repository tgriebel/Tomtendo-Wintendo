#include "stdafx.h"
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include "common.h"
#include "NesSystem.h"
#include "6502.h"

using namespace std;


void NesSystem::LoadProgram( const NesCart& cart, const uint32_t resetVectorManual )
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
		cpu.resetVector = static_cast< uint16_t >( resetVectorManual & 0xFFFF );
	}

	cpu.nmiVector = Combine( memory[NmiVectorAddr], memory[NmiVectorAddr + 1] );
	cpu.irqVector = Combine( memory[IrqVectorAddr], memory[IrqVectorAddr + 1] );

	cpu.interruptTriggered = false;
	cpu.resetTriggered = false;
	cpu.system = this;
	cpu.Reset();
}


bool NesSystem::IsPpuRegister( const uint16_t address )
{
	return ( address >= PpuRegisterBase ) && ( address < PpuRegisterEnd );
}


bool NesSystem::IsDMA( const uint16_t address )
{
	// TODO: port technically on CPU
	return ( address == PpuOamDma );
}


uint16_t NesSystem::MirrorAddress( const uint16_t address )
{
	if ( IsPpuRegister( address ) )
	{
		return ( address % PPU::RegisterCount ) + PpuRegisterBase;
	}
	else if ( IsDMA( address ) )
	{
		return address;
	}
	else if ( ( address >= PhysicalMemorySize ) && ( address < PpuRegisterBase ) )
	{
		assert( 0 );
		return ( address % PhysicalMemorySize );
	}
	else
	{
		return ( address % MemoryWrap );
	}
}


uint8_t& NesSystem::GetStack()
{
	return memory[StackBase + cpu.SP];
}


uint8_t& NesSystem::GetMemory( const uint16_t address )
{
	if ( IsPpuRegister( address ) )
	{
		return ppu.Reg( address );
	}
	else if ( IsDMA( address ) )
	{
		return memory[MirrorAddress( address )];
	}
	else
	{
		return memory[MirrorAddress( address )];
	}
}


bool NesSystem::Run( const masterCycles_t nextCycle )
{
#if DEBUG_ADDR == 1
	cpu.logFile.open( "tomTendo.log" );
#endif // #if DEBUG_ADDR == 1

	bool isRunning = true;

	static const masterCycles_t ticks( CpuClockDivide );

#if DEBUG_MODE == 1
	auto start = chrono::steady_clock::now();
#endif // #if DEBUG_MODE == 1

	//CHECK WRAP AROUND LOGIC
	while ( ( sysCycles < nextCycle ) && isRunning )
	{
		sysCycles += ticks;

		isRunning = cpu.Step( chrono::duration_cast<cpuCycle_t>( sysCycles ) );
		ppu.Step( chrono::duration_cast<ppuCycle_t>( sysCycles ) );
	}

#if DEBUG_MODE == 1
	auto end = chrono::steady_clock::now();

	auto elapsed = end - start;
	auto dur = chrono::duration <double, milli>( elapsed ).count();

	//	cout << "Elapsed:" << dur << ": Cycles: " << cpu.cycle.count() << endl;
#endif // #if DEBUG_MODE == 1

	cpu.cycle -= chrono::duration_cast<cpuCycle_t>( sysCycles );
	ppu.cycle -= chrono::duration_cast<ppuCycle_t>( sysCycles );
	sysCycles -= nextCycle;

#if DEBUG_ADDR == 1
	cpu.logFile.close();
#endif // #if DEBUG_ADDR == 1

	return isRunning;
}