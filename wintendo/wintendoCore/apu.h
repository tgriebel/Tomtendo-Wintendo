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


enum pulseChannel_t : uint8_t
{
	PULSE_1 = 1,
	PULSE_2 = 2,
};


struct PulseChannel
{
	PulseCtrl	ctrl;
	PulseRamp	ramp;
	TimerCtrl	tune;
	TimerCtrl	timer;
	uint8_t		waveForm[15000]; // TODO: buffer size
	uint8_t		waveFormIx;
	apuCycle_t	lastCycle;
	uint8_t		sequenceStep;

	void Clear()
	{
		lastCycle = apuCycle_t( 0 );
		ctrl.raw = 0;
		ramp.raw = 0;
		tune.raw = 0;
		timer.raw = 0;
		waveFormIx = 0;
		memset( waveForm, 0, 15000 );
	}
};


struct TriangleChannel
{
	TriangleCtrl	ctrl1;
	TriangleCtrl	ctrl2;
	TimerCtrl		freq;
	TimerCtrl		timer;

	void Clear()
	{
		ctrl1.raw = 0;
		ctrl2.raw = 0;
		freq.raw = 0;
	}
};


struct NoiseChannel
{
	NoiseCtrl	ctrl;
	NoiseFreq1	freq1;
	NoiseFreq2	freq2;

	void Clear()
	{
		ctrl.raw = 0;
		freq1.raw = 0;
		freq2.raw = 0;
	}
};


struct DmcChannel
{
	DmcCtrl	ctrl;
	DmcLoad	load;
	uint8_t	addr;
	uint8_t	length;

	void Clear()
	{
		ctrl.raw = 0;
		load.raw = 0;
		addr = 0;
		length = 0;
	}
};


// https://wiki.nesdev.com/w/index.php/APU_Pulse
static const uint8_t pulseWaves[4][8] = 
{
//	{ 1, 0, 0, 0, 0, 0, 0, 0 },
//	{ 1, 1, 0, 0, 0, 0, 0, 0 },
//	{ 1, 1, 1, 1, 0, 0, 0, 0 },
//	{ 0, 0, 0, 0, 0, 0, 1, 1 },
//
	{ 0, 0, 0, 0, 0, 0, 0, 1 },
	{ 0, 0, 0, 0, 0, 0, 1, 1 },
	{ 0, 0, 0, 0, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 0, 0 },
//
//	{ 0, 1, 0, 0, 0, 0, 0, 0 },
//	{ 0, 1, 1, 0, 0, 0, 0, 0 },
//	{ 0, 1, 1, 1, 1, 0, 0, 0 },
//	{ 1, 0, 0, 1, 1, 1, 1, 1 },
};

static const uint32_t ApuSamplesPerSec = CPU_HZ;
static const uint32_t ApuBufferSeconds = 5;
static const uint32_t ApuBufferSize = ApuBufferSeconds * ApuSamplesPerSec;

struct wtSoundBuffer
{
	float samples[ApuBufferSize];
	float avgFrequency;
	float hz;
	float avgPeriod;
	uint32_t currentIndex;

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

	uint8_t			frameSeqStep;

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

		frameSeqStep = 0;
		frameCounter.raw = 0;
		currentBuffer = 0;
		soundOutput = &soundOutputBuffers[0];
		finishedSoundOutput = nullptr;

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
	float PulseMixer( const float pulse1, const float pulse2 );
	void GeneratePulseSamples( PulseChannel* pulse, wtSoundBuffer* buffer );
	void GeneratePulseSamples2( PulseChannel* pulse, wtSoundBuffer* buffer );
};