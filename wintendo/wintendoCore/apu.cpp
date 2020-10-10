#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"
#include <algorithm>

float APU::GetPulseFrequency( PulseChannel* pulse )
{
	float freq = CPU_HZ / ( 16.0f * pulse->regTune.sem0.timer + 1 );
	freq *= system->config.apu.frequencyScale;
	return freq;
}


float APU::GetPulsePeriod( PulseChannel* pulse )
{
	return ( 1.0f / system->config.apu.frequencyScale ) * ( pulse->regTune.sem0.timer );
}


void APU::WriteReg( const uint16_t addr, const uint8_t value )
{
	switch( addr )
	{
		case 0x4000:	pulse1.regCtrl.raw				= value;	break;
		case 0x4001:	pulse1.regRamp.raw				= value;	break;
		case 0x4002:	pulse1.regTune.sem1.lower		= value;	break;

		case 0x4003:
		{
			pulse1.regTune.sem1.upper = value;
			pulse1.sequenceStep = 0;
			pulse1.envelope.startFlag = true;
		} break;

		case 0x4004:	pulse2.regCtrl.raw				= value;	break;
		case 0x4005:	pulse2.regRamp.raw				= value;	break;
		case 0x4006:	pulse2.regTune.sem1.lower		= value;	break;

		case 0x4007:
		{
			pulse2.regTune.sem1.upper = value;
			pulse2.sequenceStep = 0;
			pulse2.envelope.startFlag = true;
		} break;

		case 0x4008:	triangle.regCtrl1.raw		= value;	break;
		case 0x4009:	triangle.regCtrl2.raw		= value;	break;
		case 0x400A:	triangle.regFreq.sem1.lower	= value;	break;
		case 0x400B:	triangle.regFreq.sem1.upper	= value;	break;
		case 0x400C:	noise.regCtrl.raw			= value;	break;
		case 0x400E:	noise.regFreq1.raw			= value;	break;
		case 0x400F:	noise.regFreq2.raw			= value;	break;
		case 0x4010:	dmc.regCtrl.raw				= value;	break;
		case 0x4011:	dmc.regLoad.raw				= value;	break;
		case 0x4012:	dmc.regAddr					= value;	break;
		case 0x4013:	dmc.regLength				= value;	break;
		// TODO: 0x4015
		case 0x4017:
		{
			// TODO: Takes effect after 3/4 cycles
			// https://wiki.nesdev.com/w/index.php/APU - Frame Counter
			// https://wiki.nesdev.com/w/index.php/APU_Frame_Counter
			frameCounter.raw = value;
			frameSeqStep = 0;
			if( frameCounter.sem.mode )
			{
				// Trigger half and quarter clocks
			}
		} break;
		default: break;
	}
}


void APU::EnvelopeGenerater( Envelope& envelope, const uint8_t volume, const bool loop )
{
	if( !quarterClk )
		return;

	if( envelope.startFlag )
	{
		envelope.startFlag = false;
		envelope.decayLevel = 15;
		envelope.divCounter = envelope.divPeriod;
	}
	else
	{
		if( envelope.divCounter == 0 )
		{
			envelope.divPeriod = volume;
			envelope.divCounter = ( envelope.divCounter - 1 ) & 0x0F; // TODO: wrap allowed?
			if( loop )
			{
				envelope.decayLevel = 15;
			}
		}
		else
		{
			envelope.divCounter = ( envelope.divCounter - 1 ) & 0x0F; // TODO: wrap allowed?
		}
	}
}


void APU::PulseSequencer( PulseChannel* pulse, wtSoundBuffer* buffer )
{
	const int samples = ( apuCycle - pulse->lastCycle ).count();
	float volume = 1.0f;

	if ( pulse->regCtrl.sem.isConstant )
	{
		volume = pulse->regCtrl.sem.volume;
	}
	else
	{
		volume = pulse->envelope.decayLevel;
	}
	assert( volume >= 0 && volume <= 15 );

	if( pulse->regTune.sem0.timer < 8 )
	{
		volume = 0;
	}

	const float amplitude = volume;

	for ( int sample = 0; sample < samples; sample++ )
	{
		const float pulseSample = pulseWaves[pulse->regCtrl.sem.duty][pulse->sequenceStep] ? -amplitude : amplitude;
		if ( pulse->timer.sem0.counter == 0 )
		{
			buffer->Write( 0.0f );
		}
		else
		{
			buffer->Write( pulseSample );
		}
	}

	pulse->lastCycle = apuCycle;
	pulse->sequenceStep = ( pulse->sequenceStep + 1 ) % 8;
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

	EnvelopeGenerater( pulse->envelope, pulse->regCtrl.sem.volume, pulse->regCtrl.sem.counterHalt );

	pulse->timer.sem0.timer--;
	if ( !pulse->regCtrl.sem.counterHalt )
	{
		pulse->timer.sem0.counter--;
	}
	
	if ( pulse->timer.sem0.timer == 0 )
	{
		buffer->avgFrequency = GetPulseFrequency( pulse );
		buffer->avgPeriod = pulse->regTune.sem0.timer;
		pulse->timer.sem0.timer = pulse->regTune.sem0.timer;
		PulseSequencer( pulse, buffer );
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


void APU::ExecFrameCounter()
{
	halfClk		= false;
	quarterClk	= false;
	irqClk		= false;

	const uint32_t mode = frameCounter.sem.mode;
	FrameSeqEvent& event = FrameSeqEvents[frameSeqStep][mode];
	if( frameSeqTick == event.cycle )
	{
		halfClk		= event.clkHalf;
		quarterClk	= event.clkQuarter;
		irqClk		= ( event.irq && !frameCounter.sem.interrupt );
		++frameSeqStep;
	}

	if ( frameSeqStep >= 6 )
	{
		frameSeqStep = 0;
		frameSeqTick = 0;
	}
}


bool APU::Step( const cpuCycle_t& nextCpuCycle )
{
	const apuCycle_t nextApuCycle = chrono::duration_cast<apuCycle_t>( nextCpuCycle );

	dbgStartCycle		= apuCycle;
	dbgTargetCycle		= nextApuCycle;
	dbgSysStartCycle	= chrono::duration_cast<masterCycles_t>( dbgStartCycle );
	dbgSysTargetCycle	= chrono::duration_cast<masterCycles_t>( dbgTargetCycle );

	while ( cpuCycle < nextCpuCycle )
	{
		ExecFrameCounter();
		ExecChannelTri();
		++cpuCycle;
		++frameSeqTick;
	}

	while ( apuCycle < nextApuCycle )
	{
		ExecPulseChannel( PULSE_1 );
		ExecPulseChannel( PULSE_2 );
		ExecChannelNoise();
		ExecChannelDMC();
		++apuTicks;
		++apuCycle;
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

	soundOutput->pulse1.Begin();
	soundOutput->pulse2.Begin();
	soundOutput->master.Begin();
}


void APU::End()
{
	// TODO: audio frame is not in sync with the frame sync
	const uint8_t sampleStride = 20;
	assert( sampleStride > 0 );
	const float masterVolume = system->config.apu.volume;

	soundOutput->pulse1.End();
	soundOutput->pulse2.End();

	const uint32_t sampleStart = min( soundOutput->pulse1.GetFrameBegin(), soundOutput->pulse2.GetFrameBegin() );
	const uint32_t sampleCnt = min( soundOutput->pulse1.GetFrameEnd(), soundOutput->pulse2.GetFrameEnd() );
	for( uint32_t i = sampleStart; i < sampleCnt; ++i )
	{
		const float pulse1Sample = soundOutput->pulse1.Read( i ); // TODO: use a 'ReadNext()' function
		const float pulse2Sample = soundOutput->pulse2.Read( i );
		const float mixedSample = PulseMixer( pulse1Sample, pulse2Sample );
		assert( pulse1Sample <= 15);
		assert( pulse2Sample <= 15 );
		assert( mixedSample < 0.3f );

		soundOutput->master.Write( mixedSample * 3.868f * 4096 );
		soundOutput->master.hz = 894886.0f;
	}

	soundOutput->master.End();

	static int frameCnt = 0;
	const int bufferedFrames = 4;
	++frameCnt;
	if ( frameCnt == bufferedFrames )
	{
		frameCnt = 0;

		finishedSoundOutput = &soundOutputBuffers[currentBuffer];
		currentBuffer = ( currentBuffer + 1 ) % SoundBufferCnt;
		soundOutput = &soundOutputBuffers[currentBuffer];
		soundOutput->pulse1.Clear();
		soundOutput->pulse2.Clear();
		soundOutput->master.Clear();
	}
}
