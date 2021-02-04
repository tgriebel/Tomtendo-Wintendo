
#include "NesSystem.h"

template<typename Cycle>
static inline void SerializeCycle( Serializer& serializer, const serializeMode_t mode, Cycle& c )
{
	if ( mode == serializeMode_t::LOAD ) {
		uint64_t cycles = c.count();
		serializer.Next64b( *reinterpret_cast<uint64_t*>( &cycles ), mode );
		c = Cycle( cycles );
	}
	else {
		uint64_t cycles = c.count();
		serializer.Next64b( *reinterpret_cast<uint64_t*>( &cycles ), mode );
	}
}


void wtSystem::Serialize( Serializer& serializer, const serializeMode_t mode )
{
	SerializeCycle( serializer, mode, sysCycles );
	SerializeCycle( serializer, mode, frame );
	// SerializeCycle( serializer, mode, previousTime );

	serializer.Next32b( currentFrame, mode );
	serializer.Next64b( frameNumber, mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &strobeOn ),			mode );
	serializer.Next8b( btnShift[ 0 ],										mode );
	serializer.Next8b( btnShift[ 1 ],										mode );
	serializer.Next8b( mirrorMode,											mode );
	serializer.Next32b( finishedFrame.first,								mode );
	serializer.Next8b( *reinterpret_cast<uint8_t*>( &finishedFrame.second ),mode );

	serializer.NextArray( memory, VirtualMemorySize, mode );

	cpu.Serialize( serializer, mode );
	ppu.Serialize( serializer, mode );
	apu.Serialize( serializer, mode );
	cart.mapper->Serialize( serializer, mode );
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
	serializer.NextArray( vram, VirtualMemorySize, mode );
	serializer.NextArray( registers, 9, mode );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &plShifts ), 2 * sizeof( pipelineData_t ), mode );
	serializer.NextArray( reinterpret_cast<uint8_t*>( &plLatches ), sizeof( pipelineData_t ), mode );
}


void APU::Serialize( Serializer& serializer, const serializeMode_t mode )
{
}
