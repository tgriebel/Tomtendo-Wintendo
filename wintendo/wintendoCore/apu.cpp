#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"
#include <algorithm>

float APU::GetPulseFrequency( PulseChannel* pulse )
{
	float freq = CPU_HZ / ( 16.0f * pulse->period.count + 1 );
	freq *= system->config.apu.frequencyScale;
	return freq;
}


float APU::GetPulsePeriod( PulseChannel* pulse )
{
	return ( 1.0f / system->config.apu.frequencyScale ) * ( pulse->period.count );
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


void APU::EnvelopeGenerater( Envelope& envelope, const uint8_t volume, const bool loop, const bool constant )
{
	if ( system->config.apu.disableEnvelope )
	{
		envelope.output = volume;
		return;
	}

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

	if ( constant )
	{
		envelope.output = volume;
	}
	else
	{
		envelope.output = envelope.decayLevel;
	}
	assert( volume >= 0 && volume <= 15 );
}


void APU::PulseSweep( PulseChannel* pulse )
{
	// TODO: Whenever the current period changes for any reason, whether by $400x writes or by sweep, the target period also changes.
	// https://wiki.nesdev.com/w/index.php/APU_Sweep
	if( system->config.apu.disableSweep )
	{
		pulse->period.count = pulse->regTune.sem0.timer;
		return;
	}

	if ( !halfClk )
		return;

	const bool		enabled	= pulse->regRamp.sem.enabled;
	const bool		negate	= pulse->regRamp.sem.negate;
	const uint8_t	period	= pulse->regRamp.sem.period;
	const uint8_t	shift	= pulse->regRamp.sem.shift;
	const bool		mute	= false; // TODO: implement
	const bool		isZero	= ( pulse->sweep.divider.count == 0 );

	bool& reload = pulse->sweep.reloadFlag;
	Sweep& sweep = pulse->sweep;

	if( isZero && enabled && !mute )
	{
		uint16_t change = pulse->regTune.sem0.timer >> shift;
		change *= negate ? -1 : 1;
		change -= ( pulse->channelNum == PULSE_1 ) ? 1 : 0;

		pulse->period.count = pulse->regTune.sem0.timer + change;
	}
	else
	{
		pulse->period.count = pulse->regTune.sem0.timer;
	}

	if ( isZero || reload )
	{
		sweep.divider.count = period;
		reload = false;
	}
	else
	{
		sweep.divider.count--;
	}
}


bool APU::IsPulseDutyHigh( PulseChannel* pulse )
{
	return PulseLUT[pulse->regCtrl.sem.duty][( pulse->sequenceStep + system->config.apu.waveShift ) % 8];
}


void APU::PulseSequencer( PulseChannel* pulse )
{
	const int samples = ( apuCycle - pulse->lastCycle ).count();
	float volume = pulse->envelope.output;

	if( pulse->period.count < 8 )
	{
		volume = 0;
	}

	const float amplitude = volume;

	for ( int sample = 0; sample < samples; sample++ )
	{
		const float pulseSample = IsPulseDutyHigh( pulse ) ? amplitude : 0;
		if ( pulse->timer.sem0.counter == 0 )
		{
			pulse->samples.Enque( 0 );
		}
		else
		{
			pulse->samples.Enque( pulseSample );
		}
	}

	pulse->lastCycle = apuCycle;
	pulse->sequenceStep = ( pulse->sequenceStep + 1 ) % 8;
}


void APU::ExecPulseChannel( PulseChannel* pulse )
{
	PulseSweep( pulse );
	EnvelopeGenerater( pulse->envelope, pulse->regCtrl.sem.volume, pulse->regCtrl.sem.counterHalt, pulse->regCtrl.sem.isConstant );

	pulse->timer.sem0.timer--;
	if ( halfClk && !pulse->regCtrl.sem.counterHalt )
	{
		pulse->timer.sem0.counter--;
	}
	
	if ( pulse->timer.sem0.timer == 0 )
	{
		pulse->timer.sem0.timer = pulse->period.count;
		PulseSequencer( pulse );

		// pulse->volume = pulse->regCtrl.sem.volume;
	}
}

void APU::ExecChannelTri()
{
	if( quarterClk )
	{
		
	}
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
		ExecPulseChannel( &pulse1 );
		ExecPulseChannel( &pulse2 );
		ExecChannelNoise();
		ExecChannelDMC();
		++apuTicks;
		++apuCycle;
	}

	return true;
}


void APU::InitMixerLUT()
{
	squareLUT[0] = 0.0f;
	for( uint32_t i = 1; i < SquareLutEntries; ++i )
	{
		squareLUT[i] = 95.52f / ( 8128.0f / i + 100.0f );
	}
}


float APU::PulseMixer( const uint32_t pulse1, const uint32_t pulse2 )
{
	const uint32_t pulseSum = ( pulse1 + pulse2 );
	assert( ( pulseSum <= 31 ) && ( pulseSum >= 0 ) );
	return squareLUT[pulseSum];
}


void APU::GetDebugInfo( wtApuDebug& apuDebug )
{
	apuDebug.pulse1		= pulse1;
	apuDebug.pulse2		= pulse2;
	apuDebug.triangle	= triangle;
	apuDebug.noise		= noise;
	apuDebug.dmc		= dmc;
}

void APU::Begin()
{
	apuTicks = 0;
}


void APU::End()
{
	const uint8_t sampleStride = 20;
	assert( sampleStride > 0 );
	const float masterVolume = system->config.apu.volume;
	
	while( !pulse1.samples.IsEmpty() && !pulse2.samples.IsEmpty() )
	{
		const float pulse1Sample = pulse1.samples.Deque();
		const float pulse2Sample = pulse2.samples.Deque();

		const float mixedSample = PulseMixer( static_cast<uint32_t>( pulse1Sample ), static_cast<uint32_t>( pulse2Sample ) );
		assert( mixedSample < 0.3f );

		soundOutput->dbgPulse1.Write( pulse1Sample );
		soundOutput->dbgPulse2.Write( pulse2Sample );
		soundOutput->master.Enque( floor( 65535.0f * mixedSample ) );
	}

	soundOutput->master.SetHz( 894886.0f );

	soundOutput->locked = true;
	frameOutput = soundOutput;

	currentBuffer = ( currentBuffer + 1 ) % SoundBufferCnt;
	soundOutput = &soundOutputBuffers[currentBuffer];
	assert( !soundOutput->locked );
	soundOutput->master.Reset();
	soundOutput->dbgPulse1.Clear();
	soundOutput->dbgPulse2.Clear();
}
