/*
* MIT License
*
* Copyright( c ) 2023 Thomas Griebel
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

#include "input.h"
#include "command.h"
#include "playback.h"
#include "time.h"
#include "timer.h"
#include "util.h"
#include "image.h"
#include "serializer.h"
#include "log.h"

#include <cstdint>

class wtSystem;

namespace Tomtendo
{
	class wtLog;
	struct config_t;
	struct wtFrameResult;

	class Emulator
	{
	private:
		wtSystem* system = nullptr;
	public:

		~Emulator();

		Input	input;

		int		Boot( const std::wstring& filePath, const uint32_t resetVectorManual = 0x10000 );
		int		RunEpoch( const std::chrono::nanoseconds& runCycles );
		void	GetFrameResult( wtFrameResult& outFrameResult );
		void	SetConfig( config_t& cfg );

		void	SubmitCommand( const sysCmd_t& cmd );

		void	UpdateDebugImages();
		void	GenerateRomDissambly( std::string prgRomAsm[ 128 ] );
		void	GenerateChrRomTables( wtPatternTableImage chrRom[ 32 ] );
	};

	static const char* STATE_MEMORY_LABEL = "Memory";
	static const char* STATE_VRAM_LABEL	= "VRAM";

	uint32_t ScreenWidth();

	uint32_t ScreenHeight();

	uint32_t SpriteLimit();

	config_t DefaultConfig();

	enum analogMode_t
	{
		NTSC,
		PAL,
		ANALOG_MODE_COUNT,
	};

	enum class emulationFlags_t : uint32_t
	{
		NONE			= 1 << 0,
		CLAMP_FPS		= 1 << 1,
		LIMIT_STALL		= 1 << 2,
		HEADLESS		= 1 << 3,
		ALL				= 0xFFFFFFFF,
	};

	inline emulationFlags_t operator|=( emulationFlags_t lhs, emulationFlags_t rhs )
	{
		return static_cast<emulationFlags_t>( static_cast<uint32_t>( lhs ) | static_cast<uint32_t>( rhs ) );
	}
	
	inline bool operator&( emulationFlags_t lhs, emulationFlags_t rhs )
	{
		return ( ( static_cast<uint32_t>( lhs ) & static_cast<uint32_t>( rhs ) ) != 0 );
	}

	struct config_t
	{
		struct System
		{
			emulationFlags_t	flags;
		} sys;

		//struct CPU
		//{
		//} cpu;

		struct APU
		{
			float				volume;
			float				frequencyScale;
			int32_t				waveShift;
			bool				disableSweep;
			bool				disableEnvelope;
			bool				mutePulse1;
			bool				mutePulse2;
			bool				muteTri;
			bool				muteNoise;
			bool				muteDMC;
			uint8_t				dbgChannelBits;
		} apu;

		struct PPU
		{
			int32_t				chrPalette;
			int32_t				spriteLimit;
			bool				showBG;
			bool				showSprite;
		} ppu;
	};

	struct debugTiming_t
	{
		uint32_t		frameTimeUs;
		uint32_t		totalTimeUs;
		uint32_t		simulationTimeUs;
		uint32_t		realTimeUs;
		uint64_t		frameNumber;
		uint64_t		framePerRun;
		uint64_t		runInvocations;
		masterCycle_t	cycleBegin;
		masterCycle_t	cycleEnd;
		masterCycle_t	stateCycle;
	};

	struct cpuDebug_t
	{
		uint8_t			X;
		uint8_t			Y;
		uint8_t			A;
		uint8_t			SP;
		uint8_t			P;
		bool			carry;
		bool			zero;
		bool			interrupt;
		bool			decimal;
		bool			unused;
		bool			brk;
		bool			overflow;
		bool			negative;
		uint16_t		PC;
		uint16_t		resetVector;
		uint16_t		nmiVector;
		uint16_t		irqVector;
	};

	struct apuPulseDebug_t
	{
		int		duty;
		bool	constant;
		int		volume;
		int		counterHalt;
		int		timer;
		int		counter;
		int		period;
		int		sweepDelta;
		bool	sweepEnabled;
		int		sweepPeriod;
		int		sweepShift;
		int		sweepNegate;

	};

	struct apuTriangleDebug_t
	{
		int lengthCounter;
		int linearCounter;
		int timer;
		int reg4008_halt;
		int reg4008_load;
		int reg400A_timer;
		int reg400B_counter;
	};

	struct apuNoiseDebug_t
	{
		int shifter;
		int timer;
		int reg400C_halt;
		int reg400C_constant;
		int reg400C_volume;
		int reg400E_mode;
		int reg400E_period;
		int reg400E_length;
	};

	struct apuDmcDebug_t
	{
		int outputLevel;
		int sampleBuffer;
		int bitCount;
		int bytesRemaining;
		int period;
		int periodCounter;
		float frequency;

		int reg4010_Loop;
		int reg4010_Freq;
		int reg4010_Irq;
		int reg4011_load;
		int reg4012_addr;
		int reg4013_length;
	};

	struct apuDebug_t
	{
		apuPulseDebug_t		pulse1;
		apuPulseDebug_t		pulse2;
		apuNoiseDebug_t		noise;
		apuTriangleDebug_t	triangle;
		apuDmcDebug_t		dmc;

		bool				pulse1Enabled;
		bool				pulse2Enabled;
		bool				triangleEnabled;
		bool				noiseEnabled;
		bool				dmcEnabled;
		uint32_t			halfClkTicks;
		uint32_t			quarterClkTicks;
		uint32_t			irqClkEvents;
		cpuCycle_t			frameCounterTicks;
		cpuCycle_t			cycle;
		apuCycle_t			apuCycle;
	};

	struct ppuDebug_t
	{
		struct pickedSprite_t
		{
			uint8_t	x;
			uint8_t	y;
			uint8_t	tileId;
			uint8_t	palette;
			uint8_t	oamIndex;
			uint8_t	secondaryOamIndex;	// debugging
			uint8_t	tableId;			// debugging
			uint8_t	flippedHorizontal;
			uint8_t	flippedVertical;
			uint8_t	priority;
			bool	sprite0;
			bool	is8x16;
		} picked;
	};

	static constexpr uint32_t	ApuSamplesPerSec = static_cast<uint32_t>( CPU_HZ + 1 );
	static constexpr uint32_t	ApuBufferMs = static_cast<uint32_t>( 1000.0f / MinFPS );
	static constexpr uint32_t	ApuBufferSize = static_cast<uint32_t>( ApuSamplesPerSec * ( ApuBufferMs / 1000.0f ) );
	using wtSampleQueue = wtQueue< float, ApuBufferSize >;
	using wtSoundBuffer = wtBuffer< float, ApuBufferSize >;

	struct apuOutput_t
	{
		wtSampleQueue	dbgMixed;
		wtSampleQueue	dbgPulse1;
		wtSampleQueue	dbgPulse2;
		wtSampleQueue	dbgTri;
		wtSampleQueue	dbgNoise;
		wtSampleQueue	dbgDmc;
		wtSampleQueue	mixed;
	};

	struct stateHeader_t
	{
		uint8_t* memory;
		uint8_t* vram;
		uint32_t memorySize;
		uint32_t vramSize;
	};

	class wtStateBlob
	{
	public:

		wtStateBlob()
		{
			bytes = nullptr;
			byteCount = 0;
			cycle = masterCycle_t( 0 );
		}

		~wtStateBlob()
		{
			Reset();
		}

		bool		IsValid() const;
		uint32_t	GetBufferSize() const;

		uint8_t*	GetPtr();
		void		Set( Serializer& s, const masterCycle_t sysCycle );
		void		WriteTo( Serializer& s ) const;
		void		Reset();

		stateHeader_t	header;
	private:
		uint8_t*		bytes;
		uint32_t		byteCount;
		masterCycle_t	cycle;
	};

	enum wtMirrorMode : uint8_t
	{
		MIRROR_MODE_SINGLE,
		MIRROR_MODE_HORIZONTAL,
		MIRROR_MODE_VERTICAL,
		MIRROR_MODE_FOURSCREEN,
		MIRROR_MODE_SINGLE_LO,
		MIRROR_MODE_SINGLE_HI,
		MIRROR_MODE_COUNT
	};

	// TODO: bother with endianness?
	struct wtRomHeader
	{
		uint8_t type[ 3 ];
		uint8_t magic;
		uint8_t prgRomBanks;
		uint8_t chrRomBanks;
		struct ControlsBits0
		{
			uint8_t mirror : 1;
			uint8_t usesBattery : 1;
			uint8_t usesTrainer : 1;
			uint8_t fourScreenMirror : 1;
			uint8_t mapperNumberLower : 4;
		} controlBits0;
		struct ControlsBits1
		{
			uint8_t reserved0 : 4;
			uint8_t mappedNumberUpper : 4;
		} controlBits1;
		uint8_t reserved[ 8 ];
	};

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
};