#include "stdafx.h"
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include "common.h"
#include "NesSystem.h"
#include "mos6502.h"
#include "input.h"

using namespace std;

ButtonFlags keyBuffer[2] = { BUTTON_NONE, BUTTON_NONE };

void NesSystem::LoadProgram( const NesCart& loadCart, const uint32_t resetVectorManual )
{
	memset( memory, 0, VirtualMemorySize );

	memcpy( memory + Bank0, loadCart.rom, BankSize );

	// Mapper - 0
	if ( loadCart.header.prgRomBanks == 1 )
	{
		memcpy( memory + Bank1, loadCart.rom, BankSize );
	}
	else if ( loadCart.header.prgRomBanks == 2 )
	{
		memcpy( memory + Bank1, loadCart.rom + BankSize, BankSize );
	}
	else if ( loadCart.header.prgRomBanks > 2 )
	{
		// Should be the only logic needed for all cases
		size_t bank = loadCart.header.prgRomBanks - 1;
		memcpy( memory + Bank1, loadCart.rom + bank * BankSize, BankSize );
	}

	if ( resetVectorManual == 0x10000 )
	{
		cpu.resetVector = ( memory[ResetVectorAddr + 1] << 8 ) | memory[ResetVectorAddr];
	}
	else
	{
		cpu.resetVector = static_cast< uint16_t >( resetVectorManual & 0xFFFF );
	}

	prgRomBank = 0;

	//const uint16_t baseAddr = loadCart.header.prgRomBanks * NesSystem::BankSize;

	//memcpy( &ppu.vram[0], &loadCart.rom[baseAddr], 0x2000 );

	cpu.nmiVector = Combine( memory[NmiVectorAddr], memory[NmiVectorAddr + 1] );
	cpu.irqVector = Combine( memory[IrqVectorAddr], memory[IrqVectorAddr + 1] );

	cpu.interruptTriggered = false;
	cpu.resetTriggered = false;
	cpu.system = this;
	cpu.Reset();
}


bool NesSystem::IsInputRegister( const uint16_t address )
{
	return ( ( address == InputRegister0 ) || ( address == InputRegister1 ) );
}


bool NesSystem::IsPpuRegister( const uint16_t address )
{
	return ( address >= PpuRegisterBase ) && ( address < PpuRegisterEnd );
}


bool NesSystem::IsApuRegister( const uint16_t address )
{
	return ( address >= ApuRegisterBase ) && ( address < ApuRegisterEnd );
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
	else if ( IsInputRegister( address ) )
	{
		int controllerIndex = address - InputRegister0;

		controllerBuffer[controllerIndex] = 0; // FIXME: this register is a hack to get around bad GetMemory func design

		if ( strobeOn )
		{
			controllerBuffer[controllerIndex] = GetKeyBuffer( controllerIndex ) & ((ButtonFlags)0X80);
			btnShift[controllerIndex] = 0;

			return controllerBuffer[controllerIndex];
		}

		controllerBuffer[controllerIndex] = ( GetKeyBuffer( controllerIndex ) >> ( 7 - btnShift[controllerIndex] ) ) & 0x01;
		++btnShift[controllerIndex];
		btnShift[controllerIndex] %= 8;

		return controllerBuffer[controllerIndex];
	}
	else if( IsApuRegister( address ) )
	{
		return apuDummyRegister;
	}
	else
	{
		return memory[MirrorAddress( address )];
	}
}


void NesSystem::WritePhysicalMemory( const uint16_t address, const uint8_t value )
{
	memory[MirrorAddress(address)] = value;
}


void NesSystem::WriteInput( const uint8_t value )
{

}


void NesSystem::CaptureInput( const Controller keys )
{
	controller.exchange( keys );
}


bool NesSystem::Run( const masterCycles_t& nextCycle )
{
	bool isRunning = true;

	static const masterCycles_t ticks( CpuClockDivide );

#if DEBUG_MODE == 1
	if( ( cpu.logFrameCount > 0 ) && !cpu.logFile.is_open() )
	{
		cpu.logFile.open( "tomTendo.log" );
	}
	auto start = chrono::steady_clock::now();
#endif // #if DEBUG_MODE == 1

	//CHECK WRAP AROUND LOGIC
	while ( ( sysCycles < nextCycle ) && isRunning )
	{
		sysCycles += ticks;

		isRunning = cpu.Step( chrono::duration_cast<cpuCycle_t>( sysCycles ) );
		ppu.Step( chrono::duration_cast<ppuCycle_t>( cpu.cycle ) );
	}

#if DEBUG_MODE == 1
	auto end = chrono::steady_clock::now();

	auto elapsed = end - start;
	auto dur = chrono::duration <double, milli>( elapsed ).count();

	//	cout << "Elapsed:" << dur << ": Cycles: " << cpu.cycle.count() << endl;
#endif // #if DEBUG_MODE == 1

#if DEBUG_ADDR == 1
	if ( cpu.logFrameCount <= 0 )
	{
		cpu.logFile.close();
	}
	cpu.logFrameCount--;
#endif // #if DEBUG_ADDR == 1

	return isRunning;
}