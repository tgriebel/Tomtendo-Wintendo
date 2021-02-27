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
#include <deque>
#include "common.h"
#include "mos6502.h"
#include "ppu.h"
#include "apu.h"
#include "bitmap.h"
#include "input.h"
#include "serializer.h"

struct wtState;
struct wtFrameResult;
struct debugTiming_t;
struct config_t;

class wtSystem
{
public:
	// Buffer and partition sizes
	static const uint32_t VirtualMemorySize		= 0x10000;
	static const uint32_t PhysicalMemorySize	= 0x0800;
	static const uint32_t PageSize				= 0x0100;
	static const uint32_t ZeroPageSize			= 0x0100;
	static const uint32_t BankSize				= 0x4000;
	static const uint32_t SramSize				= 0x2000;
	static const uint32_t ChrRomSize			= 0x1000;
	static const uint32_t MemoryWrap			= 0x10000;
	static const uint32_t ZeroPageWrap			= 0x0100;

	// Partition offsets
	static const uint16_t StackBase				= 0x0100;
	static const uint16_t ExpansionRomBase		= 0x4020;
	static const uint16_t SramBase				= 0x6000;
	static const uint16_t SramEnd				= 0x7FFF;
	static const uint16_t Bank0					= 0x8000;
	static const uint16_t Bank0End				= 0xBFFF;
	static const uint16_t Bank1					= 0xC000;
	static const uint16_t Bank1End				= 0xFFFF;
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
	unique_ptr<wtCart>	cart;

	uint32_t			finishedFrame;
	uint32_t			currentFrame;
	uint64_t			frameNumber;
	uint64_t			previousFrameNumber;
	bool				savedState;
	bool				loadedState;
	bool				toggledFrame;
	wtDisplayImage		frameBuffer[2];

	bool				headless;

	uint8_t				mirrorMode;
	const config_t*		config;
	wtInput				input;

private:
	static const uint32_t MaxStates = 5000;

	wstring						fileName;
	wstring						baseFileName;
	Cpu6502						cpu;
	PPU							ppu;
	APU							apu;
	uint8_t						memory[ PhysicalMemorySize ];
	masterCycles_t				sysCycles;
	bool						replayFinished;
	bool						debugNTEnable;
	timePoint_t					previousTime;
	debugTiming_t				dbgInfo;
#if DEBUG_ADDR == 1
	std::map<uint16_t, uint8_t>	memoryDebug;
#endif // #if DEBUG_ADDR == 1
	wtNameTableImage			nameTableSheet;
	wtPaletteImage				paletteDebug;
	wtPatternTableImage			patternTable0;
	wtPatternTableImage			patternTable1;
	wt16x8ChrImage				pickedObj8x16;
	std::deque<wtStateBlob>		states;
	uint32_t					currentState;
	uint32_t					firstState;
	bool						strobeOn;
	uint8_t						btnShift[ 2 ];

public:
	wtSystem()
	{
		Reset();
	}

	void Reset()
	{
		sysCycles = masterCycles_t( 0 );
		previousTime = std::chrono::steady_clock::now();

		memset( memory, 0, PhysicalMemorySize );

		strobeOn = false;
		btnShift[0] = 0;
		btnShift[1] = 0;

		previousFrameNumber = 0;

		mirrorMode = MIRROR_MODE_HORIZONTAL;

		headless = false;
		debugNTEnable = true;

		loadedState = false;
		savedState = false;
		replayFinished = true;
		toggledFrame = false;

		frameBuffer[0].SetDebugName( "FrameBuffer1" );
		frameBuffer[1].SetDebugName( "FrameBuffer2" );
		nameTableSheet.SetDebugName( "nameTable" );
		paletteDebug.SetDebugName( "Palette" );
		patternTable0.SetDebugName( "PatternTable0" );
		patternTable1.SetDebugName( "PatternTable1" );
		pickedObj8x16.SetDebugName( "Picked Object 8x16" );

		frameBuffer[0].Clear();
		frameBuffer[1].Clear();
		nameTableSheet.Clear();
		paletteDebug.Clear();
		patternTable0.Clear();
		patternTable1.Clear();
		pickedObj8x16.Clear();

		states.clear();

		currentState = 0;
		firstState = 1;
		currentFrame = 0;
		finishedFrame = 1;
		frameNumber = 0;
		previousTime = chrono::steady_clock::now();
	}

	uint8_t&		GetStack();
	uint8_t			ReadMemory( const uint16_t address );
	void			WriteMemory( const uint16_t address, const uint16_t offset, const uint8_t value );
	uint8_t			GetMapperId() const;
	uint8_t			GetMirrorMode() const;

	int				Init( const wstring& filePath );
	void			Shutdown();
	void			LoadProgram( const uint32_t resetVectorManual = 0x10000 );
	void			Serialize( Serializer& serializer, const serializeMode_t mode );
	string			GetPrgBankDissambly( const uint8_t bankNum );
	void			GenerateRomDissambly( string prgRomAsm[128] );
	void			GenerateChrRomTables( wtPatternTableImage chrRom[32] );
	void			GetChrRomPalette( const uint8_t paletteId, RGBA palette[ 4 ] );
	bool			Run( const masterCycles_t& nextCycle );
	int				RunFrame();
	uint8_t			ReadInput( const uint16_t address );
	void			WriteInput( const uint16_t address, const uint8_t value );
	void			GetFrameResult( wtFrameResult& outFrameResult );
	void			GetState( wtState& state );
	const PPU&		GetPPU() const;
	const APU&		GetAPU() const;
	void			SetConfig( config_t& cfg );
	void			RequestNMI() const;
	void			RequestIRQ() const;
	void			RequestDMA() const;
	void			RequestDmcTransfer() const;
	void			SaveSate();
	void			LoadState();
	void			ToggleFrame();
	bool			MouseInRegion( const wtRect& region );
	static void		InitConfig( config_t& cfg );

	// Implemented in "mapper.h"
	unique_ptr<wtMapper> AssignMapper( const uint32_t mapperId );

private:
	void			DebugPrintFlushLog();
	void			WritePhysicalMemory( const uint16_t address, const uint8_t value );
	uint16_t		MirrorAddress( const uint16_t address ) const;
	void			RecordSate( wtStateBlob& state );
	void			RestoreState( const wtStateBlob& state );
	void			RunStateControl( const bool newFrame, masterCycles_t& nextCycle );
	void			SaveSRam();
	void			LoadSRam();

	static bool		IsInputRegister( const uint16_t address );
	static bool		IsPpuRegister( const uint16_t address );
	static bool		IsApuRegister( const uint16_t address );
	static bool		IsCartMemory( const uint16_t address );
	static bool		IsPhysicalMemory( const uint16_t address );
	static bool		IsDMA( const uint16_t address );
};


struct wtState
{
	static const uint32_t CpuMemorySize = wtSystem::PhysicalMemorySize;
	static const uint32_t PpuMemorySize = KB(2);

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
	uint64_t					currentFrame;
	wtDisplayImage				frameBuffer;
	apuOutput_t					soundOutput;
	bool						sndReady;
	bool						savedState;
	bool						loadedState;
	bool						replayFinished;

	// Debug
	debugTiming_t				dbgInfo;
	wtState						state;
	wtRomHeader					romHeader;
	wtMirrorMode				mirrorMode;
	uint32_t					mapperId;
	wtNameTableImage			nameTableSheet;
	wtPaletteImage				paletteDebug;
	wtPatternTableImage			patternTable0;
	wtPatternTableImage			patternTable1;
	wt16x8ChrImage				pickedObj8x16;
	apuDebug_t					apuDebug;
	ppuDebug_t					ppuDebug;
	wtLog*						dbgLog;
};