
#include "NesSystem.h"

template<typename Cycle>
static inline void SerializeCycle( Serializer& serializer, Cycle& c )
{
	if ( serializer.GetMode() == serializeMode_t::LOAD )
	{
		uint64_t cycles;
		serializer.Next64b( *reinterpret_cast<uint64_t*>( &cycles ) );
		c = Cycle( cycles );
	}
	else if ( serializer.GetMode() == serializeMode_t::STORE )
	{
		uint64_t cycles = static_cast<uint64_t>( c.count() );
		serializer.Next64b( *reinterpret_cast<uint64_t*>( &cycles ) );
	}
}


template<typename Counter>
static inline void SerializeBitCounter( Serializer& serializer, Counter& c )
{
	if ( serializer.GetMode() == serializeMode_t::LOAD )
	{
		uint16_t value;
		serializer.Next16b( *reinterpret_cast<uint16_t*>( &value ) );
		c.Reload( value );
	}
	else if ( serializer.GetMode() == serializeMode_t::STORE )
	{
		uint16_t value = c.Value();
		serializer.Next16b( *reinterpret_cast<uint16_t*>( &value ) );
	}
}


static inline void SerializeEnvelope( Serializer& serializer, envelope_t& e )
{
	serializer.NextBool( e.startFlag );
	serializer.Next8b( e.decayLevel );
	serializer.Next8b( e.divCounter );
	serializer.Next8b( e.output );
}


static inline void SerializeSweep( Serializer& serializer, sweep_t& s )
{
	serializer.NextBool( s.mute );
	serializer.NextBool( s.reloadFlag );
	serializer.Next16b( s.period );
	SerializeBitCounter( serializer, s.divider );
}


void wtSystem::Serialize( Serializer& serializer )
{
	SerializeCycle( serializer, sysCycles );

	serializer.Next64b( frameNumber );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &strobeOn ) );
	serializer.Next8b( btnShift[ 0 ] );
	serializer.Next8b( btnShift[ 1 ] );

	serializer.NewLabel( STATE_MEMORY_LABEL );
	serializer.NextArray( memory, PhysicalMemorySize );
	serializer.EndLabel( STATE_MEMORY_LABEL );

	cpu.Serialize( serializer );
	ppu.Serialize( serializer );
	apu.Serialize( serializer );
	cart->mapper->Serialize( serializer );
}


void Cpu6502::Serialize( Serializer& serializer )
{
	SerializeCycle( serializer, cycle );

	serializer.Next16b( nmiVector );
	serializer.Next16b( irqVector );
	serializer.Next16b( resetVector );
	serializer.Next16b( irqAddr );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &interruptRequestNMI ) );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &interruptRequest ) );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &oamInProcess ) );
	serializer.Next8b( X );
	serializer.Next8b( Y );
	serializer.Next8b( A );
	serializer.Next8b( SP );
	serializer.Next8b( P.byte );
	serializer.Next16b( PC );
}


void PPU::Serialize( Serializer& serializer )
{
	SerializeCycle( serializer, cycle );

	serializer.Next8b( *reinterpret_cast<uint8_t*>( &genNMI ) );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &nmiOccurred ) );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &vramAccessed ) );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &vramWritePending ) );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &inVBlank ) );
	serializer.Next16b( chrShifts[ 0 ] );
	serializer.Next16b( chrShifts[ 1 ] );
	serializer.Next8b( chrLatch[ 0 ] );
	serializer.Next8b( chrLatch[ 1 ] );
	serializer.Next16b( attrib );
	serializer.Next8b( palLatch[ 0 ] );
	serializer.Next8b( palLatch[ 1 ] );
	serializer.Next8b( palShifts[ 0 ] );
	serializer.Next8b( palShifts[ 1 ] );
	serializer.Next8b( ppuReadBuffer );
	serializer.Next32b( *reinterpret_cast<uint32_t*>( &currentScanline ) );
	serializer.Next8b( beam.point.x );
	serializer.Next8b( beam.point.y );
	serializer.Next16b( regT.byte2x );
	serializer.Next16b( regV.byte2x );
	serializer.Next16b( regX );
	serializer.Next16b( regW );
	serializer.Next8b( curShift );
	serializer.Next8b( secondaryOamSpriteCnt );
	serializer.Next8b( regCtrl.byte );
	serializer.Next8b( regMask.byte );
	serializer.Next8b( regStatus.current.byte );
	serializer.Next8b( regStatus.latched.byte );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &regStatus.hasLatch ) );
	
	serializer.NextArray( reinterpret_cast<uint8_t*>( &primaryOAM ), OamSize );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &secondaryOAM ), OamSize * sizeof( spriteAttrib_t ) );
	serializer.NewLabel( STATE_VRAM_LABEL );
	serializer.NextArray( nt, KB(2) );
	serializer.EndLabel( STATE_VRAM_LABEL );
	serializer.NextArray( imgPal, PPU::PaletteColorNumber );
	serializer.NextArray( sprPal, PPU::PaletteColorNumber );
	serializer.NextArray( registers, 9 );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &plShifts ), 2 * sizeof( pipelineData_t ) );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &plLatches ), sizeof( pipelineData_t ) );
}


void APU::Serialize( Serializer& serializer )
{
/*
	serializer.Next32b( currentBuffer,		mode);
	if( mode == serializeMode_t::LOAD ) {
		soundOutput = &soundOutputBuffers[ currentBuffer ];
		frameOutput = soundOutput;
	}

	// TODO: soundOutputBuffers?
	pulse1.Serialize( serializer, mode );
	pulse2.Serialize( serializer, mode );
	triangle.Serialize( serializer, mode );
	noise.Serialize( serializer, mode );
	dmc.Serialize( serializer, mode );

	serializer.Next8b( frameCounter.byte,	mode );
	serializer.Next8b( regStatus.byte,		mode );
	serializer.NextBool( halfClk,			mode );
	serializer.NextBool( quarterClk,		mode );
	serializer.NextBool( irqClk,			mode );

	serializer.Next32b( frameSeqTick,		mode );
	serializer.Next8b( frameSeqStep,		mode );
*/
}


void PulseChannel::Serialize( Serializer& serializer )
{
	serializer.Next8b( regCtrl.byte );
	serializer.Next16b( regTune.byte2x );
	serializer.Next8b( regRamp.byte );
	serializer.Next32b( volume );
	serializer.Next8b( sequenceStep );
	serializer.NextBool( mute );

	SerializeEnvelope( serializer, envelope );
	SerializeSweep( serializer, sweep );
	SerializeBitCounter( serializer, period );
	SerializeBitCounter( serializer, periodTimer );
	SerializeCycle( serializer, lastCycle );
}


void TriangleChannel::Serialize( Serializer& serializer )
{
	serializer.Next8b( regLinear.byte );
	serializer.Next8b( sequenceStep );
	serializer.Next16b( regTimer.byte2x );
	serializer.NextBool( reloadFlag );
	serializer.NextBool( mute );
	serializer.Next8b( lengthCounter );

	SerializeBitCounter( serializer, linearCounter );
	SerializeBitCounter( serializer, timer );	
	SerializeCycle( serializer, lastCycle );
}


void NoiseChannel::Serialize( Serializer& serializer )
{
	serializer.Next8b( regCtrl.byte );
	serializer.Next8b( regFreq1.byte );
	serializer.Next8b( regFreq2.byte );
	serializer.NextBool( mute );
	serializer.Next8b( lengthCounter );
	
	SerializeBitCounter( serializer, shift );
	SerializeBitCounter( serializer, timer );
	SerializeEnvelope( serializer, envelope );
	SerializeCycle( serializer, lastCycle );
}


void DmcChannel::Serialize( Serializer& serializer )
{
	serializer.Next8b( regCtrl.byte );
	serializer.Next8b( regLoad.byte );
	serializer.Next16b( regAddr );
	serializer.Next16b( regLength );
	serializer.NextBool( irq );
	serializer.NextBool( silenceFlag );
	serializer.NextBool( mute );

	serializer.Next16b( addr );
	serializer.Next16b( bitCnt );
	serializer.Next8b( sampleBuffer );
	serializer.NextBool( emptyBuffer );
	serializer.Next8b( shiftReg );

	SerializeBitCounter( serializer, outputLevel );
	SerializeCycle( serializer, lastCycle );
}