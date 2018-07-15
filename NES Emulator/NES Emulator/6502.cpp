#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <sstream>
#include <assert.h>
#include <map>
#include <bitset>
#include "common.h"
#include "debug.h"
#include "6502.h"
#include "NesSystem.h"

using namespace std;


uint8_t& PPU::Reg( uint16_t address, uint8_t value )
{
	const uint16_t regNum = ( address - 0x2000 );

	PpuRegWriteFunc regFunc = PpuRegWriteMap[regNum];

	return ( this->*regFunc )( value );
}


uint8_t& PPU::Reg( uint16_t address )
{
	const uint16_t regNum = ( address - 0x2000 );

	PpuRegReadFunc regFunc = PpuRegReadMap[regNum];

	return ( this->*regFunc )( );
}


void PPU::GenerateNMI()
{
	system->cpu.interruptTriggered = true;
}

inline void PPU::EnablePrinting()
{
#if DEBUG_ADDR == 1
	system->cpu.printToOutput = true;
#endif // #if DEBUG_ADDR == 1
}


void PPU::GenerateDMA()
{
	system->cpu.oamInProcess = true;
}


uint8_t PPU::DoDMA( const uint16_t address )
{
	//	assert( address == 0 ); // need to handle other case
	memcpy( primaryOAM, &system->GetMemory( Combine( 0x00, static_cast<uint8_t>( address ) ) ), 256 );

	return 0;
}


void RegistersToString( const Cpu6502& cpu, string& regStr )
{
	stringstream sStream;

	sStream << uppercase << "A:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.A ) << setw( 1 ) << " ";
	sStream << uppercase << "X:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.X ) << setw( 1 ) << " ";
	sStream << uppercase << "Y:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.Y ) << setw( 1 ) << " ";
	sStream << uppercase << "P:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.P.byte ) << setw( 1 ) << " ";
	sStream << uppercase << "SP:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.SP ) << setw( 1 );
	//sStream << uppercase << " CYC:" << setw( 3 ) << "0\0";

	regStr = sStream.str();
}


inline uint8_t& Cpu6502::Read( const InstrParams& params )
{
	uint32_t address = 0x00;

	const AddrFunction getAddr = params.getAddr;

	uint8_t& M = ( this->*getAddr )( params, address );

	return M;
}


inline void Cpu6502::Write( const InstrParams& params, const uint8_t value )
{
	assert( params.getAddr != &Cpu6502::Immediate );

	uint32_t address = 0x00;

	const AddrFunction getAddr = params.getAddr;

	uint8_t& M = ( this->*getAddr )( params, address );

	// TODO: Check is RAM, otherwise do another GetMem, get will do the routing logic
	// Might need to have a different wrapper that passes the work to different handlers
	// That could help with RAM, I/O regs, and mappers
	// Wait until working on mappers to design
	// Addressing functions will call a lower level RAM function only
	// TODO: Need a memory manager class

	if ( system->IsPpuRegister( address ) )
	{
		system->ppu.Reg( address, value );
	}
	else if ( system->IsDMA( address ) )
	{
		system->ppu.OAMDMA( value );
	}
	else
	{
		//uint8_t& writeLoc = system->GetMemory( address );
		//assert( M == writeLoc );
		//assert( &M == &writeLoc );
		M = value;
	}

#if DEBUG_MEM == 1
	system->memoryDebug[address] = value;
#endif // #if DEBUG_MEM == 1
}


inline void Cpu6502::SetAluFlags( const uint16_t value )
{
	P.bit.z = CheckZero( value );
	P.bit.n = CheckSign( value );
}


inline bool Cpu6502::CheckSign( const uint16_t checkValue )
{
	return ( checkValue & 0x0080 );
}


inline bool Cpu6502::CheckCarry( const uint16_t checkValue )
{
	return ( checkValue > 0x00ff );
}


inline bool Cpu6502::CheckZero( const uint16_t checkValue )
{
	return ( checkValue == 0 );
}


inline bool Cpu6502::CheckOverflow( const uint16_t src, const uint16_t temp, const uint8_t finalValue )
{
	const uint8_t signedBound = 0x80;
	return CheckSign( finalValue ^ src ) && CheckSign( temp ^ src ) && !CheckCarry( temp );
}


uint8_t Cpu6502::SEC( const InstrParams& params )
{
	P.bit.c = 1;

	return 0;
}


uint8_t Cpu6502::SEI( const InstrParams& params )
{
	P.bit.i = 1;

	return 0;
}


uint8_t Cpu6502::SED( const InstrParams& params )
{
	P.bit.d = 1;

	return 0;
}


uint8_t Cpu6502::CLC( const InstrParams& params )
{
	P.bit.c = 0;

	return 0;
}


uint8_t Cpu6502::CLI( const InstrParams& params )
{
	P.bit.i = 0;

	return 0;
}


uint8_t Cpu6502::CLV( const InstrParams& params )
{
	P.bit.v = 0;

	return 0;
}


uint8_t Cpu6502::CLD( const InstrParams& params )
{
	P.bit.d = 0;

	return 0;
}


uint8_t Cpu6502::CMP( const InstrParams& params )
{
	const uint16_t result = ( A - Read( params ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );

	return 0;
}


uint8_t Cpu6502::CPX( const InstrParams& params )
{
	const uint16_t result = ( X - Read( params ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );

	return 0;
}


uint8_t Cpu6502::CPY( const InstrParams& params )
{
	const uint16_t result = ( Y - Read( params ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );

	return 0;
}


inline uint8_t Cpu6502::AddPageCrossCycles( const uint16_t address )
{
	instructionCycles;
	return 0;
}


inline uint16_t Cpu6502::CombineIndirect( const uint8_t lsb, const uint8_t msb, const uint32_t wrap )
{
	const uint16_t address = Combine( lsb, msb );
	const uint8_t loByte = system->GetMemory( address % wrap );
	const uint8_t hiByte = system->GetMemory( ( address + 1 ) % wrap );
	const uint16_t value = Combine( loByte, hiByte );

	return value;
}


uint8_t& Cpu6502::IndexedIndirect( const InstrParams& params, uint32_t& address )
{
	const uint8_t targetAddress = ( params.param0 + X );
	address = CombineIndirect( targetAddress, 0x00, NesSystem::ZeroPageWrap );

	uint8_t& value = system->GetMemory( address );

	DEBUG_ADDR_INDEXED_INDIRECT

		return value;
}


uint8_t& Cpu6502::IndirectIndexed( const InstrParams& params, uint32_t& address )
{
	address = CombineIndirect( params.param0, 0x00, NesSystem::ZeroPageWrap );

	const uint16_t offset = ( address + Y ) % NesSystem::MemoryWrap;

	uint8_t& value = system->GetMemory( offset );

	AddPageCrossCycles( address );

	DEBUG_ADDR_INDIRECT_INDEXED

		return value;
}


uint8_t& Cpu6502::Absolute( const InstrParams& params, uint32_t& address )
{
	address = Combine( params.param0, params.param1 );

	uint8_t& value = system->GetMemory( address );

	DEBUG_ADDR_ABS

		return value;
}


inline uint8_t& Cpu6502::IndexedAbsolute( const InstrParams& params, uint32_t& address, const uint8_t& reg )
{
	const uint16_t targetAddresss = Combine( params.param0, params.param1 );

	address = ( targetAddresss + reg ) % NesSystem::MemoryWrap;

	uint8_t& value = system->GetMemory( address );

	AddPageCrossCycles( address );

	DEBUG_ADDR_INDEXED_ABS

		return value;
}


uint8_t& Cpu6502::IndexedAbsoluteX( const InstrParams& params, uint32_t& address )
{
	return IndexedAbsolute( params, address, X );
}


uint8_t& Cpu6502::IndexedAbsoluteY( const InstrParams& params, uint32_t& address )
{
	return IndexedAbsolute( params, address, Y );
}


inline uint8_t& Cpu6502::Zero( const InstrParams& params, uint32_t& address )
{
	const uint16_t targetAddresss = Combine( params.param0, 0x00 );

	address = ( targetAddresss % NesSystem::ZeroPageWrap );

	uint8_t& value = system->GetMemory( address );

	DEBUG_ADDR_ZERO

		return value;
}


inline uint8_t& Cpu6502::IndexedZero( const InstrParams& params, uint32_t& address, const uint8_t& reg )
{
	const uint16_t targetAddresss = Combine( params.param0, 0x00 );

	address = ( targetAddresss + reg ) % NesSystem::ZeroPageWrap;

	uint8_t& value = system->GetMemory( address );

	DEBUG_ADDR_INDEXED_ZERO

		return value;
}


uint8_t& Cpu6502::IndexedZeroX( const InstrParams& params, uint32_t& address )
{
	return IndexedZero( params, address, X );
}


uint8_t& Cpu6502::IndexedZeroY( const InstrParams& params, uint32_t& address )
{
	return IndexedZero( params, address, Y );
}


uint8_t& Cpu6502::Immediate( const InstrParams& params, uint32_t& address )
{
	// This is a bit dirty, but necessary to maintain uniformity, asserts have been placed at lhs usages for now
	uint8_t& value = const_cast< InstrParams& >( params ).param0;

	address = Cpu6502::InvalidAddress;

	DEBUG_ADDR_IMMEDIATE

		return value;
}


inline const uint8_t Cpu6502::ImmediateS::operator()( const InstrParams& params, uint32_t& address ) const
{
	// This is a bit dirty, but necessary to maintain uniformity, asserts have been placed at lhs usages for now
	uint8_t value = const_cast< InstrParams& >( params ).param0;

	address = Cpu6502::InvalidAddress;

	//	DEBUG_ADDR_IMMEDIATE

	return value;
}


inline void Cpu6502::ImmediateS::operator()( uint8_t& value, const InstrParams& params ) const
{
	// This is a bit dirty, but necessary to maintain uniformity, asserts have been placed at lhs usages for now
	value = const_cast< InstrParams& >( params ).param0;

	//	DEBUG_ADDR_IMMEDIATE
}


uint8_t& Cpu6502::Accumulator( const InstrParams& params, uint32_t& address )
{
	address = Cpu6502::InvalidAddress;

	DEBUG_ADDR_ACCUMULATOR

		return A;
}


uint8_t Cpu6502::AccumulatorS::operator()( const InstrParams& params, uint32_t& address ) const
{
	address = Cpu6502::InvalidAddress;

	//	DEBUG_ADDR_ACCUMULATOR

	return params.cpu->A;
}

/*
void Cpu6502::AccumulatorS::operator()( Cpu6502& cpu, const uint32_t address, const uint8_t value ) const
{
cpu.A = value;
}*/


uint8_t Cpu6502::LDA( const InstrParams& params )
{
	A = Read( params );

	SetAluFlags( A );

	return 0;
}


uint8_t Cpu6502::LDX( const InstrParams& params )
{
	X = Read( params );

	SetAluFlags( X );

	return 0;
}


uint8_t Cpu6502::LDY( const InstrParams& params )
{
	Y = Read( params );

	SetAluFlags( Y );

	return 0;
}


uint8_t Cpu6502::STA( const InstrParams& params )
{
	Write( params, A );

	return 0;
}


inline uint8_t Cpu6502::STX( const InstrParams& params )
{
	Write( params, X );

	return 0;
}


uint8_t Cpu6502::STY( const InstrParams& params )
{
	Write( params, Y );

	return 0;
}


uint8_t Cpu6502::TXS( const InstrParams& params )
{
	SP = X;

	return 0;
}


uint8_t Cpu6502::TXA( const InstrParams& params )
{
	A = X;
	SetAluFlags( A );

	return 0;
}


uint8_t Cpu6502::TYA( const InstrParams& params )
{
	A = Y;
	SetAluFlags( A );

	return 0;
}


uint8_t Cpu6502::TAX( const InstrParams& params )
{
	X = A;
	SetAluFlags( X );

	return 0;
}


uint8_t Cpu6502::TAY( const InstrParams& params )
{
	Y = A;
	SetAluFlags( Y );

	return 0;
}


uint8_t Cpu6502::TSX( const InstrParams& params )
{
	X = SP;
	SetAluFlags( X );

	return 0;
}


uint8_t Cpu6502::ADC( const InstrParams& params )
{
	// http://nesdev.com/6502.txt, "INSTRUCTION OPERATION - ADC"
	const uint8_t M = Read( params );
	const uint16_t src = A;
	const uint16_t carry = ( P.bit.c ) ? 1 : 0;
	const uint16_t temp = A + M + carry;

	A = ( temp & 0xFF );

	P.bit.z = CheckZero( temp );
	P.bit.v = CheckOverflow( M, temp, A );
	SetAluFlags( A );

	P.bit.c = ( temp > 0xFF );

	return 0;
}


uint8_t Cpu6502::SBC( const InstrParams& params )
{
	const uint8_t& M = Read( params );
	const uint16_t carry = ( P.bit.c ) ? 0 : 1;
	const uint16_t result = A - M - carry;

	SetAluFlags( result );

	P.bit.v = ( CheckSign( A ^ result ) && CheckSign( A ^ M ) );
	P.bit.c = !CheckCarry( result );

	A = result & 0xFF;

	return 0;
}


uint8_t Cpu6502::INX( const InstrParams& params )
{
	++X;
	SetAluFlags( X );

	return 0;
}


uint8_t Cpu6502::INY( const InstrParams& params )
{
	++Y;
	SetAluFlags( Y );

	return 0;
}


uint8_t Cpu6502::DEX( const InstrParams& params )
{
	--X;
	SetAluFlags( X );

	return 0;
}


uint8_t Cpu6502::DEY( const InstrParams& params )
{
	--Y;
	SetAluFlags( Y );

	return 0;
}


uint8_t Cpu6502::INC( const InstrParams& params )
{
	const uint8_t result = Read( params ) + 1;

	Write( params, result );

	SetAluFlags( result );

	return 0;
}


uint8_t Cpu6502::DEC( const InstrParams& params )
{
	const uint8_t result = Read( params ) - 1;

	Write( params, result );

	SetAluFlags( result );

	return 0;
}


void Cpu6502::Push( const uint8_t value )
{
	system->GetStack() = value;
	SP--;
}


void Cpu6502::Pushuint16_t( const uint16_t value )
{
	Push( ( value >> 8 ) & 0xFF );
	Push( value & 0xFF );
}


uint8_t Cpu6502::Pull()
{
	SP++;
	return system->GetStack();
}


uint16_t Cpu6502::Pulluint16_t()
{
	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	return Combine( loByte, hiByte );
}


uint8_t Cpu6502::PHP( const InstrParams& params )
{
	Push( P.byte | STATUS_UNUSED | STATUS_BREAK );

	return 0;
}


uint8_t Cpu6502::PHA( const InstrParams& params )
{
	Push( A );

	return 0;
}


uint8_t Cpu6502::PLA( const InstrParams& params )
{
	A = Pull();

	SetAluFlags( A );

	return 0;
}


uint8_t Cpu6502::PLP( const InstrParams& params )
{
	// https://wiki.nesdev.com/w/index.php/Status_flags
	const uint8_t status = ~STATUS_BREAK & Pull();
	P.byte = status | ( P.byte & STATUS_BREAK ) | STATUS_UNUSED;

	return 0;
}


uint8_t Cpu6502::NOP( const InstrParams& params )
{
	return 0;
}


uint8_t Cpu6502::ASL( const InstrParams& params )
{
	const uint8_t& M = Read( params );

	P.bit.c = !!( M & 0x80 );
	Write( params, M << 1 );
	SetAluFlags( M );

	return 0;
}


uint8_t Cpu6502::LSR( const InstrParams& params )
{
	const uint8_t& M = Read( params );

	P.bit.c = ( M & 0x01 );
	Write( params, M >> 1 );
	SetAluFlags( M );

	return 0;
}


uint8_t Cpu6502::AND( const InstrParams& params )
{
	A &= Read( params );

	SetAluFlags( A );

	return 0;
}


uint8_t Cpu6502::BIT( const InstrParams& params )
{
	const uint8_t& M = Read( params );

	P.bit.z = !( A & M );
	P.bit.n = CheckSign( M );
	P.bit.v = !!( M & 0x40 );

	return 0;
}


uint8_t Cpu6502::EOR( const InstrParams& params )
{
	A ^= Read( params );

	SetAluFlags( A );

	return 0;
}


uint8_t Cpu6502::ORA( const InstrParams& params )
{
	A |= Read( params );

	SetAluFlags( A );

	return 0;
}


uint8_t Cpu6502::JMP( const InstrParams& params )
{
	PC = Combine( params.param0, params.param1 );

	DEBUG_ADDR_JMP

		return 0;
}


uint8_t Cpu6502::JMPI( const InstrParams& params )
{
	const uint16_t addr0 = Combine( params.param0, params.param1 );

	// Hardware bug - http://wiki.nesdev.com/w/index.php/Errata
	if ( ( addr0 & 0xff ) == 0xff )
	{
		const uint16_t addr1 = Combine( 0x00, params.param1 );

		PC = ( Combine( system->GetMemory( addr0 ), system->GetMemory( addr1 ) ) );
	}
	else
	{
		PC = ( Combine( system->GetMemory( addr0 ), system->GetMemory( addr0 + 1 ) ) );
	}

	DEBUG_ADDR_JMPI

		return 0;
}


uint8_t Cpu6502::JSR( const InstrParams& params )
{
	uint16_t retAddr = PC - 1;

	Push( ( retAddr >> 8 ) & 0xFF );
	Push( retAddr & 0xFF );

	PC = Combine( params.param0, params.param1 );

	DEBUG_ADDR_JSR

		return 0;
}


uint8_t Cpu6502::BRK( const InstrParams& params )
{
	P.bit.b = 1;

	interruptTriggered = true;

	return 0;
}


uint8_t Cpu6502::RTS( const InstrParams& params )
{
	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	PC = 1 + Combine( loByte, hiByte );

	return 0;
}


uint8_t Cpu6502::RTI( const InstrParams& params )
{
	PLP( params );

	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	PC = Combine( loByte, hiByte );

	return 0;
}



inline uint8_t Cpu6502::Branch( const InstrParams& params, const bool takeBranch )
{
	const uint16_t branchedPC = PC + static_cast< int8_t >( params.param0 );

	uint8_t cycles = 0;

	if ( takeBranch )
	{
		PC = branchedPC;
	}
	else
	{
		++cycles;
	}

	DEBUG_ADDR_BRANCH

		return ( cycles + AddPageCrossCycles( branchedPC ) );
}


uint8_t Cpu6502::BMI( const InstrParams& params )
{
	return Branch( params, ( P.bit.n ) );
}


uint8_t Cpu6502::BVS( const InstrParams& params )
{
	return Branch( params, ( P.bit.v ) );
}


uint8_t Cpu6502::BCS( const InstrParams& params )
{
	return Branch( params, ( P.bit.c ) );
}


uint8_t Cpu6502::BEQ( const InstrParams& params )
{
	return Branch( params, ( P.bit.z ) );
}


uint8_t Cpu6502::BPL( const InstrParams& params )
{
	return Branch( params, !( P.bit.n ) );
}


uint8_t Cpu6502::BVC( const InstrParams& params )
{
	return Branch( params, !( P.bit.v ) );
}


uint8_t Cpu6502::BCC( const InstrParams& params )
{
	return Branch( params, !( P.bit.c ) );
}


uint8_t Cpu6502::BNE( const InstrParams& params )
{
	return Branch( params, !( P.bit.z ) );
}


uint8_t Cpu6502::ROL( const InstrParams& params )
{
	uint16_t temp = Read( params ) << 1;
	temp = ( P.bit.c ) ? temp | 0x0001 : temp;

	P.bit.c = CheckCarry( temp );

	temp &= 0xFF;

	SetAluFlags( temp );

	Read( params ) = static_cast< uint8_t >( temp & 0xFF );

	return 0;
}


uint8_t Cpu6502::ROR( const InstrParams& params )
{
	uint16_t temp = ( P.bit.c ) ? Read( params ) | 0x0100 : Read( params );

	P.bit.c = ( temp & 0x01 );

	temp >>= 1;

	SetAluFlags( temp );

	Read( params ) = static_cast< uint8_t >( temp & 0xFF );

	return 0;
}


uint8_t Cpu6502::NMI()
{
	Pushuint16_t( PC - 1 );
	// http://wiki.nesdev.com/w/index.php/CPU_status_flag_behavior
	Push( P.byte | STATUS_BREAK );

	P.bit.i = 1;

	PC = nmiVector;

	return 0;
}


uint8_t Cpu6502::IRQ()
{
	return NMI();
}


uint8_t Cpu6502::Illegal( const InstrParams& params )
{
	//assert( 0 );

	return 0;
}


inline uint8_t Cpu6502::PullProgramByte()
{
	return system->GetMemory( PC++ );
}


inline uint8_t Cpu6502::PeekProgramByte() const
{
	return system->GetMemory( PC );
}


inline cpuCycle_t Cpu6502::Exec()
{
#if DEBUG_ADDR == 1
	static bool enablePrinting = true;

	debugAddr.str( std::string() );
	string regStr;
	RegistersToString( *this, regStr );
#endif // #if DEBUG_ADDR == 1

	instructionCycles = cpuCycle_t( 0 );

	if ( oamInProcess )
	{

		// http://wiki.nesdev.com/w/index.php/PPU_registers#OAMDMA
		if ( ( cycle % 2 ) == cpuCycle_t( 0 ) )
			instructionCycles += cpuCycle_t( 514 );
		else
			instructionCycles += cpuCycle_t( 513 );

		oamInProcess = false;

		return instructionCycles;
	}

	const uint16_t instrBegin = PC;

#if DEBUG_MODE == 1
	if ( PC == forceStopAddr )
	{
		forceStop = true;
		return cpuCycle_t( 0 );
	}
#endif // #if DEBUG_MODE == 1

	uint8_t curbyte = system->GetMemory( instrBegin );

	PC++;

	InstrParams params;

	if ( resetTriggered )
	{
	}

	if ( interruptTriggered )
	{
		NMI();

		interruptTriggered = false;

		//Exec();

		return cpuCycle_t( 0 );
	}

	const InstructionMapTuple& pair = InstructionMap[curbyte];

	const uint8_t operands = pair.operands;

	Instruction_depr instruction = pair.instr;

	params.getAddr = pair.addrFunc;
	params.cpu = this;

	if ( operands >= 1 )
	{
		params.param0 = system->GetMemory( PC );
	}

	if ( operands == 2 )
	{
		params.param1 = system->GetMemory( PC + 1 );
	}

	PC += operands;

	if( curbyte == 0xA9 )
	{
		( *pair.addrFunctor )( params, *this );
	}
	else
	{
		( this->*instruction )( params );
	}

	instructionCycles += cpuCycle_t( pair.cycles );

	DEBUG_CPU_LOG

	return instructionCycles;
}


bool Cpu6502::Step( const cpuCycle_t nextCycle )
{
	/*
	if ( interruptTriggered )
	{
	NMI();

	interruptTriggered = false;

	Exec();
	}*/

	while ( ( cycle < nextCycle ) && !forceStop )
	{
		cycle += cpuCycle_t( Exec() );
	}

	return !forceStop;
}