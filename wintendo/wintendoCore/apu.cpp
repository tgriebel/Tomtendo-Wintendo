#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"


float APU::GetPulseFrequency()
{
	const uint32_t cpuHz = cpuCycle_t( 1 ).count();
	float freq = cpuHz / ( 16.0f * pulseTune1.sem0.timer + 1 );
	freq *= system->config.apu.frequencyScale;
	return freq;
}


float APU::GetPulsePeriod()
{
	const uint32_t cpuHz = cpuCycle_t( 1 ).count();
	const float t = cpuHz / ( 16.0f * GetPulseFrequency() ) - 1;
	return t;
}


void APU::WriteReg( const uint16_t addr, const uint8_t value )
{
	switch( addr )
	{
		case 0x4000:	pulseCtrl1.raw			= value;	break;
		case 0x4001:	pulseRamp1.raw			= value;	break;
		case 0x4002:	pulseTune1.sem1.lower	= value;	break;
		case 0x4003:	pulseTune1.sem1.upper	= value;	break;
		case 0x4004:	pulseCtrl2.raw			= value;	break;
		case 0x4005:	pulseRamp2.raw			= value;	break;
		case 0x4006:	pulseTune2.sem1.lower	= value;	break;
		case 0x4007:	pulseTune2.sem1.upper	= value;	break;
		case 0x4008:	triangleCtrl1.raw		= value;	break;
		case 0x4009:	triangleCtrl2.raw		= value;	break;
		case 0x400A:	triangleFreq.sem1.lower	= value;	break;
		case 0x400B:	triangleFreq.sem1.upper	= value;	break;
		case 0x400C:	noiseCtrl.raw			= value;	break;
		case 0x400E:	noiseFreq1.raw			= value;	break;
		case 0x400F:	noiseFreq2.raw			= value;	break;
		case 0x4010:	dmcCtrl.raw				= value;	break;
		case 0x4011:	dmcLoad.raw				= value;	break;
		case 0x4012:	dmcAddr					= value;	break;
		case 0x4013:	dmcLength				= value;	break;
		default: break;
	}
}


const apuCycle_t APU::Exec()
{
	pulseTimer1.sem0.timer--;
	if( !pulseCtrl1.sem.counterHalt )
	{
		pulseTimer1.sem0.counter--;
	}

	if( pulseTimer1.sem0.timer == 0 )
	{
		pulseTimer1.sem0.timer = pulseTune1.sem0.timer;
	//	soundOutput->soundBuffer[soundOutput->currentIndex] = pulseWaves[pulseCtrl1.sem.duty];
	//	soundOutput->currentIndex++;
		GeneratePulseSamples( pulseWaves[pulseCtrl1.sem.duty] );
	}

	soundOutput->frequency = GetPulseFrequency();
	soundOutput->period = pulseTune1.sem0.timer;

	return static_cast<apuCycle_t>( 1 );
}


bool APU::Step( const apuCycle_t& nextCycle )
{
	while ( cycle < nextCycle )
	{
		cycle += Exec();
	}

	return true;
}


float APU::PulseMixer( const float pulse1, const float pulse2 )
{
	if( ( pulse1 + pulse2 ) == 0.0f )
		return 0.0f;
	// input [0-15]
	return ( 8128.0f / ( pulse1 + pulse2 ) ) + 100.0f;
}


void APU::GeneratePulseSamples( const uint8_t dutyCycle )
{
	const int samples = static_cast<int>( GetPulsePeriod() ); // TODO: rounding errors?
	const float sampleStep = GetPulsePeriod() / 8.0f;
	float volume = 1.0f;
	
	if( pulseCtrl1.sem.isConstant )
	{
		volume = ( pulseCtrl1.sem.volume / 15.0f );
	}

	if( pulseTimer1.sem0.timer == 0 )
	{
		volume = 0.0f;
	}

	const uint8_t baseVolume = 0xFF;
	float masterVolume = system->config.apu.volume;
	const float amplitude = volume * baseVolume;

	for ( int sample = 0; sample < samples; sample++ )
	{
		uint8_t mask = ( 0x80 >> static_cast<uint8_t>( sample / sampleStep ) );
		const float pulse1Sample = ( ( mask & dutyCycle ) == 0 ) ? -amplitude : amplitude;
		soundOutput->samples[soundOutput->currentIndex] = masterVolume * PulseMixer( pulse1Sample, 0.0f );
		soundOutput->currentIndex++;
		assert( soundOutput->currentIndex <= ApuBufferSize );
	}
}


wtApuDebug APU::GetDebugInfo()
{
	wtApuDebug apuDebug;
	apuDebug.pulseCtrl1		= pulseCtrl1;
	apuDebug.pulseRamp1		= pulseRamp1;
	apuDebug.pulseTune1		= pulseTune1;
	apuDebug.pulseCtrl2		= pulseCtrl2;
	apuDebug.pulseRamp2		= pulseRamp2;
	apuDebug.pulseTune2		= pulseTune2;
	apuDebug.triangleCtrl1	= triangleCtrl1;
	apuDebug.triangleCtrl2	= triangleCtrl2;
	apuDebug.triangleFreq	= triangleFreq;
	apuDebug.noiseCtrl		= noiseCtrl;
	apuDebug.noiseFreq1		= noiseFreq1;
	apuDebug.noiseFreq2		= noiseFreq2;
	apuDebug.dmcCtrl		= dmcCtrl;
	apuDebug.dmcLoad		= dmcLoad;
	apuDebug.dmcAddr		= dmcAddr;
	apuDebug.dmcLength		= dmcLength;
	return apuDebug;
}

void APU::Begin()
{

}


void APU::End()
{
	if( ( system->currentFrame % 1 ) == 0 ) // FIXME: current frame is for the frame buffer, this only works b/c it toggles on vsync
	{
		finishedSoundOutput = &soundOutputBuffers[currentBuffer];
		currentBuffer = ( currentBuffer + 1 ) % 2;
		soundOutput = &soundOutputBuffers[currentBuffer];
		soundOutput->currentIndex = 0;
	}
	else
	{
		finishedSoundOutput = nullptr;
	}
}
