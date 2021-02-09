#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"
#include <algorithm>

//#pragma optimize("", off)

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
		case 0x4002:	pulse1.regTune.sem1.lower		= value;	break;
		case 0x4004:	pulse2.regCtrl.byte				= value;	break;
		case 0x4006:	pulse2.regTune.sem1.lower		= value;	break;
		case 0x4008:	triangle.regLinear.byte			= value;	break;
		case 0x400A:	triangle.regTimer.sem1.lower	= value;	break;
		case 0x400C:	noise.regCtrl.byte				= value;	break;
		case 0x4010:	dmc.regCtrl.byte				= value;	break;

		case 0x4001:
		{
			pulse1.regRamp.byte = value;
			pulse1.sweep.reloadFlag = true;
		} break;

		case 0x4003:
		{
			pulse1.regTune.sem1.upper = value;
			pulse1.sequenceStep = 0;
			pulse1.envelope.startFlag = true;
		} break;

		case 0x4005:
		{
			pulse2.regRamp.byte = value;
			pulse2.sweep.reloadFlag = true;
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
			noise.envelope.startFlag = true;
			noise.envelope.decayLevel = 0x0F;
			noise.regFreq2.byte = value;
			noise.lengthCounter.Reload( LengthLUT[ noise.regFreq2.sem.length ] );
		} break;

		case 0x4011:
		{
			dmc.regLoad.byte = value;
			dmc.outputLevel.Reload( dmc.regLoad.sem.counter );
			dmc.emptyBuffer = false;
		} break;

		case 0x4012:
		{
			dmc.regAddr = value;
			dmc.addr = ( 0xC000 | ( dmc.regAddr << 6 ) ) & 0xFFC0;
		} break;

		case 0x4013:
		{
			dmc.regLength = value;
			dmc.byteCnt = ( 0x01 | ( value << 4 ) );
		} break;

		case 0x4015:
		{
			regStatus.byte = value;

			pulse1.mute		= !regStatus.sem.p1;
			pulse2.mute		= !regStatus.sem.p2;
			triangle.mute	= !regStatus.sem.t;
			noise.mute		= !regStatus.sem.n;
			dmc.mute		= !regStatus.sem.d;

			if ( pulse1.mute ) {
				pulse1.timer.sem0.counter = 0;
			}

			if ( pulse2.mute ) {
				pulse2.timer.sem0.counter = 0;
			}

			if( triangle.mute ) {
				triangle.lengthCounter.Reload();
			}

			if ( noise.mute ) {
				noise.lengthCounter.Reload();
			}
			
			if ( dmc.mute ) {
				dmc.byteCnt = 0;
			}
			// If the DMC bit is set, the DMC sample will be restarted only if its bytes remaining is 0.
			// If there are bits remaining in the 1-byte sample buffer, these will finish playing before the next sample is fetched.

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
				// TODO: check correctness
				quarterClk = true;
				halfClk = true;
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

	irqClk = false;

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
		envelope.decayLevel = 0x0F;
		envelope.divCounter = volume + 1;
		envelope.startFlag = false;
	}
	else
	{
		if( ( --envelope.divCounter ) == 0 )
		{
			envelope.divCounter = volume + 1;

			if( ( envelope.decayLevel > 0 ) || loop ) {
				--envelope.decayLevel;
			}
		}
	}

	assert( envelope.divCounter <= 0x10 );
	assert( envelope.decayLevel <= 0x0F );

	if ( constant ) {
		envelope.output = volume;
	} else {
		envelope.output = envelope.decayLevel;
	}
	assert( volume >= 0x00 && volume <= 0x0F );
}


void APU::PulseSweep( PulseChannel& pulse )
{
	if ( !halfClk )
		return;

	pulse.period.Reload( pulse.regTune.sem0.timer );

	if ( system->config.apu.disableSweep ) {
		return;
	}

	const bool		enabled	= pulse.regRamp.sem.enabled;
	const bool		negate	= pulse.regRamp.sem.negate;
	const uint8_t	period	= pulse.regRamp.sem.period;
	const uint8_t	shift	= pulse.regRamp.sem.shift;

	bool& reload = pulse.sweep.reloadFlag;
	sweep_t& sweep = pulse.sweep;

	sweep.divider.Dec();

	const bool isZero = pulse.sweep.divider.IsZero();

	if ( isZero ) {
		sweep.divider.Reload( period + 1 );
	}

	uint16_t targetPeriod = pulse.period.Value();

	if ( isZero && enabled && ( shift > 0 ) && ( pulse.period.Value() > 8 ) )
	{
		int16_t change = ( pulse.period.Value() >> shift );
		if( negate ) {
			change = ( pulse.channelNum == PULSE_1 ) ? ( -change - 1 ) : -change;
		}
		
		targetPeriod += ( change <= 0 ) ? 0 : change;
	}

	if ( reload ) {
		sweep.divider.Reload( period + 1 );
		reload = false;
	}

	// Check for carry-bit
	sweep.mute = ( targetPeriod > 0x07FF );

	if( !sweep.mute ) {
		pulse.period.Reload( targetPeriod );
	}
}


bool APU::IsDutyHigh( const PulseChannel& pulse )
{
	return PulseLUT[ pulse.regCtrl.sem.duty ][ ( pulse.sequenceStep + system->config.apu.waveShift ) % 8 ];
}


void APU::PulseSequencer( PulseChannel& pulse )
{
	const uint32_t sampleCnt = static_cast<uint32_t>( ( apuCycle - pulse.lastCycle ).count() );
	float volume = pulse.envelope.output;

	float pulseSample = pulse.envelope.output;

	if ( ( pulse.timer.sem0.counter == 0 ) ||
		( pulse.period.Value() < 8 ) ||
		pulse.mute ||
		!IsDutyHigh( pulse ) /*|| pulse.sweep.mute*/ )
	{
		pulseSample = 0;
	}

	for ( uint32_t sample = 0; sample < sampleCnt; sample++ ) {
		pulse.samples.Enque( pulseSample );
	}

	pulse.lastCycle = apuCycle;
	pulse.sequenceStep = ( pulse.sequenceStep + 1 ) & 0x0F;
}


void APU::ExecPulseChannel( PulseChannel& pulse )
{
	PulseSweep( pulse );
	EnvelopeGenerater( pulse.envelope, pulse.regCtrl.sem.volume, pulse.regCtrl.sem.counterHalt, pulse.regCtrl.sem.isConstant );

	pulse.timer.sem0.timer--;
	if ( halfClk && !pulse.regCtrl.sem.counterHalt ) {
		pulse.timer.sem0.counter--;
	}
	
	if ( pulse.timer.sem0.timer == 0 )
	{
		pulse.timer.sem0.timer = pulse.period.Value() + 1;
		PulseSequencer( pulse );
	}
}


void APU::TriSequencer()
{
	const uint32_t sampleCnt = static_cast<uint32_t>( ( cpuCycle - triangle.lastCycle ).count() );

	bool isSeqHalted = false;
	isSeqHalted |= triangle.lengthCounter.IsZero();
	isSeqHalted |= triangle.linearCounter.IsZero();

	float volume = TriLUT[ triangle.sequenceStep ];
	if ( triangle.mute ) {
		volume = 0;
	}

	for ( uint32_t sample = 0; sample < sampleCnt; sample++ ) {
		triangle.samples.Enque( volume );
	}

	triangle.lastCycle = cpuCycle;

	if( !isSeqHalted ) {
		triangle.sequenceStep = ( triangle.sequenceStep + 1 ) % 32;
	}
}


void APU::ExecChannelTri()
{
	if( quarterClk )
	{
		if( triangle.reloadFlag ) {
			triangle.linearCounter.Reload( triangle.regLinear.sem.counterLoad );
		} else if ( !triangle.linearCounter.IsZero() ) {
			triangle.linearCounter.Dec();
		}

		if ( !triangle.lengthCounter.IsZero() && !triangle.regLinear.sem.counterHalt ) {
			triangle.lengthCounter.Dec();
		}

		if( !triangle.regLinear.sem.counterHalt ) {
			triangle.reloadFlag = false;
		}
	}

	if( triangle.regLinear.sem.counterHalt ) {
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
	const uint16_t bitShift	= noise.regFreq1.sem.mode ? BIT_6 : BIT_1;
	const uint16_t feedback = ( shiftValue ^ ( shiftValue >> bitShift ) )& BIT_MASK_0;
	const uint16_t newShift	= ( ( shiftValue >> 1 ) & ~BIT_MASK_14 ) | ( feedback << BIT_14 );

	noise.shift.Reload( newShift );

	uint8_t volume = noise.envelope.output;
	if ( noise.lengthCounter.IsZero() || ( noise.shift.Value() & BIT_MASK_0 ) || noise.mute ) {
		volume = 0;
	}

	const uint32_t sampleCnt = static_cast<uint32_t>( ( apuCycle - noise.lastCycle ).count() );
	for ( uint32_t sample = 0; sample < sampleCnt; sample++ ) {
		noise.samples.Enque( volume );
	}

	noise.lastCycle = apuCycle;
}


void APU::ExecChannelNoise()
{
	const noiseCtrl_t& ctrl = noise.regCtrl;
	EnvelopeGenerater( noise.envelope, ctrl.sem.volume, ctrl.sem.counterHalt, ctrl.sem.isConstant );

	if ( quarterClk ) {
		if ( !noise.lengthCounter.IsZero() && !ctrl.sem.counterHalt ) {
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
	const uint32_t sampleCnt = static_cast<uint32_t>( ( apuCycle - dmc.lastCycle ).count() );
	for ( uint32_t sample = 0; sample < sampleCnt; sample++ )
	{
		if ( !dmc.silenceFlag ) {
			dmc.samples.Enque( dmc.outputLevel.Value() );
		} else {
			dmc.samples.Enque( 0.0f );
		}
	}

	dmc.lastCycle = apuCycle;
}


void APU::ExecChannelDMC()
{
	if ( dmc.regCtrl.sem.irqEnable ) {
		system->RequestIRQ();
	}

	if( dmc.emptyBuffer ) {
		dmc.sampleBuffer = system->ReadMemory( dmc.addr );
	}

	// https://wiki.nesdev.com/w/index.php/APU_DMC (see output level)
	// 1. If the silence flag is clear, the output level changes based on bit 0 of the shift register.
	// If the bit is 1, add 2; otherwise, subtract 2. But if adding or subtracting 2 would cause the
	// output level to leave the 0-127 range, leave the output level unchanged. This means subtract 2
	// only if the current level is at least 2, or add 2 only if the current level is at most 125.
	if( dmc.silenceFlag )
	{
		if( dmc.shiftReg & 0x01  )
		{
			if( dmc.outputLevel.Value() <= 125 )
			{
				dmc.outputLevel.Inc();
				dmc.outputLevel.Inc(); // TODO: rethink interface or use byte
			}
		}
		else
		{
			if ( dmc.outputLevel.Value() >= 2 )
			{
				dmc.outputLevel.Dec();
				dmc.outputLevel.Dec();
			}
		}
	}

	// 2. The right shift register is clocked.
	dmc.shiftReg >>= 1;

	// 3. As stated above, the bits-remaining counter is decremented. If it becomes zero, a new output cycle is started.
	if ( dmc.addr == 0xFFFF ) {
		dmc.addr = 0x8000;
	} else {
		dmc.addr += 1;
		dmc.byteCnt--;
	}

	if( dmc.byteCnt == 0 )
	{
		dmc.byteCnt = 8;

		if( dmc.regCtrl.sem.loop )
		{
			// TODO: Restart
		}
		else if( dmc.regCtrl.sem.irqEnable ) {
			system->RequestIRQ(); // TODO: is this right?
		}

		if ( dmc.emptyBuffer )
		{
			dmc.silenceFlag = true;
		}
		else
		{
			dmc.silenceFlag = false;
			dmc.shiftReg = dmc.sampleBuffer;
			dmc.emptyBuffer = true;
			dmc.sampleBuffer = 0;
		}
	}

	const uint16_t dmcPeriod = DmcLUT[ NTSC] [ dmc.regCtrl.sem.freq ];
	if( cpuCycle.count() % dmcPeriod ) {
		DmcGenerator();
	}
}


void APU::ExecFrameCounter()
{
	halfClk		= false;
	quarterClk	= false;

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


bool APU::AllChannelHaveSamples()
{
	bool hasSamples = true;
	hasSamples = hasSamples && !pulse1.samples.IsEmpty();
	hasSamples = hasSamples && !pulse2.samples.IsEmpty();
	hasSamples = hasSamples && ( triangle.samples.GetSampleCnt() > 1 );
	hasSamples = hasSamples && !noise.samples.IsEmpty();
	hasSamples = hasSamples && !dmc.samples.IsEmpty();

	return hasSamples;
}


void APU::Begin()
{
	apuTicks = 0;
}


void APU::RegisterSystem( wtSystem* sys )
{
	system = sys;
}


void APU::End()
{
	while( AllChannelHaveSamples() )
	{
		// This doubles up samples from channels other than the triangle channel (which runs at 2x frequency)
		float pulse1Sample			= pulse1.samples.Deque();
		float pulse2Sample			= pulse2.samples.Deque();
		float triSample1			= triangle.samples.Deque();
		float triSample2			= triangle.samples.Deque();
		float noiseSample			= noise.samples.Deque();
		float dmcSample				= dmc.samples.Deque();

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
		soundOutput->master.Enque( floor( 32767.0f * mixedSample1 ) );
		soundOutput->master.Enque( floor( 32767.0f * mixedSample2 ) );
	}

	soundOutput->master.SetHz( CPU_HZ );

	frameOutput = soundOutput;

	currentBuffer = ( currentBuffer + 1 ) % SoundBufferCnt;
	soundOutput = &soundOutputBuffers[currentBuffer];

	soundOutput->master.Reset();
	soundOutput->dbgPulse1.Clear();
	soundOutput->dbgPulse2.Clear();
	soundOutput->dbgTri.Clear();
	soundOutput->dbgNoise.Clear();
	soundOutput->dbgDmc.Clear();
}