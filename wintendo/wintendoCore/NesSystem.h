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


extern const RGBA DefaultPalette[];

struct Controller
{


};



class NesSystem
{
public:

	// Buffer and partition sizes
	static const uint32_t VirtualMemorySize		= 0x10000;
	static const uint32_t PhysicalMemorySize	= 0x0800;
	static const uint32_t PageSize				= 0x0100;
	static const uint32_t ZeroPageSize			= PageSize;
	static const uint32_t BankSize				= 0x4000;
	static const uint32_t ChrRomSize			= 0x1000;
	static const uint32_t MemoryWrap			= VirtualMemorySize;
	static const uint32_t ZeroPageWrap			= ZeroPageSize;

	// Partition offsets
	static const uint16_t StackBase				= ZeroPageSize;
	static const uint16_t Bank0					= 0x8000;
	static const uint16_t Bank1					= Bank0 + BankSize;
	static const uint16_t ResetVectorAddr		= 0xFFFC;
	static const uint16_t NmiVectorAddr			= 0xFFFA;
	static const uint16_t IrqVectorAddr			= 0xFFFE;

	// Register locations
	static const uint16_t PpuRegisterBase		= 0x2000;
	static const uint16_t PpuRegisterEnd		= 0x4000;
	static const uint16_t ApuRegisterBase		= PpuRegisterEnd;
	static const uint16_t ApuRegisterEnd		= 0x4014;
	static const uint16_t PpuOamDma				= 0x4014;
	static const uint16_t InputRegister0		= 0x4016;
	static const uint16_t InputRegister1		= 0x4017;

	static const uint32_t NumInstructions = 256;

	// Move this and framebuffer to PPU?
	static const uint32_t ScreenWidth = 256;
	static const uint32_t ScreenHeight = 240;

	Cpu6502			cpu;
	PPU				ppu;
	masterCycles_t	sysCycles;

	const NesCart*	cart;

	uint32_t frameBuffer[ScreenWidth * ScreenHeight];
	uint32_t nameTableSheet[4 * ScreenWidth * ScreenHeight];
	uint32_t paletteDebug[4 * 16 * 2];

	uint8_t memory[VirtualMemorySize]; // TODO: make physical size

	uint8_t apuDummyRegister;

	bool headless;
	bool debugNT;

	// TODO: Need to support two controllers and clean this up
	std::atomic<Controller> controller;
	bool	strobeOn;
	uint8_t	btnShift;
	uint8_t controllerBuffer0;
	uint8_t controllerBuffer1;

#if DEBUG_ADDR == 1
	std::map<uint16_t, uint8_t> memoryDebug;
#endif // #if DEBUG_ADDR == 1

	NesSystem()
	{
		cpu.forceStop = false;
		cpu.cycle = cpuCycle_t(0);
		ppu.cycle = ppuCycle_t(0);
		sysCycles = masterCycles_t(0);

		ppu.currentScanline	= -1;
		ppu.currentPixel	= 0;
		ppu.scanelineCycle	= ppuCycle_t(0);

		ppu.regCtrl.raw = 0;
		ppu.regMask.raw = 0;
		ppu.regStatus.current.raw = 0;
		ppu.regStatus.latched.raw = 0;
		ppu.regStatus.hasLatch = false;

		ppu.vramWritePending = false;

		ppu.system = this;
		ppu.palette= &DefaultPalette[0];

		ppu.beamPosition.x = 0;
		ppu.beamPosition.y = 0;

		strobeOn = false;
		btnShift = 0;

		ppu.regT.raw = 0;
		ppu.regV.raw = 0;
		ppu.regV0.raw = 0;

		ppu.curShift = 0;
		ppu.plLatches.flags = 0;

		cart = nullptr;

		headless = false;
		debugNT = true;
	}

	uint8_t& GetStack();
	uint8_t& GetMemory( const uint16_t address );
	uint16_t MirrorAddress( const uint16_t address );

	void LoadProgram( const NesCart& cart, const uint32_t resetVectorManual = 0x10000 );
	bool Run( const masterCycles_t& nextCycle );
	bool IsInputRegister( const uint16_t address );
	bool IsPpuRegister( const uint16_t address );
	bool IsApuRegister( const uint16_t address );
	bool IsDMA( const uint16_t address );
	void CaptureInput( const Controller keys );
	void WriteInput( const uint8_t value );
};