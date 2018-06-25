#pragma once

#include <string>
#include <chrono>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <iomanip>
#include "common.h"
#include "6502.h"
#include "ppu.h"


class NesSystem
{
public:
	static const half StackBase = 0x0100;
	static const half Bank0 = 0x8000;
	static const half Bank1 = 0xC000;
	static const half BankSize = 0x4000;
	static const half ResetVectorAddr = 0xFFFC;
	static const half NmiAddr = 0xFFFC;
	static const half IrqAddr = 0xFFFA;
	static const uint NumInstructions = 256;
	static const half VirtualMemorySize = 0xFFFF;
	static const half PhysicalMemorySize = 0x0800;
	static const half ZeroPageWrap = 0x0100;
	static const uint MemoryWrap = ( VirtualMemorySize + 1 );

	byte& GetStack();
	void SetMemory( const half address, byte value );
	byte& GetMemory( const half address );
	half MirrorAddress( const half address );

	PPU ppu;
	Cpu6502 cpu;

	masterCycles_t sysCycles;

#if DEBUG_ADDR == 1
	std::map<half, byte> memoryDebug;
#endif // #if DEBUG_ADDR == 1

	NesSystem()
	{
	}

	void LoadProgram( const NesCart& cart, const uint resetVectorManual = 0x10000 );
	bool Run( masterCycles_t cycles );

	//private:
	byte memory[VirtualMemorySize];
};