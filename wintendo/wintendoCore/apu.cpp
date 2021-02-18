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
		case 0x4000: {
			pulse1.regCtrl.byte = value;
		} break;		
		
		case 0x4001:
		{
			pulse1.regRamp.byte = value;
			pulse1.sweep.reloadFlag = true;
		} break;

		case 0x4002: {
			pulse1.regTune.sem1.lower = value;
		} break;

		case 0x4003:
		{
			pulse1.regTune.sem1.upper = value;
			pulse1.timer.sem0.counter = LengthLUT[ pulse1.regTune.sem0.counter ];
			pulse1.sequenceStep = 0;
			pulse1.envelope.startFlag = true;
		} break;

		case 0x4004: {
			pulse2.regCtrl.byte = value;
		} break;

		case 0x4005:
		{
			pulse2.regRamp.byte = value;
			pulse2.sweep.reloadFlag = true;
		} break;

		case 0x4006: {
			pulse2.regTune.sem1.lower = value;
		} break;

		case 0x4007:
		{
			pulse2.regTune.sem1.upper = value;
			pulse2.timer.sem0.counter = LengthLUT[ pulse2.regTune.sem0.counter ];
			pulse2.sequenceStep = 0;
			pulse2.envelope.startFlag = true;
		} break;

		case 0x4008: {
			triangle.regLinear.byte = value;
		} break;

		case 0x400A: {
			triangle.regTimer.sem1.lower = value;
		} break;

		case 0x400B:
		{
			triangle.regTimer.sem1.upper = value;
			triangle.lengthCounter.Reload( LengthLUT[ triangle.regTimer.sem0.counter ] );
			triangle.reloadFlag = true;
		} break;

		case 0x400C: {
			noise.regCtrl.byte = value;
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

		case 0x4010: {
			dmc.regCtrl.byte = value;
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

			frameCounter.sem.interrupt = false;
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
				RunFrameClock( true, true, false );
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


void APU::ClockEnvelope( envelope_t& envelope, const uint8_t volume, const bool loop, const bool constant )
{
	if ( system->config.apu.disableEnvelope )
	{
		envelope.output = volume;
		return;
	}

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


void APU::ClockSweep( PulseChannel& pulse )
{
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


void APU::ExecPulseChannel( PulseChannel& pulse )
{
	pulse.timer.sem0.timer--;
	if ( pulse.timer.sem0.timer == 0 )
	{
		pulse.timer.sem0.timer = pulse.period.Value() + 1;
		pulse.sequenceStep = ( pulse.sequenceStep + 1 ) & 0x0F;
	}

	float pulseSample = pulse.envelope.output;
	if ( ( pulse.timer.sem0.counter == 0 ) ||
		( pulse.period.Value() < 8 ) ||
		pulse.mute ||
		!IsDutyHigh( pulse ) || pulse.sweep.mute )
	{
		pulseSample = 0;
	}

	pulse.samples.Enque( pulseSample );
	pulse.samples.Enque( pulseSample );
}


void APU::ExecChannelTri()
{
	if( triangle.regLinear.sem.counterHalt ) {
		triangle.lengthCounter.Reload();
	}

	triangle.timer.Dec();
	if ( triangle.timer.IsZero() )
	{
		bool isSeqHalted = false;
		isSeqHalted |= triangle.lengthCounter.IsZero();
		isSeqHalted |= triangle.linearCounter.IsZero();

		if ( !isSeqHalted ) {
			triangle.sequenceStep = ( triangle.sequenceStep + 1 ) % 32;
		}
		triangle.timer.Reload( 1 + triangle.regTimer.sem0.timer );
	}

	float volume = TriLUT[ triangle.sequenceStep ];
	if ( triangle.mute ) {
		volume = 0;
	}

	triangle.samples.Enque( volume );
}


void APU::ExecChannelNoise()
{
	noise.timer.Dec();
	if ( noise.timer.IsZero() )
	{
		const uint16_t shiftValue = noise.shift.Value();
		const uint16_t bitShift = noise.regFreq1.sem.mode ? 6 : 1;
		const uint16_t feedback = ( shiftValue ^ ( shiftValue >> bitShift ) ) & BIT_MASK( 0 );
		const uint16_t newShift = ( ( shiftValue >> 1 ) & ~BIT_MASK( 14 ) ) | ( feedback << 14 );

		noise.shift.Reload( newShift );
		noise.timer.Reload( NoiseLUT[NTSC][ noise.regFreq1.sem.period ] );
	}

	uint8_t volume = noise.envelope.output;
	if ( noise.lengthCounter.IsZero() || ( noise.shift.Value() & BIT_MASK( 0 ) ) || noise.mute ) {
		volume = 0;
	}

	noise.samples.Enque( volume );
	noise.samples.Enque( volume );
}


void APU::DmcGenerator()
{
	const uint32_t sampleCnt = static_cast<uint32_t>( ( apuCycle - dmc.lastApuCycle ).count() );
	const uint32_t cpuSampleCnt = static_cast<uint32_t>( ( cpuCycle - dmc.lastCycle ).count() );
	for ( uint32_t sample = 0; sample < cpuSampleCnt; sample++ )
	{
		if ( !dmc.silenceFlag ) {
			dmc.samples.Enque( dmc.outputLevel.Value() );
		} else {
			dmc.samples.Enque( 0.0f );
		}
	}

	dmc.lastApuCycle = apuCycle;
	dmc.lastCycle = cpuCycle;
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
	const uint32_t mode = frameCounter.sem.mode;
	frameSeqEvent_t& event = FrameSeqEvents[ frameSeqStep ][ mode ];
	if( frameSeqTick.count() == event.cycle )
	{
		const bool halfClk		= event.clkHalf;
		const bool quarterClk	= event.clkQuarter;
		const bool irqClk		= ( event.irq && !frameCounter.sem.interrupt );
		++frameSeqStep;

		dbgHalfClkTicks		+= halfClk ?	1: 0;
		dbgQuarterClkTicks	+= quarterClk ?	1: 0;
		dbgIrqEvents		+= irqClk ?		1: 0;

		RunFrameClock( halfClk, quarterClk, irqClk );
	}

	const uint64_t lastCycle = FrameSeqEvents[ FrameSeqEventCnt - 1 ][ mode ].cycle;
	if ( frameSeqTick.count() == lastCycle )
	{
		frameSeqStep = 0;
		frameSeqTick = cpuCycle_t( 0 );
	}
}


void APU::RunFrameClock( const bool halfClk, const bool quarterClk, const bool irq )
{
	if ( halfClk )
	{
		ClockSweep( pulse1 );
		ClockSweep( pulse2 );

		if ( pulse1.timer.sem0.counter != 0 && !pulse1.regCtrl.sem.counterHalt ) {
			pulse1.timer.sem0.counter--;
		}

		if ( pulse2.timer.sem0.counter != 0 && !pulse2.regCtrl.sem.counterHalt ) {
			pulse2.timer.sem0.counter--;
		}

		if ( !noise.lengthCounter.IsZero() && !noise.regCtrl.sem.counterHalt ) {
			noise.lengthCounter.Dec();
		}
	}

	if ( quarterClk )
	{
		// TODO: clock all length counters?
		const noiseCtrl_t& noiseCtrl = noise.regCtrl;
		ClockEnvelope( pulse1.envelope, pulse1.regCtrl.sem.volume, pulse1.regCtrl.sem.counterHalt, pulse1.regCtrl.sem.isConstant );
		ClockEnvelope( pulse2.envelope, pulse2.regCtrl.sem.volume, pulse2.regCtrl.sem.counterHalt, pulse2.regCtrl.sem.isConstant );
		ClockEnvelope( noise.envelope, noiseCtrl.sem.volume, noiseCtrl.sem.counterHalt, noiseCtrl.sem.isConstant );

		if ( triangle.reloadFlag ) {
			triangle.linearCounter.Reload( triangle.regLinear.sem.counterLoad );
			triangle.reloadFlag = false;
		}
		else if ( !triangle.linearCounter.IsZero() && !triangle.regLinear.sem.counterHalt ) {
			triangle.linearCounter.Dec();
		}

		if ( !triangle.lengthCounter.IsZero() && !triangle.regLinear.sem.counterHalt ) {
		//	triangle.lengthCounter.Dec();
		}

		if ( !triangle.regLinear.sem.counterHalt ) {
		//	triangle.reloadFlag = false;
		}
	}

	if ( irq ) {
		//	system->RequestIRQ();
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

		//apuCycle = chrono::duration_cast<apuCycle_t>( cpuCycle );
		if ( ( cpuCycle.count() % 2 ) == 0 )
		{
			ExecPulseChannel( pulse1 );
			ExecPulseChannel( pulse2 );
			ExecChannelNoise();
			ExecChannelDMC();
		}

		++cpuCycle;
		++frameSeqTick;
	}
	apuCycle = chrono::duration_cast<apuCycle_t>( cpuCycle );

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
	apuDebug.pulse1				= pulse1;
	apuDebug.pulse2				= pulse2;
	apuDebug.triangle			= triangle;
	apuDebug.noise				= noise;
	apuDebug.dmc				= dmc;
	apuDebug.frameCounter		= frameCounter;
	apuDebug.halfClkTicks		= dbgHalfClkTicks;
	apuDebug.quarterClkTicks	= dbgHalfClkTicks;
	apuDebug.irqClkEvents		= dbgIrqEvents;
	apuDebug.frameCounterTicks	= frameSeqTick;
	apuDebug.cycle				= cpuCycle;
	apuDebug.apuCycle			= apuCycle;
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
}


void APU::RegisterSystem( wtSystem* sys )
{
	system = sys;
}


void APU::End()
{
	while( AllChannelHaveSamples() )
	{
		float pulse1Sample			= pulse1.samples.Deque();
		float pulse2Sample			= pulse2.samples.Deque();
		float triSample				= triangle.samples.Deque();
		float noiseSample			= noise.samples.Deque();
		float dmcSample				= dmc.samples.Deque();

		pulse1Sample				= ( system->config.apu.mutePulse1	)	? 0.0f : pulse1Sample;
		pulse2Sample				= ( system->config.apu.mutePulse2	)	? 0.0f : pulse2Sample;
		triSample					= ( system->config.apu.muteTri		)	? 0.0f : triSample;
		noiseSample					= ( system->config.apu.muteNoise	)	? 0.0f : noiseSample;
		dmcSample					= ( system->config.apu.muteDMC		)	? 0.0f : dmcSample;

		const float pulseMixed		= PulseMixer( (uint32_t)pulse1Sample, (uint32_t)pulse2Sample );
		const float tndMixed		= TndMixer( (uint32_t)triSample, (uint32_t)noiseSample, (uint32_t)dmcSample );
		const float mixedSample		= pulseMixed + tndMixed;

		assert( pulseMixed < 0.3f );

		soundOutput->dbgPulse1.Write( pulse1Sample );
		soundOutput->dbgPulse2.Write( pulse2Sample );
		soundOutput->dbgTri.Write( triSample );
		soundOutput->dbgNoise.Write( noiseSample );
		soundOutput->dbgDmc.Write( dmcSample );
		soundOutput->master.Enque( floor( 32767.0f * mixedSample ) );
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