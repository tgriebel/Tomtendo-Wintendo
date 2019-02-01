#pragma once

#include <string>
#include <chrono>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <iomanip>
#include <atomic>
#include "common.h"
#include "mos6502.h"
#include "ppu.h"
#include "bitmap.h"


struct Controller
{


};



class NesSystem
{
public:
	static const uint16_t StackBase = 0x0100;
	static const uint16_t Bank0 = 0x8000;
	static const uint16_t Bank1 = 0xC000;
	static const uint16_t BankSize = 0x4000;
	static const uint16_t ChrRomSize = 0x1000;
	static const uint16_t ResetVectorAddr = 0xFFFC;
	static const uint16_t NmiVectorAddr = 0xFFFA;
	static const uint16_t IrqVectorAddr = 0xFFFE;
	static const uint32_t NumInstructions = 256;
	static const uint16_t VirtualMemorySize = 0xFFFF;
	static const uint16_t PhysicalMemorySize = 0x0800;
	static const uint16_t ZeroPageWrap = 0x0100;
	static const uint32_t MemoryWrap = ( VirtualMemorySize + 1 );
	static const uint16_t PpuRegisterBase = 0x2000;
	static const uint16_t PpuRegisterEnd = 0x4000;
	static const uint16_t PpuOamDma = 0x4014;
	static const uint16_t InputRegister0 = 0x4016;
	static const uint16_t InputRegister1 = 0x4017;
	static const uint16_t ApuRegisterBase = 0x4000;
	static const uint16_t ApuRegisterEnd = 0x4014;
	static const uint16_t PageSize = 0x00FF;

	// Move this and framebuffer to PPU?
	static const uint32_t ScreenWidth = 256;
	static const uint32_t ScreenHeight = 240;

	uint8_t& GetStack();
	uint8_t& GetMemory( const uint16_t address );
	uint16_t MirrorAddress( const uint16_t address );

	PPU ppu;
	Cpu6502 cpu;

	const NesCart* cart;
	const RGBA* palette;

	uint32_t frameBuffer[ScreenWidth * ScreenHeight];
	uint32_t nameTableSheet[4 * ScreenWidth * ScreenHeight];

	uint8_t controllerBuffer0;
	uint8_t controllerBuffer1;

	uint8_t apuDummyRegister;

	std::atomic<Controller> controller;

	masterCycles_t sysCycles;

	bool strobeOn;
	// TODO: Need to support two controllers
	uint8_t btnShift;

#if DEBUG_ADDR == 1
	std::map<uint16_t, uint8_t> memoryDebug;
#endif // #if DEBUG_ADDR == 1

	NesSystem()
	{
		cpu.forceStop = false;
		cpu.cycle = cpuCycle_t(0);
		ppu.cycle = ppuCycle_t(0);
		sysCycles = masterCycles_t(0);
#if DEBUG_ADDR == 1
		cpu.printToOutput = false;
#endif // #if DEBUG_ADDR == 1

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

		strobeOn = false;
		btnShift = 0;
	}

	void LoadProgram( const NesCart& cart, const uint32_t resetVectorManual = 0x10000 );
	bool Run( const masterCycles_t nextCycle );
	bool IsInputRegister( const uint16_t address );
	bool IsPpuRegister( const uint16_t address );
	bool IsApuRegister( const uint16_t address );
	bool IsDMA( const uint16_t address );
	void CaptureInput( const Controller keys );
	void WriteInput( const uint8_t value );

	//private:
	uint8_t memory[VirtualMemorySize];
};