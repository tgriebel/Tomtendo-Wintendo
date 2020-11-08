#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"
#include <algorithm>

float APU::GetPulseFrequency( PulseChannel& pulse )
{
	float freq = CPU_HZ / ( 16.0f * pulse.period.count + 1 );
	freq *= system->config.apu.frequencyScale;
	return freq;
}


float APU::GetPulsePeriod( PulseChannel& pulse )
{
	return ( 1.0f / system->config.apu.frequencyScale ) * ( pulse.period.count );
}


void APU::WriteReg( const uint16_t addr, const uint8_t value )
{
	switch( addr )
	{
		case 0x4000:	pulse1.regCtrl.raw				= value;	break;
		case 0x4001:	pulse1.regRamp.raw				= value;	break;
		case 0x4002:	pulse1.regTune.sem1.lower		= value;	break;
		case 0x4004:	pulse2.regCtrl.raw				= value;	break;
		case 0x4005:	pulse2.regRamp.raw				= value;	break;
		case 0x4006:	pulse2.regTune.sem1.lower		= value;	break;
		case 0x4008:	triangle.regLinear.raw			= value;	break;
		case 0x400A:	triangle.regTimer.sem1.lower	= value;	break;
		case 0x400C:	noise.regCtrl.raw				= value;	break;
		case 0x400E:	noise.regFreq1.raw				= value;	break;
		case 0x400F:	noise.regFreq2.raw				= value;	break;
		case 0x4010:	dmc.regCtrl.raw					= value;	break;
		case 0x4011:	dmc.regLoad.raw					= value;	break;
		case 0x4012:	dmc.regAddr						= value;	break;
		case 0x4013:	dmc.regLength					= value;	break;

		case 0x4003:
		{
			pulse1.regTune.sem1.upper = value;
			pulse1.sequenceStep = 0;
			pulse1.envelope.startFlag = true;
			// TODO: Reload length counter?
		} break;

		case 0x4007:
		{
			pulse2.regTune.sem1.upper = value;
			pulse2.sequenceStep = 0;
			pulse2.envelope.startFlag = true;
		} break;

		case 0x400B:
		{
			triangle.regTimer.sem1.upper = value;
			triangle.lengthCounter.count = LengthLUT[value];
			triangle.reloadFlag = true;
		} break;

		case 0x4015:
		{
			regStatus.raw = value;

			if ( !regStatus.sem.p1 )
			{
				pulse1.timer.sem0.counter = 0;
			}

			if ( !regStatus.sem.p2 )
			{
				pulse2.timer.sem0.counter = 0;
			}

			if( !regStatus.sem.t )
			{
				triangle.lengthCounter.count = 0;
			}

		} break;

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


void APU::EnvelopeGenerater( PulseChannel& pulse )
{
	Envelope& envelope		= pulse.envelope;
	const uint8_t volume	= pulse.regCtrl.sem.volume;
	const bool loop			= pulse.regCtrl.sem.counterHalt;
	const bool constant		= pulse.regCtrl.sem.isConstant;

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


void APU::PulseSweep( PulseChannel& pulse )
{
	// TODO: Whenever the current period changes for any reason, whether by $400x writes or by sweep, the target period also changes.
	// https://wiki.nesdev.com/w/index.php/APU_Sweep
	if( system->config.apu.disableSweep )
	{
		pulse.period.count = pulse.regTune.sem0.timer;
		return;
	}

	if ( !halfClk )
		return;

	const bool		enabled	= pulse.regRamp.sem.enabled;
	const bool		negate	= pulse.regRamp.sem.negate;
	const uint8_t	period	= pulse.regRamp.sem.period;
	const uint8_t	shift	= pulse.regRamp.sem.shift;
	const bool		mute	= false; // TODO: implement
	const bool		isZero	= ( pulse.sweep.divider.count == 0 );

	bool& reload = pulse.sweep.reloadFlag;
	Sweep& sweep = pulse.sweep;

	if( isZero && enabled && !mute )
	{
		uint16_t change = pulse.regTune.sem0.timer >> shift;
		change *= negate ? -1 : 1;
		change -= ( pulse.channelNum == PULSE_1 ) ? 1 : 0;

		pulse.period.count = pulse.regTune.sem0.timer + change;
	}
	else
	{
		pulse.period.count = pulse.regTune.sem0.timer;
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


bool APU::IsPulseDutyHigh( const PulseChannel& pulse )
{
	return PulseLUT[ pulse.regCtrl.sem.duty ][ ( pulse.sequenceStep + system->config.apu.waveShift ) % 8 ];
}


void APU::PulseSequencer( PulseChannel& pulse )
{
	const int samples = ( apuCycle - pulse.lastCycle ).count();
	float volume = pulse.envelope.output;

	if( pulse.period.count < 8 )
	{
		volume = 0;
	}

	const float amplitude = volume;

	for ( int sample = 0; sample < samples; sample++ )
	{
		const float pulseSample = IsPulseDutyHigh( pulse ) ? amplitude : 0;
		if ( pulse.timer.sem0.counter == 0 )
		{
			pulse.samples.Enque( 0 );
		}
		else
		{
			pulse.samples.Enque( pulseSample );
		}
	}

	pulse.lastCycle = apuCycle;
	pulse.sequenceStep = ( pulse.sequenceStep + 1 ) % 8;
}


void APU::ExecPulseChannel( PulseChannel& pulse )
{
	PulseSweep( pulse );
	EnvelopeGenerater( pulse );

	pulse.timer.sem0.timer--;
	if ( halfClk && !pulse.regCtrl.sem.counterHalt )
	{
		pulse.timer.sem0.counter--;
	}
	
	if ( pulse.timer.sem0.timer == 0 )
	{
		pulse.timer.sem0.timer = pulse.period.count;
		PulseSequencer( pulse );

		// pulse->volume = pulse->regCtrl.sem.volume;
	}
}


void APU::TriSequencer()
{
	const int samples = ( cpuCycle - triangle.lastCycle ).count();

	bool isSeqHalted = false;
	isSeqHalted |= triangle.lengthCounter.count == 0;
	isSeqHalted |= triangle.linearCounter.count == 0;

	for ( int sample = 0; sample < samples; sample++ )
	{
		if( !regStatus.sem.t )
		{
			triangle.samples.Enque( 0.0f );
		}
		else
		{
			triangle.samples.Enque( SawLUT[ triangle.sequenceStep ] );
		}
	}

	triangle.lastCycle = cpuCycle;

	if( !isSeqHalted )
	{
		triangle.sequenceStep = ( triangle.sequenceStep + 1 ) % 32;
	}
}


void APU::ExecChannelTri()
{
	if( quarterClk )
	{
		if( triangle.reloadFlag )
		{
			triangle.linearCounter.count = triangle.regLinear.sem.counterLoad;
		}
		else if ( triangle.linearCounter.count > 0 )
		{
			triangle.linearCounter.count--;
		}

		if ( triangle.lengthCounter.count > 0 && !triangle.regLinear.sem.counterHalt )
		{
			triangle.lengthCounter.count--;
		}

		if( !triangle.regLinear.sem.counterHalt )
		{
			triangle.reloadFlag = false;
		}
	}

	if( triangle.regLinear.sem.counterHalt )
	{
		triangle.lengthCounter.count = 0;
	}

	triangle.timer.count--;
	if ( triangle.timer.count == 0 )
	{
		TriSequencer();
		triangle.timer.count = triangle.regTimer.sem0.timer;
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
		ExecPulseChannel( pulse1 );
		ExecPulseChannel( pulse2 );
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


float APU::TndMixer( const uint32_t triangle, const uint32_t noise, const uint32_t dmc )
{
	if( ( triangle + noise + dmc ) == 0.0f )
		return 0.0f;

	const float denom = 100.0f + ( 1.0f / ( ( triangle / 8227.0f ) + ( noise / 12241.0f ) + ( dmc / 22638.0f ) ) );
	return ( 159.79f / denom );
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
	while( !pulse1.samples.IsEmpty() && !pulse2.samples.IsEmpty() && ( triangle.samples.GetSampleCnt() > 1 ) )
	{
		// This doubles up samples from channels other than the triangle channel (which runs at 2x frequency)
		float pulse1Sample			= pulse1.samples.Deque();
		float pulse2Sample			= pulse2.samples.Deque();
		float triSample1			= triangle.samples.Deque();
		float triSample2			= triangle.samples.Deque();
		float noiseSample			= 0;
		float dmcSample				= 0;

		pulse1Sample				= ( system->config.apu.mutePulse1	)	? 0.0f : pulse1Sample;
		pulse2Sample				= ( system->config.apu.mutePulse2	)	? 0.0f : pulse2Sample;
		triSample1					= ( system->config.apu.muteTri		)	? 0.0f : triSample1;
		triSample2					= ( system->config.apu.muteTri		)	? 0.0f : triSample2;
		noiseSample					= ( system->config.apu.muteNoise	)	? 0.0f : noiseSample;
		dmcSample					= ( system->config.apu.muteDMC		)	? 0.0f : dmcSample;

		const float pulseMixed		= PulseMixer( (uint32_t)pulse1Sample, (uint32_t)pulse2Sample );
		const float tndMixed1		= TndMixer( (uint32_t)triSample1, (uint32_t)noiseSample, (uint32_t)dmcSample );
		const float tndMixed2		= TndMixer( (uint32_t)triSample2, (uint32_t)noiseSample, (uint32_t)dmcSample );
		const float mixedSample1	= pulseMixed + tndMixed1;
		const float mixedSample2	= pulseMixed + tndMixed2;

		assert( pulseMixed < 0.3f );

		soundOutput->dbgPulse1.Write( pulse1Sample );
		soundOutput->dbgPulse2.Write( pulse2Sample );
		soundOutput->dbgTri.Write( triSample1 );
		soundOutput->dbgTri.Write( triSample2 );
		soundOutput->dbgNoise.Write( noiseSample );
		soundOutput->dbgDmc.Write( dmcSample );
		soundOutput->master.Enque( floor( 65535.0f * mixedSample1 ) );
		soundOutput->master.Enque( floor( 65535.0f * mixedSample2 ) );
	}

	soundOutput->master.SetHz( CPU_HZ );

	soundOutput->dbgLocked = true;
	frameOutput = soundOutput;

	currentBuffer = ( currentBuffer + 1 ) % SoundBufferCnt;
	soundOutput = &soundOutputBuffers[currentBuffer];
	assert( !soundOutput->dbgLocked );

	soundOutput->master.Reset();
	soundOutput->dbgPulse1.Clear();
	soundOutput->dbgPulse2.Clear();
	soundOutput->dbgTri.Clear();
	soundOutput->dbgNoise.Clear();
	soundOutput->dbgDmc.Clear();
}