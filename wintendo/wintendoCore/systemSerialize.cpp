
#include "NesSystem.h"

template<typename Cycle>
static inline void SerializeCycle( Serializer& serializer, const serializeMode_t mode, Cycle& c )
{
	if ( mode == serializeMode_t::LOAD )
	{
		uint64_t cycles;
		serializer.Next64b( *reinterpret_cast<uint64_t*>( &cycles ), mode );
		c = Cycle( cycles );
	}
	else if ( mode == serializeMode_t::STORE )
	{
		uint64_t cycles = static_cast<uint64_t>( c.count() );
		serializer.Next64b( *reinterpret_cast<uint64_t*>( &cycles ), mode );
	}
}


template<typename Counter>
static inline void SerializeBitCounter( Serializer& serializer, const serializeMode_t mode, Counter& c )
{
	if ( mode == serializeMode_t::LOAD )
	{
		uint16_t value;
		serializer.Next16b( *reinterpret_cast<uint16_t*>( &value ), mode );
		c.Reload( value );
	}
	else if ( mode == serializeMode_t::STORE )
	{
		uint16_t value = c.Value();
		serializer.Next16b( *reinterpret_cast<uint16_t*>( &value ), mode );
	}
}


static inline void SerializeEnvelope( Serializer& serializer, const serializeMode_t mode, envelope_t& e )
{
	serializer.NextBool( e.startFlag,	mode );
	serializer.Next8b( e.decayLevel,	mode );
	serializer.Next8b( e.divCounter,	mode );
	serializer.Next8b( e.output,		mode );
}


static inline void SerializeSweep( Serializer& serializer, const serializeMode_t mode, sweep_t& s )
{
	serializer.NextBool( s.mute, mode );
	serializer.NextBool( s.reloadFlag, mode );
	serializer.Next16b( s.period, mode );
	SerializeBitCounter( serializer, mode, s.divider );
}


void wtSystem::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	SerializeCycle( serializer, mode, sysCycles );
	SerializeCycle( serializer, mode, frame );

	if( mode == serializeMode_t::LOAD ) {
		previousTime = chrono::steady_clock::now(); // This might not be needed
	}

	serializer.Next32b( currentFrame, mode );
	serializer.Next64b( frameNumber, mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &strobeOn ),			mode );
	serializer.Next8b( btnShift[ 0 ],										mode );
	serializer.Next8b( btnShift[ 1 ],										mode );
	serializer.Next8b( mirrorMode,											mode );
	serializer.Next32b( finishedFrame.first,								mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &finishedFrame.second ),mode );

	serializer.NextArray( memory, PhysicalMemorySize, mode );

	cpu.Serialize( serializer, mode );
	ppu.Serialize( serializer, mode );
	apu.Serialize( serializer, mode );
	cart->mapper->Serialize( serializer, mode );
}


void Cpu6502::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	SerializeCycle( serializer, mode, cycle );

	serializer.Next16b( nmiVector,											mode );
	serializer.Next16b( irqVector,											mode );
	serializer.Next16b( resetVector,										mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &interruptRequestNMI ),	mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &interruptRequest ),	mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &oamInProcess ),		mode );
	serializer.Next8b( X,													mode );
	serializer.Next8b( Y,													mode );
	serializer.Next8b( A,													mode );
	serializer.Next8b( SP,													mode );
	serializer.Next8b( P.byte,												mode );
	serializer.Next16b( PC,													mode );
}


void PPU::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	SerializeCycle( serializer, mode, cycle );
	SerializeCycle( serializer, mode, scanelineCycle );

	serializer.Next8b( *reinterpret_cast<uint8_t*>( &genNMI ),				mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &loadingSecondaryOAM ),	mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &nmiOccurred ),			mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &vramAccessed ),		mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &vramWritePending ),	mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &inVBlank ),			mode );
	serializer.Next16b( chrShifts[ 0 ],										mode );
	serializer.Next16b( chrShifts[ 1 ],										mode );
	serializer.Next8b( chrLatch[ 0 ],										mode );
	serializer.Next8b( chrLatch[ 1 ],										mode );
	serializer.Next16b( attrib,												mode );
	serializer.Next8b( palLatch[ 0 ],										mode );
	serializer.Next8b( palLatch[ 1 ],										mode );
	serializer.Next8b( palShifts[ 0 ],										mode );
	serializer.Next8b( palShifts[ 1 ],										mode );
	serializer.Next8b( ppuReadBuffer[ 0 ],									mode );
	serializer.Next8b( ppuReadBuffer[ 1 ],									mode );
	serializer.Next32b( *reinterpret_cast<uint32_t*>( &currentScanline ),	mode );
	serializer.Next32b( *reinterpret_cast<uint32_t*>( &beamPosition.x ),	mode );
	serializer.Next32b( *reinterpret_cast<uint32_t*>( &beamPosition.y ),	mode );
	serializer.Next16b( regT.byte2x,										mode );
	serializer.Next16b( regV.byte2x,										mode );
	serializer.Next16b( regX,												mode );
	serializer.Next16b( regW,												mode );
	serializer.Next8b( curShift,											mode );
	serializer.Next8b( secondaryOamSpriteCnt,								mode );
	serializer.Next8b( regCtrl.byte,										mode );
	serializer.Next8b( regMask.byte,										mode );
	serializer.Next8b( regStatus.current.byte,								mode );
	serializer.Next8b( regStatus.latched.byte,								mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &regStatus.hasLatch ),	mode );
	
	serializer.NextArray( reinterpret_cast<uint8_t*>( &primaryOAM ), OamSize, mode );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &secondaryOAM ), OamSize * sizeof( spriteAttrib_t ), mode );
	serializer.NextArray( nt, KB_2, mode );
	serializer.NextArray( imgPal, PPU::PaletteColorNumber, mode );
	serializer.NextArray( sprPal, PPU::PaletteColorNumber, mode );
	serializer.NextArray( registers, 9, mode );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &plShifts ), 2 * sizeof( pipelineData_t ), mode );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &plLatches ), sizeof( pipelineData_t ), mode );
}


void APU::Serialize( Serializer& serializer, const serializeMode_t mode )
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
	serializer.Next32b( apuTicks,			mode );
*/
}


void PulseChannel::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	serializer.Next8b( regCtrl.byte, mode );
	serializer.Next16b( regTune.byte2x, mode );
	serializer.Next8b( regRamp.byte, mode );
	serializer.Next16b( timer.byte2x, mode );
	serializer.Next32b( volume, mode );
	serializer.Next8b( sequenceStep, mode );
	serializer.NextBool( mute, mode );

	SerializeEnvelope( serializer, mode, envelope );
	SerializeSweep( serializer, mode, sweep );
	SerializeBitCounter( serializer, mode, period );
	SerializeCycle( serializer, mode, lastCycle );
}


void TriangleChannel::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	serializer.Next8b( regLinear.byte,		mode );
	serializer.Next8b( sequenceStep,		mode );
	serializer.Next16b( regTimer.byte2x,	mode );
	serializer.NextBool( reloadFlag,		mode );
	serializer.NextBool( mute,				mode );

	SerializeBitCounter( serializer, mode, linearCounter );
	SerializeBitCounter( serializer, mode, lengthCounter );
	SerializeBitCounter( serializer, mode, timer );	
	SerializeCycle( serializer, mode, lastCycle );
}


void NoiseChannel::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	serializer.Next8b( regCtrl.byte, mode );
	serializer.Next8b( regFreq1.byte, mode );
	serializer.Next8b( regFreq2.byte, mode );
	serializer.NextBool( mute, mode );
	
	SerializeBitCounter( serializer, mode, shift );
	SerializeBitCounter( serializer, mode, timer );
	SerializeBitCounter( serializer, mode, lengthCounter );
	SerializeEnvelope( serializer, mode, envelope );
	SerializeCycle( serializer, mode, lastCycle );
}


void DmcChannel::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	serializer.Next8b( regCtrl.byte, mode );
	serializer.Next8b( regLoad.byte, mode );
	serializer.Next8b( regAddr, mode );
	serializer.Next8b( regLength, mode );
	serializer.NextBool( irq, mode );
	serializer.NextBool( silenceFlag, mode );
	serializer.NextBool( mute, mode );

	serializer.Next16b( addr, mode );
	serializer.Next16b( byteCnt, mode );
	serializer.Next8b( sampleBuffer, mode );
	serializer.NextBool( emptyBuffer, mode );
	serializer.Next8b( shiftReg, mode );

	SerializeBitCounter( serializer, mode, outputLevel );
	SerializeCycle( serializer, mode, lastCycle );
}