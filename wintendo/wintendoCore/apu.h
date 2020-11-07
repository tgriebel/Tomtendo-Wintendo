#pragma once

#include "stdafx.h"
#include "common.h"

static const uint32_t ApuSamplesPerSec	= CPU_HZ;
static const uint32_t ApuBufferMs		= 250;
static const uint32_t ApuBufferSize		= ApuSamplesPerSec * ( ApuBufferMs / 1000.0f );

class wtSampleQueue
{
public:
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

private:
	float		samples[ApuBufferSize];
	float		hz;
	uint32_t	currentIndex;
};

union PulseCtrl
{
	struct PulseCtrlSemantic
	{
		uint8_t volume		: 4;
		uint8_t isConstant	: 1;
		uint8_t counterHalt : 1;
		uint8_t duty		: 2;
	} sem;

	uint8_t raw;
};

union PulseRamp
{
	struct PulseRampSemantic
	{
		uint8_t shift	: 3;
		uint8_t negate	: 1;
		uint8_t period	: 3;
		uint8_t enabled	: 1;
	} sem;

	uint8_t raw;
};

union TimerCtrl
{
	struct TimerCtrlSemantic0
	{
		uint16_t timer		: 11;
		uint16_t counter	: 5;
	} sem0;

	struct TimerCtrlSemantic1
	{
		uint16_t lower : 8;
		uint16_t upper : 8;
	} sem1;

	uint16_t raw;
};

union TriangleCtrl
{
	struct TriangleCtrlSemantic
	{
		uint8_t counterLoad : 7;
		uint8_t counterHalt : 1;
	} sem;

	uint8_t raw;
};


union NoiseCtrl
{
	struct NoiseCtrlSemantic
	{
		uint8_t volume		: 4;
		uint8_t isConstant	: 1;
		uint8_t counterHalt	: 1;
		uint8_t unused	: 2;
	} sem;

	uint8_t raw;
};


union NoiseFreq1
{
	struct NoiseFreq1Semantic
	{
		uint8_t period	: 4;
		uint8_t unused	: 3;
		uint8_t loop	: 1;
	} sem;

	uint8_t raw;
};


union NoiseFreq2
{
	struct NoiseFreq2Semantic
	{
		uint8_t unused : 3;
		uint8_t length : 5;
	} sem;

	uint8_t raw;
};


union DmcCtrl
{
	struct DmcCtrlSemantic
	{
		uint8_t freq		: 4;
		uint8_t unused		: 2;
		uint8_t loop		: 1;
		uint8_t irqEnable	: 1;
	} sem;

	uint8_t raw;
};


union DmcLoad
{
	struct DmcLoadSemantic
	{
		uint8_t counter	: 7;
		uint8_t unused	: 1;
	} sem;

	uint8_t raw;
};


template < uint16_t B >
union Counter
{
	uint16_t count	: B;
	uint16_t unused	: ( 16 - B );
};


struct FrameCounter
{
	struct FrameCounterSemantic
	{
		uint8_t unused		: 6;
		uint8_t interrupt	: 1;
		uint8_t mode		: 1;
	} sem;

	uint8_t raw;
};


struct Envelope
{
	bool	startFlag;
	uint8_t	divider;
	uint8_t	decayLevel;
	uint8_t	divPeriod;
	uint8_t	divCounter;
	uint8_t	output;
};


struct Sweep
{
	bool		reloadFlag;
	Counter<11>	divider;
};


enum pulseChannel_t : uint8_t
{
	PULSE_1 = 1,
	PULSE_2 = 2,
};

class wtSampleQueue;
struct PulseChannel
{
	// TODO: record timing signals as graphs
	pulseChannel_t	channelNum;

	PulseCtrl		regCtrl;
	PulseRamp		regRamp;
	TimerCtrl		regTune;
	TimerCtrl		timer;
	Counter<11>		period;
	apuCycle_t		lastCycle;
	uint8_t			sequenceStep;
	Envelope		envelope;
	Sweep			sweep;
	uint32_t		volume;
	wtSampleQueue	samples;

	void Clear()
	{
		lastCycle			= apuCycle_t( 0 );
		regCtrl.raw			= 0;
		regRamp.raw			= 0;
		regTune.raw			= 0;
		timer.raw			= 0;
		envelope.decayLevel	= 0;
		envelope.divCounter = 0;
		envelope.divider	= 0;
		envelope.divPeriod	= 0;
		envelope.startFlag	= false;
		sweep.divider.count	= 0;
		sweep.reloadFlag	= false;
		samples.Reset();
	}
};


struct TriangleChannel
{
	TriangleCtrl	regCtrl1;
	TriangleCtrl	regCtrl2;
	TimerCtrl		regFreq;
	TimerCtrl		timer;

	void Clear()
	{
		regCtrl1.raw	= 0;
		regCtrl2.raw	= 0;
		regFreq.raw		= 0;
	}
};


struct NoiseChannel
{
	NoiseCtrl	regCtrl;
	NoiseFreq1	regFreq1;
	NoiseFreq2	regFreq2;

	void Clear()
	{
		regCtrl.raw		= 0;
		regFreq1.raw	= 0;
		regFreq2.raw	= 0;
	}
};


struct DmcChannel
{
	DmcCtrl	regCtrl;
	DmcLoad	regLoad;
	uint8_t	regAddr;
	uint8_t	regLength;

	void Clear()
	{
		regCtrl.raw	= 0;
		regLoad.raw	= 0;
		regAddr		= 0;
		regLength	= 0;
	}
};


struct FrameSeqEvent
{
	uint32_t	cycle;
	bool		clkQuarter;
	bool		clkHalf;
	bool		irq;
};

static const uint32_t FrameSeqEventCnt	= 6;
static const uint32_t FrameSeqModeCnt	= 2;

static FrameSeqEvent FrameSeqEvents[FrameSeqEventCnt][FrameSeqModeCnt] =
{
	FrameSeqEvent{ 7457,	true,	false,	false },	FrameSeqEvent{ 7457,	true,	false,	false },
	FrameSeqEvent{ 14913,	true,	true,	false },	FrameSeqEvent{ 14913,	true,	true,	false },
	FrameSeqEvent{ 22371,	true,	false,	false },	FrameSeqEvent{ 22371,	true,	false,	false },
	FrameSeqEvent{ 29828,	false,	false,	true },		FrameSeqEvent{ 29829,	false,	false,	false },
	FrameSeqEvent{ 29829,	true,	true,	true },		FrameSeqEvent{ 37281,	true,	true,	false },
	FrameSeqEvent{ 29830,	false,	false,	true },		FrameSeqEvent{ 37282,	false,	false,	false },
};


// https://wiki.nesdev.com/w/index.php/APU_Pulse
static const uint8_t PulseLUT[4][8] = 
{
#if 0
	{ 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 1, 1 },
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


static const uint8_t SawLUT[32] =
{
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,	0x09, 0x08,
	0x07, 0x06, 0x05, 0x04,	0x03, 0x02, 0x01, 0x00,
	0x00, 0x01,	0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};


struct wtApuOutput
{
	wtSoundBuffer	dbgPulse1;
	wtSoundBuffer	dbgPulse2;
	wtSampleQueue	master;
	bool			locked;
};


struct wtApuDebug
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
	FrameCounter	frameCounter;

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

	float			squareLUT[SquareLutEntries];
	float			tndLUT[TndLutEntries];

public:
	// TODO: make this stuff private
	cpuCycle_t		cpuCycle;
	apuCycle_t		apuCycle;
	apuSeqCycle_t	seqCycle;

	wtApuOutput*	frameOutput;
	wtApuOutput		soundOutputBuffers[SoundBufferCnt];
	wtApuOutput*	soundOutput;
	uint32_t		currentBuffer;
	wtSystem*		system;

	APU()
	{
		Reset();
		InitMixerLUT();
	}

	void Reset()
	{
		cpuCycle = cpuCycle_t( 0 );
		pulse1.Clear();
		pulse2.Clear();
		triangle.Clear();
		noise.Clear();
		dmc.Clear();

		pulse1.channelNum = PULSE_1;
		pulse2.channelNum = PULSE_2;

		frameSeqStep = 0;
		frameCounter.raw = 0;
		currentBuffer = 0;
		soundOutput = &soundOutputBuffers[0];

		halfClk = false;
		quarterClk = false;
		irqClk = false;

		frameSeqTick = 0;
		frameOutput = nullptr;

		for ( uint32_t i = 0; i < SoundBufferCnt; ++i )
		{
			soundOutputBuffers[i].master.Reset();
			soundOutputBuffers[i].dbgPulse1.Clear();
			soundOutputBuffers[i].dbgPulse2.Clear();
		}

		system = nullptr;
	}

	void	Begin();
	void	End();
	bool	Step( const cpuCycle_t& nextCpuCycle );
	void	WriteReg( const uint16_t addr, const uint8_t value );

	float	GetPulseFrequency( PulseChannel* pulse );
	float	GetPulsePeriod( PulseChannel* pulse );
	void	GetDebugInfo( wtApuDebug& apuDebug );
private:
	void	ExecPulseChannel( PulseChannel* pulse );
	void	ExecChannelTri();
	void	ExecChannelNoise();
	void	ExecChannelDMC();
	void	ExecFrameCounter();
	void	EnvelopeGenerater( Envelope& envelope, const uint8_t volume, const bool loop, const bool constant );
	bool	IsPulseDutyHigh( PulseChannel* pulse );
	void	PulseSweep( PulseChannel* pulse );
	void	PulseSequencer( PulseChannel* pulse );
	void	InitMixerLUT();
	float	PulseMixer( const uint32_t pulse1, const uint32_t pulse2 );
};