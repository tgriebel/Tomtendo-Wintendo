/*
* MIT License
*
* Copyright( c ) 2017-2021 Thomas Griebel
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this softwareand associated documentation files( the "Software" ), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright noticeand this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

#include <string>
#include <chrono>
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
#include "command.h"
#include "playback.h"
#include "serializer.h"
#include "cart.h"

struct cpuDebug_t;
struct wtFrameResult;
struct debugTiming_t;
struct config_t;
struct command_t;

struct wtFrameResult
{
	uint64_t					currentFrame;
	uint64_t					stateCount;
	playbackState_t				playbackState;
	wtDisplayImage*				frameBuffer;
	apuOutput_t*				soundOutput;
	wtStateBlob*				frameState;

	// Debug
	debugTiming_t				dbgInfo;
	wtRomHeader					romHeader;
	wtMirrorMode				mirrorMode;
	uint32_t					mapperId;
	uint64_t					dbgFrameBufferIx;
	uint64_t					frameToggleCount;
	wtNameTableImage*			nameTableSheet;
	wtPaletteImage*				paletteDebug;
	wtPatternTableImage*		patternTable0;
	wtPatternTableImage*		patternTable1;
	wt16x8ChrImage*				pickedObj8x16;
	cpuDebug_t					cpuDebug;
	apuDebug_t					apuDebug;
	ppuDebug_t					ppuDebug;
	wtLog*						dbgLog;
};


void BackgroundWorker();

class wtSystem
{
public:
	// Buffer and partition sizes
	static const uint32_t VirtualMemorySize		= 0x10000;
	static const uint32_t PhysicalMemorySize	= 0x0800;
	static const uint32_t PageSize				= 0x0100;
	static const uint32_t ZeroPageEnd			= 0x00FF;
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
	static const uint32_t OutputBuffersCount	= 3;

	// TODO: Need to abstract memory access for mappers
	unique_ptr<wtCart>			cart;

private:
	static const uint32_t		MaxStates = 5000;

	wstring						fileName;
	wstring						baseFileName;
	Cpu6502						cpu;
	PPU							ppu;
	APU							apu;
	uint8_t						memory[ PhysicalMemorySize ];
	masterCycle_t				sysCycles;
	bool						replayFinished;
	bool						debugNTEnable;
	int64_t						overflowCycles;
	debugTiming_t				dbgInfo;
#if DEBUG_ADDR == 1
	std::map<uint16_t, uint8_t>	memoryDebug;
#endif // #if DEBUG_ADDR == 1
	wtFrameResult				result;
	wtDisplayImage				frameBuffer[ OutputBuffersCount ];
	wtNameTableImage			nameTableSheet;
	wtPaletteImage				paletteDebug;
	wtPatternTableImage			patternTable0;
	wtPatternTableImage			patternTable1;
	wt16x8ChrImage				pickedObj8x16;
	std::deque<wtStateBlob>		states;
	wtStateBlob					frameState;
	uint32_t					currentState;
	uint32_t					firstState;
	bool						strobeOn;
	uint8_t						btnShift[ 2 ];
	std::deque<sysCmd_t>		commands;
	playbackState_t				playbackState;
	wtInput						input;
	const config_t*				config;
	uint32_t					currentFrameIx;
	uint32_t					finishedFrameIx;
	uint64_t					frameNumber;
	uint64_t					previousFrameNumber;
	uint64_t					frameTogglesPerRun;
	bool						toggledFrame;
	uint8_t						mirrorMode;

public:
	wtSystem()
	{
		Reset();
	}

	void Reset()
	{
		sysCycles = masterCycle_t( 0 );

		memset( memory, 0, PhysicalMemorySize );

		strobeOn = false;
		btnShift[0] = 0;
		btnShift[1] = 0;

		previousFrameNumber = 0;

		mirrorMode = MIRROR_MODE_HORIZONTAL;

		debugNTEnable = true;

		replayFinished = true;
		toggledFrame = false;

		nameTableSheet.SetDebugName( "nameTable" );
		paletteDebug.SetDebugName( "Palette" );
		patternTable0.SetDebugName( "PatternTable0" );
		patternTable1.SetDebugName( "PatternTable1" );
		pickedObj8x16.SetDebugName( "Picked Object 8x16" );

		for( uint32_t i = 0; i < OutputBuffersCount; ++i ) {
			frameBuffer[i].Clear();
			char dbgName[ 128 ];
			sprintf_s( dbgName, "FrameBuffer%i", i );
			frameBuffer[ i ].SetDebugName( dbgName );
		}
		nameTableSheet.Clear();
		paletteDebug.Clear();
		patternTable0.Clear();
		patternTable1.Clear();
		pickedObj8x16.Clear();

		states.clear();
		playbackState.currentFrame = 0;
		playbackState.replayState = replayStateCode_t::LIVE;
		playbackState.finalFrame = INT64_MAX;

		currentState = 0;
		firstState = 1;
		currentFrameIx = 0;
		finishedFrameIx = 1;
		frameNumber = 0;

		memset( &dbgInfo, 0, sizeof( dbgInfo ) );
	}

	// Emulation functions - TODO: make visible only to other emulation components
	uint8_t&				GetStack();
	uint8_t					ReadMemory( const uint16_t address );
	void					WriteMemory( const uint16_t address, const uint16_t offset, const uint8_t value );
	uint8_t					ReadZeroPage( const uint16_t address );
	uint8_t					GetMapperId() const;
	uint8_t					GetMirrorMode() const;
	void					SetMirrorMode( uint8_t mode );
	void					RequestNMI( const uint16_t vector ) const;
	void					RequestNMI() const;
	void					RequestIRQ() const;
	void					RequestDMA() const;
	void					RequestDmcTransfer() const;
	void					SaveFrameState();
	void					ToggleFrame();
	wtDisplayImage*			GetBackbuffer();
	void					Serialize( Serializer& serializer );
	unique_ptr<wtMapper>	AssignMapper( const uint32_t mapperId ); // In "mapper.h"

	// External functions
	int						Init( const wstring& filePath, const uint32_t resetVectorManual = 0x10000 );
	void					Shutdown();
	void					LoadProgram( const uint32_t resetVectorManual = 0x10000 );
	string					GetPrgBankDissambly( const uint8_t bankNum );
	void					GenerateRomDissambly( string prgRomAsm[128] );
	void					GenerateChrRomTables( wtPatternTableImage chrRom[32] );
	void					GetChrRomPalette( const uint8_t paletteId, RGBA palette[ 4 ] );
	void					GetGrayscalePalette( RGBA palette[ 4 ] );
	bool					Run( const masterCycle_t& nextCycle );
	int						RunEpoch( const std::chrono::nanoseconds& runCycles );
	uint8_t					ReadInput( const uint16_t address );
	void					WriteInput( const uint16_t address, const uint8_t value );
	void					GetFrameResult( wtFrameResult& outFrameResult );
	void					GetState( cpuDebug_t& state );
	const PPU&				GetPPU() const;
	const APU&				GetAPU() const;
	void					SetConfig( config_t& cfg );
	void					SaveSate();
	void					LoadState();
	bool					MouseInRegion( const wtRect& region );
	static void				InitConfig( config_t& cfg );
	wtInput*				GetInput();
	const config_t*			GetConfig();
	bool					HasNewFrame() const;
	void					UpdateDebugImages();
	void					SubmitCommand( const sysCmd_t& cmd ); // In "command.cpp"

private:
	void					DebugPrintFlushLog();
	void					WritePhysicalMemory( const uint16_t address, const uint8_t value );
	uint16_t				MirrorAddress( const uint16_t address ) const;
	void					RecordSate( wtStateBlob& state );
	void					RestoreState( const wtStateBlob& state );
	void					RunStateControl( const bool toggledFrame );
	void					SaveSRam();
	void					LoadSRam();
	void					BackgroundUpdate();

	// command.cpp
	void					ProcessCommands();

	static bool				IsInputRegister( const uint16_t address );
	static bool				IsPpuRegister( const uint16_t address );
	static bool				IsApuRegister( const uint16_t address );
	static bool				IsCartMemory( const uint16_t address );
	static bool				IsPhysicalMemory( const uint16_t address );
	static bool				IsDMA( const uint16_t address );
};