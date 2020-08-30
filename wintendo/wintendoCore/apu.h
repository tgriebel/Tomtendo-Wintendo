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

static const uint8_t pulseWaves[4] = { 0x40, 0x60, 0x78, 0x9F };

static const uint32_t ApuSamplesPerSec = 44100;
static const uint32_t ApuBufferSeconds = 10;
static const uint32_t ApuBufferSize = ApuBufferSeconds * ApuSamplesPerSec;

struct wtApuOutput
{
	uint8_t soundBuffer[ApuBufferSize];
	float samples[ApuBufferSize];
	float frequency;
	float period;
	uint32_t currentIndex;
};

struct wtApuDebug
{
	PulseCtrl		pulseCtrl1;
	PulseRamp		pulseRamp1;
	TimerCtrl		pulseTune1;
	PulseCtrl		pulseCtrl2;
	PulseRamp		pulseRamp2;
	TimerCtrl		pulseTune2;
	TriangleCtrl	triangleCtrl1;
	TriangleCtrl	triangleCtrl2;
	TimerCtrl		triangleFreq;
	NoiseCtrl		noiseCtrl;
	NoiseFreq1		noiseFreq1;
	NoiseFreq2		noiseFreq2;
	DmcCtrl			dmcCtrl;
	DmcLoad			dmcLoad;
	uint8_t			dmcAddr;
	uint8_t			dmcLength;
};

struct APU
{
	static const uint32_t SoundBufferCnt = 2;

	PulseCtrl		pulseCtrl1;
	PulseRamp		pulseRamp1;
	TimerCtrl		pulseTune1;
	PulseCtrl		pulseCtrl2;
	PulseRamp		pulseRamp2;
	TimerCtrl		pulseTune2;
	TriangleCtrl	triangleCtrl1;
	TriangleCtrl	triangleCtrl2;
	TimerCtrl		triangleFreq;
	NoiseCtrl		noiseCtrl;
	NoiseFreq1		noiseFreq1;
	NoiseFreq2		noiseFreq2;
	DmcCtrl			dmcCtrl;
	DmcLoad			dmcLoad;
	uint8_t			dmcAddr;
	uint8_t			dmcLength;
	// TODO: 0x4015
	// TODO: 0x4017

	apuCycle_t		cycle;
	TimerCtrl		pulseTimer1;
	TimerCtrl		pulseTimer2;

	wtApuOutput		soundOutputBuffers[2];
	wtApuOutput*	soundOutput;
	wtApuOutput*	finishedSoundOutput;
	uint32_t		currentBuffer;

	wtSystem*		system;

	APU()
	{
		cycle = apuCycle_t(0);
		pulseCtrl1.raw = 0;
		pulseRamp1.raw = 0;
		pulseTune1.raw = 0;
		pulseTimer1.raw = 0;
		pulseCtrl2.raw = 0;
		pulseRamp2.raw = 0;
		pulseTune2.raw = 0;
		pulseTimer2.raw = 0;
		triangleCtrl1.raw = 0;
		triangleCtrl2.raw = 0;
		triangleFreq.raw = 0;
		noiseCtrl.raw = 0;
		noiseFreq1.raw = 0;
		noiseFreq2.raw = 0;
		dmcCtrl.raw = 0;
		dmcLoad.raw = 0;
		dmcAddr = 0;
		dmcLength = 0;
		
		currentBuffer = 0;
		soundOutput = &soundOutputBuffers[0];
		finishedSoundOutput = nullptr;

		for( uint32_t i = 0; i < 2; ++i )
		{
			soundOutputBuffers[i].frequency = 0.0f;
			soundOutputBuffers[i].period = 0.0f;
			soundOutputBuffers[i].currentIndex = 0;
			memset( soundOutputBuffers[i].soundBuffer, 0, sizeof( soundOutputBuffers[i].soundBuffer[0] ) * ApuBufferSize );
		}

		system = nullptr;
	}

	void Begin();
	void End();
	const apuCycle_t Exec();
	bool Step( const apuCycle_t& nextCycle );
	void WriteReg( const uint16_t addr, const uint8_t value );

	float GetPulseFrequency();
	float GetPulsePeriod();
	wtApuDebug GetDebugInfo();
private:
	float PulseMixer( const float pulse1, const float pulse2 );
	void GeneratePulseSamples( const uint8_t dutyCycle );
};