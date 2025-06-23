#pragma once

#include "base.h"
#include "input.h"
#include "command.h"
#include "playback.h"
#include "time.h"
#include "timer.h"
#include "util.h"
#include "image.h"
#include "serializer.h"
#include "log.h"

class wtSystem;

namespace Tomtendo
{
	class wtLog;
	struct config_t;
	struct wtFrameResult;

	struct Emulator
	{
		wtSystem*	system = nullptr;
		Input		input;
	};

	EXPORT_DLL bool	Boot( Emulator* emu, const wchar_t* filePath, const uint32_t resetVectorManual = 0x10000 );
	EXPORT_DLL void	Shutdown( Emulator* emu );
	EXPORT_DLL int	RunEpoch( Emulator* emu, const std::chrono::nanoseconds& runCycles );
	EXPORT_DLL void	GetFrameResult( Emulator* emu, wtFrameResult& outFrameResult );
	EXPORT_DLL void	SetConfig( Emulator* emu, config_t& cfg );

	EXPORT_DLL void	SubmitCommand( Emulator* emu, const sysCmd_t& cmd );

	EXPORT_DLL void	UpdateDebugImages( Emulator* emu );
	EXPORT_DLL void	GenerateRomDissambly( Emulator* emu, std::string prgRomAsm[ 128 ] );
	EXPORT_DLL void	GenerateChrRomTables( Emulator* emu, wtPatternTableImage chrRom[ 32 ] );

	static const char* STATE_MEMORY_LABEL = "Memory";
	static const char* STATE_VRAM_LABEL	= "VRAM";

	EXPORT_DLL void CreateEmulatorInstance( Emulator** emulatorInstance );

	EXPORT_DLL void DestroyEmulatorInstance( Emulator** emulatorInstance );

	EXPORT_DLL uint32_t	ScreenWidth();

	EXPORT_DLL uint32_t	ScreenHeight();

	EXPORT_DLL uint32_t	SpriteLimit();

#ifdef IMPORT_WIN
	struct runtimeDllInterface_t
	{
		typedef void( __cdecl* PFN_CreateEmulatorInstance )( Emulator** emulatorInstance );
		PFN_CreateEmulatorInstance CreateEmulatorInstance = nullptr;

		typedef void( __cdecl* PFN_DestroyEmulatorInstance )( Emulator** emulatorInstance );
		PFN_DestroyEmulatorInstance DestroyEmulatorInstance = nullptr;

		typedef bool( __cdecl* PFN_Boot )( Emulator* emu, const wchar_t* filePath, const uint32_t resetVectorManual );
		PFN_Boot Boot = nullptr;

		typedef void( __cdecl* PFN_Shutdown )( Emulator* emu );
		PFN_Shutdown Shutdown;

		typedef int( __cdecl* PFN_RunEpoch )( Emulator* emu, const std::chrono::nanoseconds& runCycles );
		PFN_RunEpoch RunEpoch = nullptr;

		typedef void( __cdecl* PFN_GetFrameResult )( Emulator* emu, wtFrameResult& outFrameResult );
		PFN_GetFrameResult GetFrameResult;

		typedef uint32_t( __cdecl* PFN_ScreenWidth )( void );
		PFN_ScreenWidth ScreenWidth = nullptr;

		typedef uint32_t( __cdecl* PFN_ScreenHeight )( void );
		PFN_ScreenHeight ScreenHeight = nullptr;

		typedef uint32_t( __cdecl* PFN_SpriteLimit )( void );
		PFN_SpriteLimit SpriteLimit = nullptr;

		typedef Tomtendo::config_t( __cdecl* PFN_DefaultConfig )( );
		PFN_DefaultConfig DefaultConfig = nullptr;

		typedef void( __cdecl* PFN_SetConfig )( Emulator* emu, config_t& cfg );
		PFN_SetConfig SetConfig = nullptr;

		typedef void( __cdecl* PFN_SubmitCommand )( Emulator* emu, const sysCmd_t& cmd );
		PFN_SubmitCommand SubmitCommand = nullptr;

		typedef void( __cdecl* PFN_UpdateDebugImages )( Emulator* emu );
		PFN_UpdateDebugImages UpdateDebugImages = nullptr;

		typedef void( __cdecl* PFN_GenerateRomDissambly )( Emulator* emu, std::string prgRomAsm[ 128 ] );
		PFN_GenerateRomDissambly GenerateRomDissambly = nullptr;

		typedef void( __cdecl* PFN_GenerateChrRomTables )( Emulator* emu, wtPatternTableImage chrRom[ 32 ] );
		PFN_GenerateChrRomTables GenerateChrRomTables = nullptr;

		typedef void( __cdecl* PFN_GetKeyBuffer )( const Input* input, const ControllerId controllerId );
		PFN_GetKeyBuffer GetKeyBuffer = nullptr;

		typedef void( __cdecl* PFN_GetMouse )( const Input* input );
		PFN_GetMouse GetMouse = nullptr;

		typedef void( __cdecl* PFN_BindKey )( Input* input, const char key, const ControllerId controllerId, const ButtonFlags button );
		PFN_BindKey BindKey = nullptr;

		typedef void( __cdecl* PFN_StoreKey )( Input* input, const uint32_t key );
		PFN_StoreKey StoreKey = nullptr;

		typedef void( __cdecl* PFN_ReleaseKey )( Input* input, const uint32_t key );
		PFN_ReleaseKey ReleaseKey = nullptr;

		typedef void( __cdecl* PFN_StoreMouseClick )( Input* input, const int32_t x, const int32_t y );
		PFN_StoreMouseClick StoreMouseClick = nullptr;

		typedef void( __cdecl* PFN_ClearMouseClick )( Input* input );
		PFN_ClearMouseClick ClearMouseClick = nullptr;
	};

	void LoadDllInterface( runtimeDllInterface_t* dllInterface, HINSTANCE libInstance )
	{
		assert( dllInterface != nullptr );
		assert( libInstance != nullptr );

		if ( ( dllInterface == nullptr ) || ( libInstance == nullptr ) ) {
			return;
		}

		// Global
		RuntimeImportDllFunction( libInstance, dllInterface, DefaultConfig );
		RuntimeImportDllFunction( libInstance, dllInterface, ScreenWidth );
		RuntimeImportDllFunction( libInstance, dllInterface, ScreenHeight );
		RuntimeImportDllFunction( libInstance, dllInterface, SpriteLimit );

		// Init
		RuntimeImportDllFunction( libInstance, dllInterface, CreateEmulatorInstance );
		RuntimeImportDllFunction( libInstance, dllInterface, DestroyEmulatorInstance );

		// Interface 
		RuntimeImportDllFunction( libInstance, dllInterface, Boot );
		RuntimeImportDllFunction( libInstance, dllInterface, Shutdown );
		RuntimeImportDllFunction( libInstance, dllInterface, RunEpoch );
		RuntimeImportDllFunction( libInstance, dllInterface, GetFrameResult );	
		RuntimeImportDllFunction( libInstance, dllInterface, SetConfig );
		RuntimeImportDllFunction( libInstance, dllInterface, SubmitCommand );
		RuntimeImportDllFunction( libInstance, dllInterface, UpdateDebugImages );
		RuntimeImportDllFunction( libInstance, dllInterface, GenerateRomDissambly );
		RuntimeImportDllFunction( libInstance, dllInterface, GenerateChrRomTables );

		// Input
		RuntimeImportDllFunction( libInstance, dllInterface, GetKeyBuffer );
		RuntimeImportDllFunction( libInstance, dllInterface, GetMouse );
		RuntimeImportDllFunction( libInstance, dllInterface, BindKey );
		RuntimeImportDllFunction( libInstance, dllInterface, StoreKey );
		RuntimeImportDllFunction( libInstance, dllInterface, ReleaseKey );
		RuntimeImportDllFunction( libInstance, dllInterface, StoreMouseClick );
		RuntimeImportDllFunction( libInstance, dllInterface, ClearMouseClick );
	}
#endif

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

	inline uint32_t operator&( emulationFlags_t lhs, emulationFlags_t rhs )
	{
		return ( static_cast<uint32_t>( lhs ) & static_cast<uint32_t>( rhs ) );
	}

	struct EXPORT_CLASS_DLL config_t
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

	EXPORT_DLL config_t DefaultConfig();

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