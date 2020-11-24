#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"
#include <algorithm>

float APU::GetPulseFrequency( PulseChannel& pulse )
{
	float freq = CPU_HZ / ( 16.0f * pulse.period.Value() + 1 );
	freq *= system->config.apu.frequencyScale;
	return freq;
}


float APU::GetPulsePeriod( PulseChannel& pulse )
{
	return ( 1.0f / system->config.apu.frequencyScale ) * ( pulse.period.Value() );
}


void APU::WriteReg( const uint16_t addr, const uint8_t value )
{
	switch( addr )
	{
		case 0x4000:	pulse1.regCtrl.byte				= value;	break;
		case 0x4001:	pulse1.regRamp.byte				= value;	break;
		case 0x4002:	pulse1.regTune.sem1.lower		= value;	break;
		case 0x4004:	pulse2.regCtrl.byte				= value;	break;
		case 0x4005:	pulse2.regRamp.byte				= value;	break;
		case 0x4006:	pulse2.regTune.sem1.lower		= value;	break;
		case 0x4008:	triangle.regLinear.byte			= value;	break;
		case 0x400A:	triangle.regTimer.sem1.lower	= value;	break;
		case 0x400C:	noise.regCtrl.byte				= value;	break;
		case 0x4010:	dmc.regCtrl.byte					= value;	break;
		case 0x4011:	dmc.regLoad.byte					= value;	break;
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
			triangle.lengthCounter.Reload( LengthLUT[ triangle.regTimer.sem0.counter ] );
			triangle.reloadFlag = true;
		} break;

		case 0x400E:
		{
			noise.regFreq1.byte = value;
			noise.timer.Reload( NoiseLUT[ NTSC ][ noise.regFreq1.sem.period ] );
		} break;

		case 0x400F:
		{
			noise.regFreq2.byte = value;
			noise.lengthCounter.Reload( LengthLUT[ noise.regFreq2.sem.length ] );
		} break;

		case 0x4015:
		{
			regStatus.byte = value;

			pulse1.mute		= !regStatus.sem.p1;
			pulse2.mute		= !regStatus.sem.p2;
			triangle.mute	= !regStatus.sem.t;
			noise.mute		= !regStatus.sem.n;
			dmc.mute		= !regStatus.sem.d;

			if ( pulse1.mute )
			{
				pulse1.timer.sem0.counter = 0;
			}

			if ( pulse2.mute )
			{
				pulse2.timer.sem0.counter = 0;
			}

			if( triangle.mute )
			{
				triangle.lengthCounter.Reload();
			}

			if ( noise.mute )
			{
				noise.lengthCounter.Reload();
			}

			if ( dmc.mute )
			{
			}

			dmc.irq = false;

		} break;

		case 0x4017:
		{
			// TODO: Takes effect after 3/4 cycles
			// https://wiki.nesdev.com/w/index.php/APU - Frame Counter
			// https://wiki.nesdev.com/w/index.php/APU_Frame_Counter
			frameCounter.byte = value;
			frameSeqStep = 0;
			if( frameCounter.sem.mode )
			{
				// Trigger half and quarter clocks
			}
		} break;
		default: break;
	}
}


uint8_t APU::ReadReg( const uint16_t addr )
{
	if( addr != wtSystem::ApuRegisterStatus ) {
		return 0;
	}

	// TODO:
	// Status ($4015) -- https://wiki.nesdev.com/w/index.php/APU
	//	N / T / 2 / 1 will read as 1 if the corresponding length counter is greater than 0. For the triangle channel, the status of the linear counter is irrelevant.
	//	D will read as 1 if the DMC bytes remaining is more than 0.
	//	Reading this register clears the frame interrupt flag( but not the DMC interrupt flag ).
	//	If an interrupt flag was set at the same moment of the read, it will read back as 1 but it will not be cleared.

	apuStatus_t result;
	result.sem.p1 = ( pulse1.timer.sem0.counter > 0 );
	result.sem.p2 = ( pulse2.timer.sem0.counter > 0 );
	result.sem.n = !noise.lengthCounter.IsZero();

	return result.byte;
}


void APU::EnvelopeGenerater( envelope_t& envelope, const uint8_t volume, const bool loop, const bool constant )
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


void APU::PulseSweep( PulseChannel& pulse )
{
	// TODO: Whenever the current period changes for any reason, whether by $400x writes or by sweep, the target period also changes.
	// https://wiki.nesdev.com/w/index.php/APU_Sweep
	if( system->config.apu.disableSweep )
	{
		pulse.period.Reload( pulse.regTune.sem0.timer );
		return;
	}

	if ( !halfClk )
		return;

	const bool		enabled	= pulse.regRamp.sem.enabled;
	const bool		negate	= pulse.regRamp.sem.negate;
	const uint8_t	period	= pulse.regRamp.sem.period;
	const uint8_t	shift	= pulse.regRamp.sem.shift;
	const bool		mute	= false; // TODO: implement
	const bool		isZero	= pulse.sweep.divider.IsZero();

	bool& reload = pulse.sweep.reloadFlag;
	sweep_t& sweep = pulse.sweep;

	if( isZero && enabled && !mute )
	{
		uint16_t change = pulse.regTune.sem0.timer >> shift;
		change *= negate ? -1 : 1;
		change -= ( pulse.channelNum == PULSE_1 ) ? 1 : 0;

		pulse.period.Reload( pulse.regTune.sem0.timer + change );
	}
	else
	{
		pulse.period.Reload( pulse.regTune.sem0.timer );
	}

	if ( isZero || reload )
	{
		sweep.divider.Reload( period );
		reload = false;
	}
	else
	{
		sweep.divider.Dec();
	}
}


bool APU::IsPulseDutyHigh( const PulseChannel& pulse )
{
	return PulseLUT[ pulse.regCtrl.sem.duty ][ ( pulse.sequenceStep + system->config.apu.waveShift ) % 8 ];
}


void APU::PulseSequencer( PulseChannel& pulse )
{
	const int sampleCnt = ( apuCycle - pulse.lastCycle ).count();
	float volume = pulse.envelope.output;

	if( pulse.period.Value() < 8 )
	{
		volume = 0;
	}

	const float amplitude = volume;

	for ( int sample = 0; sample < sampleCnt; sample++ )
	{
		const float pulseSample = IsPulseDutyHigh( pulse ) ? amplitude : 0;
		if ( ( pulse.timer.sem0.counter == 0 ) || pulse.mute )
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
	EnvelopeGenerater( pulse.envelope, pulse.regCtrl.sem.volume, pulse.regCtrl.sem.counterHalt, pulse.regCtrl.sem.isConstant );

	pulse.timer.sem0.timer--;
	if ( halfClk && !pulse.regCtrl.sem.counterHalt )
	{
		pulse.timer.sem0.counter--;
	}
	
	if ( pulse.timer.sem0.timer == 0 )
	{
		pulse.timer.sem0.timer = pulse.period.Value();
		PulseSequencer( pulse );

		// pulse->volume = pulse->regCtrl.sem.volume;
	}
}


void APU::TriSequencer()
{
	const int sampleCnt = ( cpuCycle - triangle.lastCycle ).count();

	bool isSeqHalted = false;
	isSeqHalted |= triangle.lengthCounter.IsZero();
	isSeqHalted |= triangle.linearCounter.IsZero();

	for ( int sample = 0; sample < sampleCnt; sample++ )
	{
		if( triangle.mute )
		{
			triangle.samples.Enque( 0.0f );
		}
		else
		{
			triangle.samples.Enque( TriLUT[ triangle.sequenceStep ] );
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
			triangle.linearCounter.Reload( triangle.regLinear.sem.counterLoad );
		}
		else if ( !triangle.linearCounter.IsZero() )
		{
			triangle.linearCounter.Dec();
		}

		if ( !triangle.lengthCounter.IsZero() && !triangle.regLinear.sem.counterHalt )
		{
			triangle.lengthCounter.Dec();
		}

		if( !triangle.regLinear.sem.counterHalt )
		{
			triangle.reloadFlag = false;
		}
	}

	if( triangle.regLinear.sem.counterHalt )
	{
		triangle.lengthCounter.Reload();
	}

	triangle.timer.Dec();
	if ( triangle.timer.IsZero() )
	{
		TriSequencer();
		triangle.timer.Reload( 1 + triangle.regTimer.sem0.timer );
	}
}


void APU::NoiseGenerator()
{
	const uint16_t shiftValue = noise.shift.Value();
	const uint16_t bitMask = noise.regFreq1.sem.mode ? BIT_MASK_6 : BIT_MASK_1;
	const uint16_t bitShift = noise.regFreq1.sem.mode ? BIT_6 : BIT_1;
	const uint16_t feedback = ( shiftValue & 0x01 ) ^ ( ( shiftValue & bitMask ) >> bitShift );
	const uint16_t newShift = ( shiftValue >> 1 ) | ( feedback << BIT_14 );

	noise.shift.Reload( newShift );

	const uint8_t volume = noise.envelope.output;

	const int sampleCnt = ( apuCycle - noise.lastCycle ).count();
	for ( int sample = 0; sample < sampleCnt; sample++ )
	{
		if ( noise.lengthCounter.IsZero() || ( noise.shift.Value() & 0x01 ) || noise.mute )
		{
			noise.samples.Enque( 0.0f );
		}
		else
		{
			noise.samples.Enque( volume );
		}
	}

	noise.lastCycle = apuCycle;
}


void APU::ExecChannelNoise()
{
	EnvelopeGenerater( noise.envelope, noise.regCtrl.sem.volume, noise.regCtrl.sem.counterHalt, noise.regCtrl.sem.isConstant );

	if ( quarterClk )
	{
		if ( !noise.lengthCounter.IsZero() && !noise.regCtrl.sem.counterHalt )
		{
			noise.lengthCounter.Dec();
		}
	}

	noise.timer.Dec();
	if ( noise.timer.IsZero() )
	{
		NoiseGenerator();
		noise.timer.Reload( NoiseLUT[NTSC][ noise.regFreq1.sem.period ] );
	}
}


void APU::DmcGenerator()
{
	const int sampleCnt = ( apuCycle - dmc.lastCycle ).count();
	for ( int sample = 0; sample < sampleCnt; sample++ )
	{
		dmc.samples.Enque( 0.0f );
	}

	dmc.lastCycle = apuCycle;
}


void APU::ExecChannelDMC()
{
	// if cpuCycle % dmc_table[r]
	// then generate
	// Sample address = % 11AAAAAA.AA000000 = $C000 + ( A * 64 )
	// ( 0xC000 | ( address << 6 ) ) & 0xFFC0
	// Sample length = %LLLL.LLLL0001 = (L * 16) + 1 bytes
	// ( 0x01 | ( address << 4 ) )
}


void APU::ExecFrameCounter()
{
	halfClk		= false;
	quarterClk	= false;
	irqClk		= false;

	const uint32_t mode = frameCounter.sem.mode;
	frameSeqEvent_t& event = FrameSeqEvents[frameSeqStep][mode];
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


void APU::GetDebugInfo( apuDebug_t& apuDebug )
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


bool APU::HasAllChannelSamples()
{
	bool hasSamples = true;
	hasSamples = hasSamples && !pulse1.samples.IsEmpty();
	hasSamples = hasSamples && !pulse2.samples.IsEmpty();
	hasSamples = hasSamples && ( triangle.samples.GetSampleCnt() > 1 );
	hasSamples = hasSamples && !noise.samples.IsEmpty();
	// hasSamples = hasSamples && !dmc.samples.IsEmpty();

	return hasSamples;
}


void APU::End()
{
	while( HasAllChannelSamples() )
	{
		// This doubles up samples from channels other than the triangle channel (which runs at 2x frequency)
		float pulse1Sample			= pulse1.samples.Deque();
		float pulse2Sample			= pulse2.samples.Deque();
		float triSample1			= triangle.samples.Deque();
		float triSample2			= triangle.samples.Deque();
		float noiseSample			= noise.samples.Deque();
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