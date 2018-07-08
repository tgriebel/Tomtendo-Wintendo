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
	static const half NmiVectorAddr = 0xFFFA;
	static const half IrqVectorAddr = 0xFFFE;
	static const uint NumInstructions = 256;
	static const half VirtualMemorySize = 0xFFFF;
	static const half PhysicalMemorySize = 0x0800;
	static const half ZeroPageWrap = 0x0100;
	static const uint MemoryWrap = ( VirtualMemorySize + 1 );
	static const half PpuRegisterBase = 0x2000;
	static const half PpuRegisterEnd = 0x4000;
	static const half PpuOamDma = 0x4014;
	static const half PageSize = 0xFF;

	byte& GetStack();
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
		cpu.forceStop = false;
		cpu.cycle = cpuCycle_t(0);
		ppu.cycle = ppuCycle_t(0);
		sysCycles = masterCycles_t(0);
		cpu.printToOutput = false;

		ppu.currentScanline = -1;
		ppu.currentPixel = 0;
		ppu.scanelineCycle = ppuCycle_t(0);

		ppu.regCtrl.raw = 0;
		ppu.regMask.raw = 0;
		ppu.regStatus.current.raw = 0;
		ppu.regStatus.latched.raw = 0;
		ppu.regStatus.hasLatch = false;

		ppu.vramWritePending = false;

		ppu.system = this;
	}

	void LoadProgram( const NesCart& cart, const uint resetVectorManual = 0x10000 );
	bool Run( const masterCycles_t nextCycle );
	bool IsPpuRegister( const half address );
	bool IsDMA( const half address );

	//private:
	byte memory[VirtualMemorySize];
};