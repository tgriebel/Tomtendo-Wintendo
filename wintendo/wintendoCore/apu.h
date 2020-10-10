#pragma once

#include "stdafx.h"
#include "common.h"

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
};


struct Sweep
{
	bool	reloadFlag;
	uint8_t	divider;
};


enum pulseChannel_t : uint8_t
{
	PULSE_1 = 1,
	PULSE_2 = 2,
};


struct PulseChannel
{
	pulseChannel_t	channelNum;

	PulseCtrl	regCtrl;
	PulseRamp	regRamp;
	TimerCtrl	regTune;
	TimerCtrl	timer;
	apuCycle_t	lastCycle;
	uint8_t		sequenceStep;
	Envelope	envelope;

	void Clear()
	{
		lastCycle = apuCycle_t( 0 );
		regCtrl.raw = 0;
		regRamp.raw = 0;
		regTune.raw = 0;
		timer.raw = 0;
		envelope.decayLevel = 0;
		envelope.divCounter = 0;
		envelope.divider = 0;
		envelope.divPeriod = 0;
		envelope.startFlag = false;
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
		regCtrl1.raw = 0;
		regCtrl2.raw = 0;
		regFreq.raw = 0;
	}
};


struct NoiseChannel
{
	NoiseCtrl	regCtrl;
	NoiseFreq1	regFreq1;
	NoiseFreq2	regFreq2;

	void Clear()
	{
		regCtrl.raw = 0;
		regFreq1.raw = 0;
		regFreq2.raw = 0;
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
		regCtrl.raw = 0;
		regLoad.raw = 0;
		regAddr = 0;
		regLength = 0;
	}
};


struct FrameSeqEvent
{
	uint32_t	cycle;
	bool		clkQuarter;
	bool		clkHalf;
	bool		irq;
};

static const uint32_t FrameSeqEventCnt = 6;
static const uint32_t FrameSeqModeCnt = 2;

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
static const uint8_t pulseWaves[4][8] = 
{
	{ 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 1, 1 },
//
//	{ 0, 0, 0, 0, 0, 0, 0, 1 },
//	{ 0, 0, 0, 0, 0, 0, 1, 1 },
//	{ 0, 0, 0, 0, 1, 1, 1, 1 },
//	{ 1, 1, 1, 1, 1, 1, 0, 0 },
//
//	{ 0, 1, 0, 0, 0, 0, 0, 0 },
//	{ 0, 1, 1, 0, 0, 0, 0, 0 },
//	{ 0, 1, 1, 1, 1, 0, 0, 0 },
//	{ 1, 0, 0, 1, 1, 1, 1, 1 },
};

static const uint32_t ApuSamplesPerSec	= CPU_HZ;
static const uint32_t ApuBufferMs		= 250;
static const uint32_t ApuBufferSize		= ApuSamplesPerSec * ( 1000.0f / ApuBufferMs );

struct wtSoundBuffer
{
	float samples[ApuBufferSize];
	float avgFrequency;
	float hz;
	float avgPeriod;
	uint32_t currentIndex;
	uint32_t frameBegin;
	uint32_t frameEnd;

	void Begin()
	{
		frameBegin = currentIndex;
	}

	void End()
	{
		frameEnd = currentIndex;
	}

	uint32_t GetFrameBegin()
	{
		return frameBegin;
	}

	uint32_t GetFrameEnd()
	{
		return frameEnd;
	}

	void Clear()
	{
		Reset();
		memset( samples, 0, sizeof( ApuBufferSize * samples[0] ) );
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

	float Read( const uint32_t index )
	{
		assert( index <= ApuBufferSize );
		return samples[index];
	}

	uint32_t GetSampleCnt()
	{
		return currentIndex;
	}
};


struct wtApuOutput
{
	wtSoundBuffer pulse1;
	wtSoundBuffer pulse2;
	wtSoundBuffer master;
};


struct wtApuDebug
{
	PulseChannel	pulse1;
	PulseChannel	pulse2;
	TriangleChannel	triangle;
	NoiseChannel	noise;
	DmcChannel		dmc;
};


struct APU
{
	static const uint32_t SoundBufferCnt = 3;

	PulseChannel	pulse1;
	PulseChannel	pulse2;
	TriangleChannel	triangle;
	NoiseChannel	noise;
	DmcChannel		dmc;
	FrameCounter	frameCounter;

	cpuCycle_t		cpuCycle;
	apuCycle_t		apuCycle;
	apuSeqCycle_t	seqCycle;

	uint32_t		frameSeqTick;
	uint8_t			frameSeqStep;

	bool			halfClk;
	bool			quarterClk;
	bool			irqClk;

	wtApuOutput		soundOutputBuffers[SoundBufferCnt];
	wtApuOutput*	soundOutput;
	wtApuOutput*	finishedSoundOutput;
	uint32_t		currentBuffer;

	apuCycle_t		dbgStartCycle;
	apuCycle_t		dbgTargetCycle;
	masterCycles_t	dbgSysStartCycle;
	masterCycles_t	dbgSysTargetCycle;

	uint32_t		startSoundIndex;
	uint32_t		apuTicks;

	wtSystem*		system;

	APU()
	{
		cpuCycle = apuCycle_t(0);
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
		finishedSoundOutput = nullptr;

		halfClk = false;
		quarterClk = false;
		irqClk = false;

		for( uint32_t i = 0; i < SoundBufferCnt; ++i )
		{
			soundOutputBuffers[i].master.Clear();
			soundOutputBuffers[i].pulse1.Clear();
			soundOutputBuffers[i].pulse2.Clear();
		}

		system = nullptr;
	}

	void Begin();
	void End();
	bool Step( const cpuCycle_t& nextCpuCycle );
	void WriteReg( const uint16_t addr, const uint8_t value );

	float GetPulseFrequency( PulseChannel* pulse );
	float GetPulsePeriod( PulseChannel* pulse );
	wtApuDebug GetDebugInfo();
private:
	void ExecPulseChannel( const pulseChannel_t channel );
	void ExecChannelTri();
	void ExecChannelNoise();
	void ExecChannelDMC();
	void ExecFrameCounter();	
	void EnvelopeGenerater( Envelope& envelope, const uint8_t volume, const bool loop );
	void PulseSequencer( PulseChannel* pulse, wtSoundBuffer* buffer );
	float PulseMixer( const float pulse1, const float pulse2 );
};