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
#include "apu.h"
#include "bitmap.h"

struct wtState;
struct wtFrameResult;
struct wtDebugInfo;
struct wtConfig;

class wtSystem
{
public:
	// Buffer and partition sizes
	static const uint32_t VirtualMemorySize		= 0x10000;
	static const uint32_t PhysicalMemorySize	= 0x0800;
	static const uint32_t PageSize				= 0x0100;
	static const uint32_t ZeroPageSize			= 0x0100;
	static const uint32_t BankSize				= 0x4000;
	static const uint32_t ChrRomSize			= 0x1000;
	static const uint32_t MemoryWrap			= 0x10000;
	static const uint32_t ZeroPageWrap			= 0x0100;

	// Partition offsets
	static const uint16_t StackBase				= 0x0100;
	static const uint16_t Bank0					= 0x8000;
	static const uint16_t Bank1					= 0xC000;
	static const uint16_t ResetVectorAddr		= 0xFFFC;
	static const uint16_t NmiVectorAddr			= 0xFFFA;
	static const uint16_t IrqVectorAddr			= 0xFFFE;

	// Register locations
	static const uint16_t PpuRegisterBase		= 0x2000;
	static const uint16_t PpuRegisterEnd		= 0x3FFF;
	static const uint16_t ApuRegisterBase		= 0x4000;
	static const uint16_t ApuRegisterEnd		= 0x4015;
	static const uint16_t ApuRegisterStatus		= 0x4015;
	static const uint16_t ApuRegisterCounter	= 0x4017;
	static const uint16_t PpuOamDma				= 0x4014;
	static const uint16_t InputRegister0		= 0x4016;
	static const uint16_t InputRegister1		= 0x4017;

	// TODO: Need to abstract memory access for mappers
	PPU					ppu;
	masterCycles_t		sysCycles;

	wstring				fileName;
	wtCart				cart;

	pair<uint32_t,bool>	finishedFrame;
	uint32_t			currentFrame;
	uint64_t			frameNumber;
	wtDisplayImage		frameBuffer[2];
	wtNameTableImage	nameTableSheet;
	wtPaletteImage		paletteDebug;
	wtPatternTableImage	patternTable0;
	wtPatternTableImage	patternTable1;

	// TODO: Need to abstract memory access for mappers
	// TODO: make physical size
	uint8_t				memory[VirtualMemorySize];

	bool				headless;

	// TODO: Need to support two controllers and clean this up
	std::atomic<Controller> controller;
	bool				strobeOn;
	uint8_t				btnShift[2];

	uint8_t				mirrorMode;
	wtConfig			config;

private:
	Cpu6502				cpu;
	APU					apu;
	bool				debugNTEnable;
	wtDebugInfo			dbgInfo;
#if DEBUG_ADDR == 1
	std::map<uint16_t, uint8_t> memoryDebug;
#endif // #if DEBUG_ADDR == 1

public:
	wtSystem()
	{
		Reset();
	}

	void Reset()
	{
		InitConfig();

		cpu.cycle = cpuCycle_t( 0 );
		sysCycles = masterCycles_t( 0 );

		memset( memory, 0, sizeof( memory ) );

		strobeOn = false;
		btnShift[0] = 0;
		btnShift[1] = 0;

		mirrorMode = MIRROR_MODE_HORIZONTAL;

		headless = false;
		debugNTEnable = true;

		frameBuffer[0].SetDebugName( "FrameBuffer1" );
		frameBuffer[1].SetDebugName( "FrameBuffer2" );
		nameTableSheet.SetDebugName( "nameTable" );
		paletteDebug.SetDebugName( "Palette" );
		patternTable0.SetDebugName( "PatternTable0" );
		patternTable1.SetDebugName( "PatternTable1" );

		finishedFrame.first = 0;
		finishedFrame.second = false;
		frameNumber = 0;
	}

	uint8_t&	GetStack();
	uint8_t		ReadMemory( const uint16_t address );
	void		WriteMemory( const uint16_t address, const uint16_t offset, const uint8_t value );
	uint8_t		GetMapperId();
	uint8_t		GetMirrorMode();

	int			Init( const wstring& filePath );
	void		Shutdown();
	void		LoadProgram( wtCart& cart, const uint32_t resetVectorManual = 0x10000 );
	string		GetPrgBankDissambly( const uint8_t bankNum );
	void		GenerateRomDissambly( string prgRomAsm[16] );
	void		GenerateChrRomTables( wtPatternTableImage chrRom[16] );
	unique_ptr<wtMapper> AssignMapper( const uint32_t mapperId );
	bool		Run( const masterCycles_t& nextCycle );
	int			RunFrame();
	void		CaptureInput( const Controller keys );
	void		WriteInput( const uint8_t value );
	void		GetFrameResult( wtFrameResult& outFrameResult );
	void		GetState( wtState& state );
	void		SyncState( wtState& state );
	void		GetConfig( wtConfig& config );
	void		SyncConfig( wtConfig& config );
	void		InitConfig();
	void		RequestNMI();
	void		RequestIRQ();
	void		RequestDMA();

	static bool	MouseInRegion( const wtRect& region );

private:
	void		DebugPrintFlushLog();
	void		WritePhysicalMemory( const uint16_t address, const uint8_t value );
	uint16_t	MirrorAddress( const uint16_t address );

	static bool	IsInputRegister( const uint16_t address );
	static bool	IsPpuRegister( const uint16_t address );
	static bool	IsApuRegister( const uint16_t address );
	static bool	IsDMA( const uint16_t address );

};


struct wtState
{
	static const uint32_t CpuMemorySize = wtSystem::VirtualMemorySize;
	static const uint32_t PpuMemorySize = PPU::VirtualMemorySize;

	uint8_t				X;
	uint8_t				Y;
	uint8_t				A;
	uint8_t				SP;
	statusReg_t			P;
	uint16_t			PC;
	uint8_t				cpuMemory[CpuMemorySize];
	uint8_t				ppuMemory[PpuMemorySize];
};


struct wtFrameResult
{
	uint64_t			currentFrame;
	wtDisplayImage		frameBuffer;
	apuOutput_t			soundOutput;
	bool				sndReady;

	// Debug
	OpDebugInfo			dbgMetrics;
	wtDebugInfo			dbgInfo;
	wtState				state;
	wtRomHeader			romHeader;
	wtMirrorMode		mirrorMode;
	uint32_t			mapperId;
	wtNameTableImage	nameTableSheet;
	wtPaletteImage		paletteDebug;
	wtPatternTableImage patternTable0;
	wtPatternTableImage patternTable1;
	apuDebug_t			apuDebug;
};