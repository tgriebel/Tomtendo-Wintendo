#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"


float APU::GetPulseFrequency()
{
	float freq = CPU_HZ / ( 16.0f * pulse1.tune.sem0.timer + 1 );
	freq *= system->config.apu.frequencyScale;
	return freq;
}


float APU::GetPulsePeriod()
{
	return ( 1.0f / system->config.apu.frequencyScale ) * ( pulse1.tune.sem0.timer );
}


void APU::WriteReg( const uint16_t addr, const uint8_t value )
{
	switch( addr )
	{
		case 0x4000:	pulse1.ctrl.raw				= value;	break;
		case 0x4001:	pulse1.ramp.raw				= value;	break;
		case 0x4002:	pulse1.tune.sem1.lower		= value;	break;
		case 0x4003:	pulse1.tune.sem1.upper		= value;	break;
		case 0x4004:	pulse2.ctrl.raw				= value;	break;
		case 0x4005:	pulse2.ramp.raw				= value;	break;
		case 0x4006:	pulse2.tune.sem1.lower		= value;	break;
		case 0x4007:	pulse2.tune.sem1.upper		= value;	break;
		case 0x4008:	triangle.ctrl1.raw			= value;	break;
		case 0x4009:	triangle.ctrl2.raw			= value;	break;
		case 0x400A:	triangle.freq.sem1.lower	= value;	break;
		case 0x400B:	triangle.freq.sem1.upper	= value;	break;
		case 0x400C:	noise.ctrl.raw				= value;	break;
		case 0x400E:	noise.freq1.raw				= value;	break;
		case 0x400F:	noise.freq2.raw				= value;	break;
		case 0x4010:	dmc.ctrl.raw				= value;	break;
		case 0x4011:	dmc.load.raw				= value;	break;
		case 0x4012:	dmc.addr					= value;	break;
		case 0x4013:	dmc.length					= value;	break;
		// TODO: 0x4015
		case 0x4017:	frameCounter.raw			= value;	break;			
		default: break;
	}
}


void APU::ExecPulseChannel( const pulseChannel_t channel )
{
	PulseChannel* pulse;
	wtSoundBuffer* buffer;
	if( channel == PULSE_1 )
	{
		pulse = &pulse1;
		buffer = &soundOutput->pulse1;
	}
	else
	{
		pulse = &pulse2;
		buffer = &soundOutput->pulse2;
	}

	pulse->timer.sem0.timer--;
	if ( !pulse->ctrl.sem.counterHalt )
	{
		pulse->timer.sem0.counter--;
	}

	if ( pulse->timer.sem0.timer == 0 )
	{
		buffer->frequency = GetPulseFrequency();
		buffer->period = pulse->tune.sem0.timer;
		pulse->timer.sem0.timer = pulse->tune.sem0.timer;
		GeneratePulseSamples( pulse, buffer, pulseWaves[pulse->ctrl.sem.duty] );
	}
}

void APU::ExecChannelTri()
{

}


void APU::ExecChannelNoise()
{

}


void APU::ExecChannelDMC()
{

}


bool APU::Step( const cpuCycle_t& nextCpuCycle )
{
	const apuCycle_t nextApuCycle = chrono::duration_cast<apuCycle_t>( nextCpuCycle );

	dbgStartCycle		= apuCycle;
	dbgTargetCycle		= nextApuCycle;
	dbgSysStartCycle	= chrono::duration_cast<masterCycles_t>( dbgStartCycle );
	dbgSysTargetCycle	= chrono::duration_cast<masterCycles_t>( dbgTargetCycle );

	while ( apuCycle < nextApuCycle )
	{
		ExecPulseChannel( PULSE_1 );
		ExecPulseChannel( PULSE_2 );
		ExecChannelNoise();
		ExecChannelDMC();
		++apuTicks;
		++apuCycle;
	}

	while ( cpuCycle < nextCpuCycle )
	{
		ExecChannelTri();
		++cpuCycle;
	}

	return true;
}


float APU::PulseMixer( const float pulse1, const float pulse2 )
{
	if( ( pulse1 + pulse2 ) == 0.0f )
		return 0.0f;
	// input [0-15]
	return 95.88f / ( ( 8128.0f / ( pulse1 + pulse2 ) ) + 100.0f );
}


void APU::GeneratePulseSamples( PulseChannel* pulse, wtSoundBuffer * buffer, const uint8_t waveForm[8] )
{
	const int samples = static_cast<int>( GetPulsePeriod() ); // TODO: rounding errors?
	const float sampleStep = samples / 8.0f;
	float volume = 1.0f;

	if( pulse->ctrl.sem.isConstant )
	{
		volume = pulse->ctrl.sem.volume;
	}

	const float amplitude = volume;

	for ( int sample = 0; sample < samples; sample++ )
	{
		uint8_t srcSample = static_cast<uint8_t>( sample / sampleStep );
		const float pulse1Sample = waveForm[srcSample] ? -amplitude : amplitude;
		if ( pulse1.timer.sem0.counter == 0 )
		{
			buffer->Write( 0.0f );
		}
		else
		{
			buffer->Write( pulse1Sample );
		}
	}
}


wtApuDebug APU::GetDebugInfo()
{
	wtApuDebug apuDebug;
	apuDebug.pulse1		= pulse1;
	apuDebug.pulse2		= pulse2;
	apuDebug.triangle	= triangle;
	apuDebug.noise		= noise;
	apuDebug.dmc		= dmc;
	return apuDebug;
}

void APU::Begin()
{
	apuTicks = 0;
	startSoundIndex = soundOutput->pulse1.currentIndex;
}


void APU::End()
{
	if( ( system->currentFrame % 60 ) == 0 ) // FIXME: current frame is for the frame buffer, this only works b/c it toggles on vsync
	{
		const float masterVolume = 10000.0f * system->config.apu.volume;
		for ( uint32_t i = 0; i < soundOutput->pulse1.currentIndex; ++i )
		{
			const float pulse1Sample = soundOutput->pulse1.Read( i );
			const float pulse2Sample = soundOutput->pulse2.Read( i );
			soundOutput->master.Write( masterVolume * PulseMixer( pulse1Sample, pulse2Sample ) );
		}

		finishedSoundOutput = &soundOutputBuffers[currentBuffer];
		currentBuffer = ( currentBuffer + 1 ) % SoundBufferCnt;
		soundOutput = &soundOutputBuffers[currentBuffer];
		soundOutput->pulse1.Clear();
		soundOutput->pulse2.Clear();
		soundOutput->master.Clear();
	}
	else
	{
		finishedSoundOutput = nullptr;
	}
}
