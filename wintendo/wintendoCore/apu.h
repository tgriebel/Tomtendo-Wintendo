#pragma once

#include "stdafx.h"
#include "common.h"
#include "serializer.h"

static const uint32_t		ApuSamplesPerSec	= static_cast<uint32_t>( CPU_HZ );
static constexpr uint32_t	ApuBufferMs			= static_cast<uint32_t>( 1000.0f / MinFPS );
static constexpr uint32_t	ApuBufferSize		= static_cast<uint32_t>( ApuSamplesPerSec *  ( ApuBufferMs / 1000.0f ) );

class wtSampleQueue
{
public:
	wtSampleQueue()
	{
		Reset();
	}

	void Enque( const float sample )
	{
		if ( IsFull() )
			return;

		if ( begin == -1 )
		{
			begin = 0;
		}

		end = ( end + 1 ) % ApuBufferSize;
		samples[end] = sample;
	}

	float Deque()
	{
		if ( IsEmpty() )
			return 0.0f;

		const float retValue = samples[begin];
		samples[begin] = 0;
		if ( begin == end )
		{
			begin = -1;
			end = -1;
		}
		else
		{
			begin = ( begin + 1 ) % ApuBufferSize;
		}

		return retValue;
	}

	bool IsEmpty() const
	{
		return ( GetSampleCnt() == 0 );
	}

	bool IsFull()
	{
		return ( GetSampleCnt() == ( ApuBufferSize - 1 ) );
	}

	uint32_t GetSampleCnt() const
	{
		if ( ( begin == -1 ) || ( end == -1 ) )
		{
			return 0;
		}
		else if ( end >= begin )
		{
			return ( ( end - begin ) + 1 );
		}
		else
		{
			return ( ApuBufferSize - ( begin - end ) + 1 );
		}
	}

	void SetHz( const float _hz )
	{
		hz = _hz;
	}

	float GetHz() const
	{
		return hz;
	}

	void Reset()
	{
		begin = -1;
		end = -1;
	}

private:
	float	samples[ApuBufferSize];
	float	hz;
	int32_t	begin;
	int32_t	end;
};


class wtSoundBuffer
{
public:
	void Clear()
	{
		Reset();
		for( uint32_t i = 0; i < ApuBufferSize; ++i )
		{
			samples[i] = 0;
		}
	}

	void Reset()
	{
		currentIndex = 0;
		memset( samples, 0xFF, sizeof( samples[0] ) * ApuBufferSize );
	}

	void Write( const float sample )
	{
		samples[currentIndex] = sample;
		currentIndex++;
		assert( currentIndex <= ApuBufferSize );
	}

	void SetHz( const float _hz )
	{
		hz = _hz;
	}

	float GetHz() const
	{
		return hz;
	}

	float Read( const uint32_t index ) const
	{
		assert( index <= ApuBufferSize );
		return samples[index];
	}

	const float* GetRawBuffer()
	{
		return &samples[0];
	}

	uint32_t GetSampleCnt() const
	{
		return currentIndex;
	}

	bool IsFull() const
	{
		return ( currentIndex == ApuBufferSize );
	}

private:
	float		samples[ApuBufferSize];
	float		hz;
	uint32_t	currentIndex;
};

union pulseCtrl_t
{
	struct semantic
	{
		uint8_t volume		: 4;
		uint8_t isConstant	: 1;
		uint8_t counterHalt : 1;
		uint8_t duty		: 2;
	} sem;

	uint8_t byte;
};

union pulseRamp_t
{
	struct semantic
	{
		uint8_t shift	: 3;
		uint8_t negate	: 1;
		uint8_t period	: 3;
		uint8_t enabled	: 1;
	} sem;

	uint8_t byte;
};

union timerCtrl_t
{
	struct semantic0_t
	{
		uint16_t timer		: 11;
		uint16_t counter	: 5;
	} sem0;

	struct semantic1_t
	{
		uint16_t lower		: 8;
		uint16_t upper		: 8;
	} sem1;

	uint16_t byte2x;
};

union triangleLinear_t
{
	struct semantic
	{
		uint8_t counterLoad : 7;
		uint8_t counterHalt : 1;
	} sem;

	uint8_t byte;
};

union noiseCtrl_t
{
	struct semantic
	{
		uint8_t volume		: 4;
		uint8_t isConstant	: 1;
		uint8_t counterHalt	: 1;
		uint8_t unused		: 2;
	} sem;

	uint8_t byte;
};


union noiseFreq_t
{
	struct semantic
	{
		uint8_t period	: 4;
		uint8_t unused	: 3;
		uint8_t mode	: 1;
	} sem;

	uint8_t byte;
};


union noiseLength_t
{
	struct semantic
	{
		uint8_t unused : 3;
		uint8_t length : 5;
	} sem;

	uint8_t byte;
};


union dmcCtrl_t
{
	struct semantic
	{
		uint8_t freq		: 4;
		uint8_t unused		: 2;
		uint8_t loop		: 1;
		uint8_t irqEnable	: 1;
	} sem;

	uint8_t byte;
};


union dmcLoad_t
{
	struct semantic
	{
		uint8_t counter	: 7;
		uint8_t unused	: 1;
	} sem;

	uint8_t byte;
};


union apuStatus_t
{
	struct semantic
	{
		uint8_t p1		: 1;
		uint8_t p2		: 1;
		uint8_t t		: 1;
		uint8_t n		: 1;
		uint8_t d		: 1;
		uint8_t unused	: 1;
		uint8_t frInt	: 1;
		uint8_t dmcInt	: 1;
	} sem;

	uint8_t byte;
};


union frameCounter_t
{
	struct semantic
	{
		uint8_t unused		: 6;
		uint8_t interrupt	: 1;
		uint8_t mode		: 1;
	} sem;

	uint8_t byte;
};


struct envelope_t
{
	bool	startFlag;
	uint8_t	decayLevel;
	uint8_t	divCounter;
	uint8_t	output;
};


struct sweep_t
{
	bool			mute;
	bool			reloadFlag;
	BitCounter<11>	divider;
	uint16_t		period;
};


enum pulseChannel_t : uint8_t
{
	PULSE_1 = 1,
	PULSE_2 = 2,
};


class PulseChannel
{
public:
	// TODO: record timing signals as graphs
	pulseChannel_t		channelNum;

	pulseCtrl_t			regCtrl;
	pulseRamp_t			regRamp;
	timerCtrl_t			regTune;
	timerCtrl_t			timer;
	BitCounter<11>		period;
	apuCycle_t			lastCycle;
	uint8_t				sequenceStep;
	envelope_t			envelope;
	sweep_t				sweep;
	uint32_t			volume;
	wtSampleQueue		samples;
	bool				mute;

	void Clear()
	{
		lastCycle				= apuCycle_t( 0 );
		regCtrl.byte			= 0;
		regRamp.byte			= 0;
		regTune.byte2x			= 0;
		timer.byte2x			= 0;

		sequenceStep			= 0;
		volume					= 0;

		mute					= true;
		sweep.reloadFlag		= true;
		sweep.mute				= false;
		
		sweep.divider.Reload();
		period.Reload();		
		samples.Reset();

		memset( &envelope, 0, sizeof( envelope ) );
		envelope.startFlag = true;
	}

	void Serialize( Serializer& serializer, const serializeMode_t mode );
};


class TriangleChannel
{
public:
	triangleLinear_t	regLinear;
	timerCtrl_t			regTimer;
	bool				reloadFlag;
	BitCounter<7>		linearCounter;
	BitCounter<7>		lengthCounter;
	BitCounter<11>		timer;
	uint8_t				sequenceStep;
	cpuCycle_t			lastCycle;
	wtSampleQueue		samples;
	bool				mute;

	void Clear()
	{
		regLinear.byte	= 0;
		regTimer.byte2x	= 0;
		reloadFlag		= false;
		sequenceStep	= 0;
		lastCycle		= cpuCycle_t( 0 );

		linearCounter.Reload();
		lengthCounter.Reload();
		timer.Reload();
		samples.Reset();

		mute = true;
	}

	void Serialize( Serializer& serializer, const serializeMode_t mode );
};


class NoiseChannel
{
public:
	noiseCtrl_t			regCtrl;
	noiseFreq_t			regFreq1;
	noiseLength_t		regFreq2;
	BitCounter<15>		shift;
	envelope_t			envelope;
	BitCounter<12>		timer; // TODO: how many bits?
	BitCounter<7>		lengthCounter; // TODO: how many bits?
	wtSampleQueue		samples;
	apuCycle_t			lastCycle;
	bool				mute;

	void Clear()
	{
		regCtrl.byte	= 0;
		regFreq1.byte	= 0;
		regFreq2.byte	= 0;

		shift.Reload( 1 );
		memset( &envelope, 0, sizeof( envelope ) );
		timer.Reload();
		lengthCounter.Reload();
		samples.Reset();
		lastCycle = apuCycle_t( 0 );

		mute = true;
	}

	void Serialize( Serializer& serializer, const serializeMode_t mode );
};


class DmcChannel
{
public:
	dmcCtrl_t			regCtrl;
	dmcLoad_t			regLoad;
	uint8_t				regAddr;
	uint8_t				regLength;

	wtSampleQueue		samples;
	apuCycle_t			lastCycle;
	bool				irq;
	bool				silenceFlag;
	bool				mute;

	uint16_t			addr;
	uint16_t			byteCnt;
	uint8_t				sampleBuffer;
	BitCounter<7>		outputLevel;
	bool				emptyBuffer;
	uint8_t				shiftReg;

	void Clear()
	{
		regCtrl.byte	= 0;
		regLoad.byte	= 0;
		regAddr			= 0;
		regLength		= 0;

		addr			= 0;
		byteCnt			= 0;
		sampleBuffer	= 0;
		emptyBuffer		= false;

		shiftReg		= 0;
		silenceFlag		= false;
		irq				= false;
		mute			= false;

		outputLevel.Reload();
		samples.Reset();
		lastCycle = apuCycle_t( 0 );
	}

	void Serialize( Serializer& serializer, const serializeMode_t mode );
};


struct frameSeqEvent_t
{
	uint32_t	cycle;
	bool		clkQuarter;
	bool		clkHalf;
	bool		irq;
};

static const uint32_t FrameSeqEventCnt	= 6;
static const uint32_t FrameSeqModeCnt	= 2;

static frameSeqEvent_t FrameSeqEvents[FrameSeqEventCnt][FrameSeqModeCnt] =
{
	frameSeqEvent_t{ 7457,	true,	false,	false },	frameSeqEvent_t{ 7457,	true,	false,	false },
	frameSeqEvent_t{ 14913,	true,	true,	false },	frameSeqEvent_t{ 14913,	true,	true,	false },
	frameSeqEvent_t{ 22371,	true,	false,	false },	frameSeqEvent_t{ 22371,	true,	false,	false },
	frameSeqEvent_t{ 29828,	false,	false,	true },		frameSeqEvent_t{ 29829,	false,	false,	false },
	frameSeqEvent_t{ 29829,	true,	true,	true },		frameSeqEvent_t{ 37281,	true,	true,	false },
	frameSeqEvent_t{ 29830,	false,	false,	true },		frameSeqEvent_t{ 37282,	false,	false,	false },
};


// https://wiki.nesdev.com/w/index.php/APU_Pulse
static const uint8_t PulseLUT[4][8] = 
{
#if 1
	{ 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 0, 0, 0, 0 },
	{ 0, 0, 1, 1, 1, 1, 1, 1 },
#elif 0
	{ 0, 0, 0, 0, 0, 0, 0, 1 },
	{ 0, 0, 0, 0, 0, 0, 1, 1 },
	{ 0, 0, 0, 0, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 0, 0 },
#else
	{ 0, 1, 0, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 1, 1, 0, 0, 0 },
	{ 1, 0, 0, 1, 1, 1, 1, 1 },
#endif
};


static const uint8_t TriLUT[32] =
{
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,	0x09, 0x08,
	0x07, 0x06, 0x05, 0x04,	0x03, 0x02, 0x01, 0x00,
	0x00, 0x01,	0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};


static const uint16_t LengthLUT[] =
{
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};


static const uint16_t NoiseLUT[ ANALOG_MODE_COUNT ][16] =
{
	{ 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068, }, // NTSC LUT
	{ 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778, }, // PAL LUT
};


static const uint16_t DmcLUT[ANALOG_MODE_COUNT][16] =
{
	{ 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54 }, // NTSC LUT
	{ 398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50 }, // PAL LUT
};


struct apuOutput_t
{
	wtSoundBuffer	dbgPulse1;
	wtSoundBuffer	dbgPulse2;
	wtSoundBuffer	dbgTri;
	wtSoundBuffer	dbgNoise;
	wtSoundBuffer	dbgDmc;
	wtSampleQueue	master;
};


struct apuDebug_t
{
	PulseChannel	pulse1;
	PulseChannel	pulse2;
	TriangleChannel	triangle;
	NoiseChannel	noise;
	DmcChannel		dmc;
};


class APU
{
public:
	static const uint32_t SoundBufferCnt	= 3;
	static const uint32_t SquareLutEntries	= 31;
	static const uint32_t TndLutEntries		= 203;

private:
	PulseChannel	pulse1;
	PulseChannel	pulse2;
	TriangleChannel	triangle;
	NoiseChannel	noise;
	DmcChannel		dmc;
	frameCounter_t	frameCounter;

	apuStatus_t		regStatus;

	uint32_t		frameSeqTick;
	uint8_t			frameSeqStep;

	bool			halfClk;
	bool			quarterClk;
	bool			irqClk;

	apuCycle_t		dbgStartCycle;
	apuCycle_t		dbgTargetCycle;
	masterCycles_t	dbgSysStartCycle;
	masterCycles_t	dbgSysTargetCycle;

	uint32_t		apuTicks;

	cpuCycle_t		cpuCycle;
	apuCycle_t		apuCycle;
	apuSeqCycle_t	seqCycle;

	float			squareLUT[SquareLutEntries];
	float			tndLUT[TndLutEntries];

	uint32_t		currentBuffer;
	apuOutput_t*	soundOutput;
	apuOutput_t		soundOutputBuffers[ SoundBufferCnt ];

public:
	// TODO: make 'frameOutput' private
	apuOutput_t*	frameOutput;
	wtSystem*		system;

	APU()
	{
		Reset();
		InitMixerLUT();
	}

	void Reset()
	{
		cpuCycle = cpuCycle_t( 0 );
		apuCycle = apuCycle_t( 0 );
		seqCycle = apuSeqCycle_t( 0 );
		
		pulse1.Clear();
		pulse2.Clear();
		triangle.Clear();
		noise.Clear();
		dmc.Clear();

		pulse1.channelNum	= PULSE_1;
		pulse2.channelNum	= PULSE_2;

		frameSeqStep		= 0;
		frameCounter.byte	= 0;
		currentBuffer		= 0;
		soundOutput			= &soundOutputBuffers[0];

		regStatus.byte		= 0x00;

		halfClk				= false;
		quarterClk			= false;
		irqClk				= false;

		frameSeqTick		= 0;
		frameOutput			= nullptr;

		for ( uint32_t i = 0; i < SoundBufferCnt; ++i )
		{
			soundOutputBuffers[i].master.Reset();
			soundOutputBuffers[i].dbgPulse1.Clear();
			soundOutputBuffers[i].dbgPulse2.Clear();
			soundOutputBuffers[i].dbgTri.Clear();
			soundOutputBuffers[i].dbgNoise.Clear();
			soundOutputBuffers[i].dbgDmc.Clear();
		}

		system = nullptr;
	}

	void	Begin();
	void	End();
	void	RegisterSystem( wtSystem* system );

	bool	Step( const cpuCycle_t& nextCpuCycle );
	void	WriteReg( const uint16_t addr, const uint8_t value );
	uint8_t	ReadReg( const uint16_t addr );

	float	GetPulseFrequency( PulseChannel& pulse );
	float	GetPulsePeriod( PulseChannel& pulse );
	void	GetDebugInfo( apuDebug_t& apuDebug );

	void	Serialize( Serializer& serializer, const serializeMode_t mode );

private:
	void	ExecPulseChannel( PulseChannel& pulse );
	void	ExecChannelTri();
	void	ExecChannelNoise();
	void	ExecChannelDMC();
	void	ExecFrameCounter();
	void	EnvelopeGenerater( envelope_t& envelope, const uint8_t volume, const bool loop, const bool constant );
	bool	IsDutyHigh( const PulseChannel& pulse );
	void	PulseSweep( PulseChannel& pulse );
	void	PulseSequencer( PulseChannel& pulse );
	void	InitMixerLUT();
	float	PulseMixer( const uint32_t pulse1, const uint32_t pulse2 );
	float	TndMixer( const uint32_t triangle, const uint32_t noise, const uint32_t dmc );
	void	TriSequencer();
	void	NoiseGenerator();
	void	DmcGenerator();
	bool	AllChannelHaveSamples();
};