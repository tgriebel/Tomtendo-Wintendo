#include "stdafx.h"
#include "apu.h"
#include "NesSystem.h"
#include <algorithm>

//#pragma optimize("", off)

float APU::GetPulseFrequency( PulseChannel& pulse )
{
	float freq = CPU_HZ / ( 16.0f * pulse.period.Value() + 1 );
	freq *= system->GetConfig()->apu.frequencyScale;
	return freq;
}


float APU::GetPulsePeriod( PulseChannel& pulse )
{
	return ( 1.0f / system->GetConfig()->apu.frequencyScale ) * ( pulse.period.Value() );
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
			pulse1.period.Reload( pulse1.regTune.sem0.timer );
		} break;

		case 0x4003:
		{
			pulse1.regTune.sem1.upper = value;
			pulse1.lengthCounter = LengthLUT[ pulse1.regTune.sem0.counter ];
			pulse1.period.Reload( pulse1.regTune.sem0.timer );
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
			pulse2.period.Reload( pulse2.regTune.sem0.timer );
		} break;

		case 0x4007:
		{
			pulse2.regTune.sem1.upper = value;
			pulse2.lengthCounter = LengthLUT[ pulse2.regTune.sem0.counter ];
			pulse2.period.Reload( pulse2.regTune.sem0.timer );
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
			triangle.lengthCounter = LengthLUT[ triangle.regTimer.sem0.counter ];
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
			noise.lengthCounter = LengthLUT[ noise.regFreq2.sem.length ];
		} break;

		case 0x4010: {
			dmc.regCtrl.byte = value;
			dmc.period = DmcLUT[ NTSC ][ dmc.regCtrl.sem.freq ];
		} break;

		case 0x4011:
		{
			dmc.regLoad.byte = value;
			dmc.outputLevel.Reload( dmc.regLoad.sem.counter );
			dmc.emptyBuffer = false;
		} break;

		case 0x4012:
		{
			dmc.regAddr = ( 0xC000 | ( value << 6 ) ) & 0xFFC0;
		} break;

		case 0x4013:
		{
			dmc.regLength = ( 0x01 | ( value << 4 ) );
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
				pulse1.lengthCounter = 0;
			}

			if ( pulse2.mute ) {
				pulse2.lengthCounter = 0;
			}

			if( triangle.mute ) {
				triangle.lengthCounter = 0;
			}

			if ( noise.mute ) {
				noise.lengthCounter = 0;
			}
			
			if ( dmc.mute ) {
				dmc.bytesRemaining = 0;
			} else if ( dmc.bytesRemaining == 0 ) {
				dmc.startRead = true;
			}

			frameCounter.sem.interrupt = false;
			dmc.regCtrl.sem.irqEnable = false;

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
	result.sem.p1 = ( pulse1.lengthCounter > 0 );
	result.sem.p2 = ( pulse2.lengthCounter > 0 );
	result.sem.n = ( noise.lengthCounter > 0 );
	result.sem.t = ( triangle.lengthCounter > 0 );
	result.sem.d = ( dmc.bytesRemaining > 0 );
	result.sem.dmcIrq = dmc.regCtrl.sem.irqEnable; // TODO: is this right?
	result.sem.frIrq = frameCounter.sem.interrupt;

	frameCounter.sem.interrupt = false;

	return result.byte;
}


void APU::ClockEnvelope( envelope_t& envelope, const uint8_t volume, const bool loop, const bool constant )
{
	if ( system->GetConfig()->apu.disableEnvelope )
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
//	assert( envelope.decayLevel <= 0x0F );

	if ( constant ) {
		envelope.output = volume;
	} else {
		envelope.output = envelope.decayLevel;
	}
	assert( volume >= 0x00 && volume <= 0x0F );
}


void APU::ClockSweep( PulseChannel& pulse )
{
	if ( system->GetConfig()->apu.disableSweep || !pulse.regRamp.sem.enabled ) { // TODO: avoid checking enable here since more conditions matter
		pulse.period.Reload( pulse.regTune.sem0.timer );
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
		
		targetPeriod += change;
	}

	if ( reload )
	{
		sweep.divider.Reload( period + 1 );
		reload = false;
	}

	// Check for carry-bit and overflow
	sweep.mute = ( targetPeriod > 0x07FF );
	if( !sweep.mute ) {
		pulse.period.Reload( targetPeriod );
	}
}


bool APU::IsDutyHigh( const PulseChannel& pulse )
{
	return PulseLUT[ pulse.regCtrl.sem.duty ][ ( pulse.sequenceStep + system->GetConfig()->apu.waveShift ) % 8 ];
}


void APU::ExecPulseChannel( PulseChannel& pulse )
{
	pulse.periodTimer.Dec();
	if ( pulse.periodTimer.IsZero() )
	{
		pulse.periodTimer.Reload( pulse.period.Value() + 1 );
		pulse.sequenceStep = ( pulse.sequenceStep + 1 ) & 0x07;
	}
	
	float pulseSample = pulse.envelope.output;
	if ( ( pulse.lengthCounter == 0 ) ||
		( pulse.period.Value() < 8 ) ||
		pulse.mute ||
		!IsDutyHigh( pulse ) || pulse.sweep.mute )
	{
		pulseSample = 0;
	}

	pulse.sample = pulseSample;
}


void APU::ExecChannelTri()
{
	if( triangle.regLinear.sem.counterHalt ) {
		triangle.lengthCounter = 0;
	}

	triangle.timer.Dec();
	if ( triangle.timer.IsZero() )
	{
		bool isSeqHalted = false;
		isSeqHalted |= ( triangle.lengthCounter == 0 );
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

	triangle.sample = volume;
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
	if ( ( noise.lengthCounter == 0 ) || ( noise.shift.Value() & BIT_MASK( 0 ) ) || noise.mute ) {
		volume = 0;
	}

	noise.sample = volume;
}


void APU::SampleDmcBuffer()
{
	if ( dmc.startRead )
	{
		dmc.startRead = false;
		dmc.addr = dmc.regAddr;
		dmc.bytesRemaining = dmc.regLength;
	}

	if ( dmc.emptyBuffer && ( dmc.bytesRemaining > 0 ) ) {
		dmc.sampleBuffer = system->ReadMemory( dmc.addr );
		dmc.emptyBuffer = false;

		if ( dmc.addr == 0xFFFF ) {
			dmc.addr = 0x8000;
		}
		else {
			++dmc.addr;
		}

		--dmc.bytesRemaining;
		if ( dmc.bytesRemaining == 0 ) {
			if ( dmc.regCtrl.sem.loop ) {
				dmc.startRead = true;
			}
			else if ( dmc.regCtrl.sem.irqEnable ) {
				dmc.irq = true;
			}
		}
	}
}


void APU::ClockDmc()
{
	// https://wiki.nesdev.com/w/index.php/APU_DMC (see output level)
	// 1. If the silence flag is clear, the output level changes based on bit 0 of the shift register.
	// If the bit is 1, add 2; otherwise, subtract 2. But if adding or subtracting 2 would cause the
	// output level to leave the 0-127 range, leave the output level unchanged. This means subtract 2
	// only if the current level is at least 2, or add 2 only if the current level is at most 125.
	if ( !dmc.silenceFlag )
	{
		if ( dmc.shiftReg & 0x01 )
		{
			if ( dmc.outputLevel.Value() <= 125 )
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
		// 2. The right shift register is clocked.
		dmc.shiftReg >>= 1;
	}

	// 3. As stated above, the bits-remaining counter is decremented. If it becomes zero, a new output cycle is started.
	--dmc.bitCnt;
	if ( dmc.bitCnt == 0 )
	{
		dmc.bitCnt = 8;
		if ( dmc.regCtrl.sem.loop )
		{
			// TODO: Restart	
			assert( 0 ); // implement
		}
		else if ( dmc.regCtrl.sem.irqEnable && dmc.irq ) {
			//	TODO: 
			assert( 0 ); // untested
			system->RequestIRQ();
		}

		if ( dmc.emptyBuffer ) {
			dmc.silenceFlag = true;
		} else {
			dmc.silenceFlag = false;
			dmc.shiftReg = dmc.sampleBuffer;
			dmc.emptyBuffer = true;
		}
	}
}


void APU::ExecChannelDMC()
{
	--dmc.periodCounter;
	if( dmc.periodCounter == 0 )
	{
		SampleDmcBuffer();
		ClockDmc();
		dmc.periodCounter = dmc.period;
	}

	const float volume = dmc.outputLevel.Value();
	dmc.sample = ( dmc.mute ? 0.0f : volume );
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

		if ( ( pulse1.lengthCounter != 0 ) && !pulse1.regCtrl.sem.counterHalt ) {
			pulse1.lengthCounter--;
		}

		if ( ( pulse2.lengthCounter != 0 ) && !pulse2.regCtrl.sem.counterHalt ) {
			pulse2.lengthCounter--;
		}

		if ( ( noise.lengthCounter != 0 ) && !noise.regCtrl.sem.counterHalt ) {
			noise.lengthCounter--;
		}
	}

	if ( quarterClk )
	{
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

		if ( ( triangle.lengthCounter != 0 ) && !triangle.regLinear.sem.counterHalt ) {
			triangle.lengthCounter--;
		}

		if ( !triangle.regLinear.sem.counterHalt ) {
			triangle.reloadFlag = false;
		}
	}

	if ( irq ) {
		//	system->RequestIRQ();
	}
}


bool APU::Step( const cpuCycle_t& nextCpuCycle )
{
	const apuCycle_t nextApuCycle = CpuToApuCycle( nextCpuCycle );

	//dbgStartCycle		= apuCycle;
	//dbgTargetCycle	= nextApuCycle;
	//dbgSysStartCycle	= chrono::duration_cast<masterCycle_t>( dbgStartCycle );
	//dbgSysTargetCycle	= chrono::duration_cast<masterCycle_t>( dbgTargetCycle );

	while ( cpuCycle < nextCpuCycle )
	{
		ExecFrameCounter();
		ExecChannelTri();
		ExecChannelDMC();

		//apuCycle = chrono::duration_cast<apuCycle_t>( cpuCycle );
		if ( ( cpuCycle.count() & 1 ) == 0 )
		{
			ExecPulseChannel( pulse1 );
			ExecPulseChannel( pulse2 );
			ExecChannelNoise();
		}

		Mixer();

		++cpuCycle;
		++frameSeqTick;
	}
	apuCycle = CpuToApuCycle( cpuCycle );

	return true;
}


void APU::End()
{
	frameOutput = soundOutput;

	currentBuffer = ( currentBuffer + 1 ) % SoundBufferCnt;
	soundOutput = &soundOutputBuffers[ currentBuffer ];

	soundOutput->mixed.Reset();
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
	apuDebug.status				= regStatus;
	apuDebug.halfClkTicks		= dbgHalfClkTicks;
	apuDebug.quarterClkTicks	= dbgQuarterClkTicks;
	apuDebug.irqClkEvents		= dbgIrqEvents;
	apuDebug.frameCounterTicks	= frameSeqTick;
	apuDebug.cycle				= cpuCycle;
	apuDebug.apuCycle			= apuCycle;
}


void APU::Begin()
{
}


void APU::RegisterSystem( wtSystem* sys )
{
	system = sys;
}


void APU::Mixer()
{
	const config_t::APU* config	= &system->GetConfig()->apu;
	const float pulse1Sample	= ( config->mutePulse1	) ? 0.0f : pulse1.sample;
	const float pulse2Sample	= ( config->mutePulse2	) ? 0.0f : pulse2.sample;
	const float triSample		= ( config->muteTri		) ? 0.0f : triangle.sample;
	const float noiseSample		= ( config->muteNoise	) ? 0.0f : noise.sample;
	const float dmcSample		= ( config->muteDMC		) ? 0.0f : dmc.sample;

	const float pulseMixed		= PulseMixer( (uint32_t)pulse1Sample, (uint32_t)pulse2Sample );
	const float tndMixed		= TndMixer( (uint32_t)triSample, (uint32_t)noiseSample, (uint32_t)dmcSample );
	const float mixedSample		= pulseMixed + tndMixed;

	assert( pulseMixed < 0.3f );

	soundOutput->mixed.Enque( 32767.0f * mixedSample );

#if DEBUG_APU_CHANNELS
	const float volumeScale = 1.0f;
	if ( config->dbgChannelBits & 0x01 ) {
		soundOutput->dbgMixed.EnqueFIFO( volumeScale * mixedSample );
	}
	if( config->dbgChannelBits & 0x02 ) {
		const float pulseMixed = PulseMixer( (uint32_t)pulse1Sample, 0 );
		soundOutput->dbgPulse1.EnqueFIFO( volumeScale * pulseMixed );
	}
	if ( config->dbgChannelBits & 0x04 ) {
		const float pulseMixed = PulseMixer( 0, (uint32_t)pulse2Sample );
		soundOutput->dbgPulse2.EnqueFIFO( volumeScale * pulseMixed );
	}
	if ( config->dbgChannelBits & 0x08 ) {
		const float triMixed = TndMixer( (uint32_t)triSample, 0, 0 );
		soundOutput->dbgTri.EnqueFIFO( volumeScale * triMixed );
	}
	if ( config->dbgChannelBits & 0x10 ) {
		const float noiseMixed = TndMixer( 0, (uint32_t)noiseSample, 0 );
		soundOutput->dbgNoise.EnqueFIFO( volumeScale * noiseMixed );
	}
	if ( config->dbgChannelBits & 0x20 ) {
		const float dmcMixed = TndMixer( 0, 0, (uint32_t)dmcSample );
		soundOutput->dbgDmc.EnqueFIFO( volumeScale * dmcMixed );
	}
#endif
}