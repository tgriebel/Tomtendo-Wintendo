
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

//test commit
using namespace std;
using namespace chrono;
using namespace this_thread;

std::map<half, byte> memoryDebug;
ofstream logFile;


void RegistersToString( const Cpu6502& cpu, string& regStr )
{
	stringstream sStream;

	sStream << uppercase << "A:" << setfill( '0' ) << setw(2) << hex << static_cast<int>( cpu.A ) << setw(1) << " ";
	sStream << uppercase << "X:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.X ) << setw( 1 ) << " ";
	sStream << uppercase << "Y:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.Y ) << setw( 1 ) << " ";
	sStream << uppercase << "P:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.P ) << setw( 1 ) << " ";
	sStream << uppercase << "SP:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.SP ) << setw( 1 );
	//sStream << "PC:" << hex << static_cast<int>( cpu.PC ) << " ";
	//sStream << uppercase << " CYC:" << setw( 3 ) << "0\0";

	bitset<8> p( cpu.P );

	//sStream << "\tP: NV-BDIZC" << "\t" << p;

	regStr = sStream.str();
}


void RecordMemoryWrite( const half address, const byte value )
{
	memoryDebug[address] = value;
}


inline void Cpu6502::SetProcessorStatus( const StatusBit bit, const bool toggleOn )
{
	P &= ~bit;

	if ( !toggleOn )
		return;

	P |= ( bit | STATUS_UNUSED );
}


inline void Cpu6502::SetAluFlags( const half value )
{
	SetProcessorStatus( STATUS_ZERO,		CheckZero( value ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( value ) );
}


inline bool Cpu6502::CheckSign( const half checkValue )
{
	return ( checkValue & 0x0080 );
}


inline bool Cpu6502::CheckCarry( const half checkValue )
{
	return ( checkValue > 0x00ff );
}


inline bool Cpu6502::CheckZero( const half checkValue )
{
	return ( checkValue == 0 );
}


inline bool Cpu6502::CheckOverflow( const half src, const half temp, const byte finalValue )
{
	const byte signedBound = 0x80;
	return CheckSign( finalValue ^ src ) && CheckSign( temp ^ src ) && !CheckCarry( temp );
}


byte Cpu6502::SEC( const InstrParams& params )
{
	SetProcessorStatus( STATUS_CARRY, true );

	return params.cycles;
}


byte Cpu6502::SEI( const InstrParams& params )
{
	SetProcessorStatus( STATUS_INTERRUPT, true );

	return params.cycles;
}


byte Cpu6502::SED( const InstrParams& params )
{
	SetProcessorStatus( STATUS_DECIMAL, true );

	return params.cycles;
}


byte Cpu6502::CLC( const InstrParams& params )
{
	SetProcessorStatus( STATUS_CARRY, false );

	return params.cycles;
}


byte Cpu6502::CLI( const InstrParams& params )
{
	SetProcessorStatus( STATUS_INTERRUPT, false );

	return params.cycles;
}


byte Cpu6502::CLV( const InstrParams& params )
{
	SetProcessorStatus( STATUS_OVERFLOW, false );

	return params.cycles;
}


byte Cpu6502::CLD( const InstrParams& params )
{
	SetProcessorStatus( STATUS_DECIMAL, false );

	return params.cycles;
}


inline byte& Cpu6502::Read( const InstrParams& params )
{
	word address = 0x00;

	const AddrFunction getAddr = params.getAddr;

	byte& M = ( this->*getAddr )( params, address );

	return M;
}


inline void Cpu6502::Write( const InstrParams& params, const byte value )
{
	assert( params.getAddr != &Cpu6502::Immediate );

	word address = 0x00;

	const AddrFunction getAddr = params.getAddr;

	byte& M = ( this->*getAddr )( params, address );

	M = value;

#if DEBUG_MEM == 1
	memoryDebug[address] = value;
#endif // #if DEBUG_MEM == 1
}


byte Cpu6502::CMP( const InstrParams& params )
{
	const half result = A - Read( params );

	SetProcessorStatus( STATUS_CARRY, !CheckCarry( result ) );
	SetAluFlags( result );

	return params.cycles;
}


byte Cpu6502::CPX( const InstrParams& params )
{
	const half result = X - Read( params );

	SetProcessorStatus( STATUS_CARRY, !CheckCarry( result ) );
	SetAluFlags( result );

	return params.cycles;
}


byte Cpu6502::CPY( const InstrParams& params )
{
	const half result = Y - Read( params );

	SetProcessorStatus( STATUS_CARRY, !CheckCarry( result ) );
	SetAluFlags( result );

	return params.cycles;
}


inline byte& Cpu6502::GetStack()
{
	return memory[StackBase + SP];
}


inline byte& Cpu6502::GetMemory( const half address )
{
	assert( !( ( address >= 0x0800 ) && ( address < 0x2000 ) ) );
	assert( !( ( address >= 0x2800 ) && ( address < 0x4000 ) ) );
	// TODO: Implement mirroring, etc
	return memory[address];
}


inline void Cpu6502::SetMemory( const half address, byte value )
{
	assert( !( ( address >= 0x0800 ) && ( address < 0x2000 ) ) );
	assert( !( ( address >= 0x2800 ) && ( address < 0x4000 ) ) );

#if DEBUG_MODE == 1
	if ( ( address == 0x4014 || address == 0x2003 ) || ( ( address >= 0x2000 ) && ( address <= 0x2008) ) )
	{
		cout << "DMA/PPU Registers Address Hit" << endl;
	}
#endif // #if DEBUG_MODE == 1

	// TODO: Implement mirroring, etc
	memory[address] = value;
}


inline half Cpu6502::CombineIndirect( const byte lsb, const byte msb, const word wrap )
{
	const half address	= Combine( lsb, msb );
	const byte loByte	= GetMemory( address % wrap );
	const byte hiByte	= GetMemory( ( address + 1 ) % wrap );
	const half value	= Combine( loByte, hiByte );

	return value;
}


byte& Cpu6502::IndexedIndirect( const InstrParams& params, word& address )
{
	const byte targetAddress = ( params.param0 + X );
	address = CombineIndirect( targetAddress, 0x00, ZeroPageWrap );

	byte& value = GetMemory( address );

	DEBUG_ADDR_INDEXED_INDIRECT

	return value;
}


byte& Cpu6502::IndirectIndexed( const InstrParams& params, word& address )
{
	address = CombineIndirect( params.param0, 0x00, ZeroPageWrap );

	const half offset = ( address + Y ) % MemoryWrap;

	byte& value = GetMemory( offset );

	DEBUG_ADDR_INDIRECT_INDEXED

	return value;
}


byte& Cpu6502::Absolute( const InstrParams& params, word& address )
{
	address = Combine( params.param0, params.param1 );

	byte& value = GetMemory( address );

	DEBUG_ADDR_ABS

	return value;
}


inline byte& Cpu6502::IndexedAbsolute( const InstrParams& params, word& address, const byte& reg )
{
	const half targetAddresss = Combine( params.param0, params.param1 );

	address = ( targetAddresss + reg ) % MemoryWrap;

	byte& value = GetMemory( address );

	DEBUG_ADDR_INDEXED_ABS

	return value;
}


byte& Cpu6502::IndexedAbsoluteX( const InstrParams& params, word& address )
{
	return IndexedAbsolute( params, address, X );
}


byte& Cpu6502::IndexedAbsoluteY( const InstrParams& params, word& address )
{
	return IndexedAbsolute( params, address, Y );
}


byte& Cpu6502::Zero( const InstrParams& params, word& address )
{
	const half targetAddresss = Combine( params.param0, 0x00 );

	address = ( targetAddresss % ZeroPageWrap );

	byte& value = GetMemory( address );

	DEBUG_ADDR_ZERO

	return value;
}


inline byte& Cpu6502::IndexedZero( const InstrParams& params, word& address, const byte& reg )
{
	const half targetAddresss = Combine( params.param0, 0x00 );

	address = ( targetAddresss + reg ) % ZeroPageWrap;

	byte& value = GetMemory( address );

	DEBUG_ADDR_INDEXED_ZERO

	return value;
}


byte& Cpu6502::IndexedZeroX( const InstrParams& params, word& address )
{
	return IndexedZero( params, address, X );
}


byte& Cpu6502::IndexedZeroY( const InstrParams& params, word& address )
{
	return IndexedZero( params, address, Y );
}


byte& Cpu6502::Immediate( const InstrParams& params, word& address )
{
	// This is a bit dirty, but necessary to maintain uniformity, asserts have been placed at lhs usages for now
	byte& value = const_cast< InstrParams& >( params ).param0;

	DEBUG_ADDR_IMMEDIATE

	return value;
}


byte& Cpu6502::Accumulator( const InstrParams& params, word& address )
{
	DEBUG_ADDR_ACCUMULATOR

	return A;
}


byte Cpu6502::LDA( const InstrParams& params )
{
	A = Read( params );

	SetAluFlags( A );

	return params.cycles;
}


byte Cpu6502::LDX( const InstrParams& params )
{
	X = Read( params );

	SetAluFlags( X );

	return params.cycles;
}


byte Cpu6502::LDY( const InstrParams& params )
{
	Y = Read( params );

	SetAluFlags( Y );

	return params.cycles;
}


byte Cpu6502::STA( const InstrParams& params )
{
	Write( params, A );

	return params.cycles;
}


byte Cpu6502::STX( const InstrParams& params )
{
	Write( params, X );

	return params.cycles;
}


byte Cpu6502::STY( const InstrParams& params )
{
	Write( params, Y );

	return params.cycles;
}


byte Cpu6502::TXS( const InstrParams& params )
{
	SP = X;

	return params.cycles;
}


byte Cpu6502::TXA( const InstrParams& params )
{
	A = X;
	SetAluFlags( A );

	return params.cycles;
}


byte Cpu6502::TYA( const InstrParams& params )
{
	A = Y;
	SetAluFlags( A );

	return params.cycles;
}


byte Cpu6502::TAX( const InstrParams& params )
{
	X = A;
	SetAluFlags( X );

	return params.cycles;
}


byte Cpu6502::TAY( const InstrParams& params )
{
	Y = A;
	SetAluFlags( Y );

	return params.cycles;
}


byte Cpu6502::TSX( const InstrParams& params )
{
	X = SP;
	SetAluFlags( X );

	return params.cycles;
}


byte Cpu6502::ADC( const InstrParams& params )
{
	// http://nesdev.com/6502.txt, INSTRUCTION OPERATION - ADC
	const byte M		= Read( params );
	const half src		= A;
	const half carry	= ( P & STATUS_CARRY ) ? 1 : 0;
	const half temp		= A + M + carry;

	A = ( temp & 0xFF );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( temp ) );
	SetProcessorStatus( STATUS_OVERFLOW,	CheckOverflow( M, temp, A ) );
	SetAluFlags( A );
	SetProcessorStatus( STATUS_CARRY,		( temp > 0xFF ) );

	return params.cycles;
}


byte Cpu6502::SBC( const InstrParams& params )
{
	const byte& M		= Read( params );
	const half carry	= ( P & STATUS_CARRY ) ? 0 : 1;
	const half result	= A - M - carry;

	SetAluFlags( result );
	SetProcessorStatus( STATUS_OVERFLOW,	CheckSign( A ^ result ) && CheckSign( A ^ M ) );
	SetProcessorStatus( STATUS_CARRY,		!CheckCarry( result ) );

	A = result & 0xFF;

	return params.cycles;
}


byte Cpu6502::INX( const InstrParams& params )
{
	X++;
	SetAluFlags( X );

	return params.cycles;
}


byte Cpu6502::INY( const InstrParams& params )
{
	Y++;
	SetAluFlags( Y );

	return params.cycles;
}


byte Cpu6502::DEX( const InstrParams& params )
{
	X--;
	SetAluFlags( X );

	return params.cycles;
}


byte Cpu6502::DEY( const InstrParams& params )
{
	Y--;
	SetAluFlags( Y );

	return params.cycles;
}


byte Cpu6502::INC( const InstrParams& params )
{
	const byte result = Read( params ) + 1;

	Write( params, result );

	SetAluFlags( result );

	return params.cycles;
}


byte Cpu6502::DEC( const InstrParams& params )
{
	const byte result = Read( params ) - 1;

	Write( params, result );

	SetAluFlags( result );

	return params.cycles;
}


void Cpu6502::PushToStack( const byte value )
{
	GetStack() = value;
	SP--;
}


byte Cpu6502::PopFromStack()
{
	SP++;
	return GetStack();
}


byte Cpu6502::PHP( const InstrParams& params )
{
	PushToStack( P | STATUS_UNUSED | STATUS_BREAK );

	return params.cycles;
}


byte Cpu6502::PHA( const InstrParams& params )
{
	PushToStack( A );

	return params.cycles;
}


byte Cpu6502::PLA( const InstrParams& params )
{
	A = PopFromStack();

	SetAluFlags( A );

	return params.cycles;
}


byte Cpu6502::PLP( const InstrParams& params )
{
	// https://wiki.nesdev.com/w/index.php/Status_flags
	const byte status = ~STATUS_BREAK & PopFromStack();
	P = status | ( P & STATUS_BREAK ) | STATUS_UNUSED;

	return params.cycles;
}


byte Cpu6502::NOP( const InstrParams& params )
{
	return params.cycles;
}


byte Cpu6502::ASL( const InstrParams& params )
{
	const byte& M = Read( params );

	SetProcessorStatus( STATUS_CARRY,		M & 0x80 );
	Write( params, M << 1 );
	SetAluFlags( M );

	return params.cycles;
}


byte Cpu6502::LSR( const InstrParams& params )
{
	const byte& M = Read( params );

	SetProcessorStatus( STATUS_CARRY,		M & 0x01 );
	Write( params, M >> 1 );
	SetAluFlags( M );

	return params.cycles;
}


byte Cpu6502::AND( const InstrParams& params )
{
	A &= Read( params );

	SetAluFlags( A );

	return params.cycles;
}


byte Cpu6502::BIT( const InstrParams& params )
{
	const byte& M = Read( params );

	SetProcessorStatus( STATUS_ZERO,		!( A & M ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( M ) );
	SetProcessorStatus( STATUS_OVERFLOW,	M & 0x40 );

	return params.cycles;
}


byte Cpu6502::EOR( const InstrParams& params )
{
	A ^= Read( params );

	SetAluFlags( A );

	return params.cycles;
}


byte Cpu6502::ORA( const InstrParams& params )
{
	A |= Read( params );

	SetAluFlags( A );

	return params.cycles;
}


byte Cpu6502::JMP( const InstrParams& params )
{
	PC = Combine( params.param0, params.param1 );

	DEBUG_ADDR_JMP

	return params.cycles;
}


byte Cpu6502::JMPI( const InstrParams& params )
{
	const half addr0 = Combine( params.param0, params.param1 );

	// Hardware bug - http://wiki.nesdev.com/w/index.php/Errata
	if ( ( addr0 & 0xff ) == 0xff )
	{
		const half addr1 = Combine( 0x00, params.param1 );

		PC = ( Combine( GetMemory( addr0 ), GetMemory( addr1 ) ) );
	}
	else
	{
		PC = ( Combine( GetMemory( addr0 ), GetMemory( addr0 + 1 ) ) );
	}

	DEBUG_ADDR_JMPI

	return params.cycles;
}


byte Cpu6502::JSR( const InstrParams& params )
{
	half retAddr = PC - 1;

	PushToStack( ( retAddr >> 8 ) & 0xFF );
	PushToStack( retAddr & 0xFF );

	PC = Combine( params.param0, params.param1 );

	DEBUG_ADDR_JSR

	return params.cycles;
}


byte Cpu6502::BRK( const InstrParams& params )
{
	assert( 0 );
	SetProcessorStatus( STATUS_BREAK, true );

	return params.cycles;
}


byte Cpu6502::RTS( const InstrParams& params )
{
	const byte loByte = PopFromStack();
	const byte hiByte = PopFromStack();

	PC = 1 + Combine( loByte, hiByte );

	return params.cycles;
}


byte Cpu6502::RTI( const InstrParams& params )
{
	PLP( params );

	const byte loByte = PopFromStack();
	const byte hiByte = PopFromStack();

	PC	= Combine( loByte, hiByte );

	return params.cycles;
}


inline void Cpu6502::Branch( const InstrParams& params, const bool takeBranch )
{
	const half branchedPC = PC + static_cast< byte_signed >( params.param0 );

	if ( takeBranch )
	{
		PC = branchedPC;
	}
	else
	{
		// cycles
	}

#if DEBUG_MODE == 1
	debugAddr << uppercase << "$" << setfill( '0' ) << setw( 2 ) << hex << branchedPC;
#endif // #if DEBUG_MODE == 1
}


byte Cpu6502::BMI( const InstrParams& params )
{
	Branch( params, ( P & STATUS_NEGATIVE ) );

	return params.cycles;
}


byte Cpu6502::BVS( const InstrParams& params )
{
	Branch( params, ( P & STATUS_OVERFLOW ) );

	return params.cycles;
}


byte Cpu6502::BCS( const InstrParams& params )
{
	Branch( params, ( P & STATUS_CARRY ) );

	return params.cycles;
}


byte Cpu6502::BEQ( const InstrParams& params )
{
	Branch( params, ( P & STATUS_ZERO ) );

	return params.cycles;
}


byte Cpu6502::BPL( const InstrParams& params )
{
	Branch( params, !( P & STATUS_NEGATIVE ) );

	return params.cycles;
}


byte Cpu6502::BVC( const InstrParams& params )
{
	Branch( params, !( P & STATUS_OVERFLOW ) );

	return params.cycles;
}


byte Cpu6502::BCC( const InstrParams& params )
{
	Branch( params, !( P & STATUS_CARRY ) );

	return params.cycles;
}


byte Cpu6502::BNE( const InstrParams& params )
{
	Branch( params, !( P & STATUS_ZERO ) );

	return params.cycles;
}


byte Cpu6502::ROL( const InstrParams& params )
{
	half temp = Read( params ) << 1;
	temp = ( P & STATUS_CARRY ) ? temp | 0x0001 : temp;
	
	SetProcessorStatus( STATUS_CARRY, CheckCarry( temp ) );
	
	temp &= 0xFF;

	SetAluFlags( temp );

	Read( params ) = static_cast< byte >( temp & 0xFF );

	return params.cycles;
}


byte Cpu6502::ROR( const InstrParams& params )
{
	half temp = ( P & STATUS_CARRY ) ? Read( params ) | 0x0100 : Read( params );
	
	SetProcessorStatus( STATUS_CARRY, temp & 0x01 );

	temp >>= 1;

	SetAluFlags( temp );

	Read( params ) = static_cast< byte >( temp & 0xFF );

	return params.cycles;
}


byte Cpu6502::Illegal( const InstrParams& params )
{
	//assert( 0 );

	return 0;
}


bool Cpu6502::IsIllegalOp( const byte opCode )
{
	// These are explicitly excluded since the instruction map code will generate garbage for these ops
	const byte illegalOpFormat = 0x03;
	const byte numIllegalOps = 23;
	static const byte illegalOps[numIllegalOps]
	{
		0x02, 0x22, 0x34, 0x3C, 0x42, 0x44, 0x54, 0x5C,
		0x62, 0x64, 0x74, 0x7C, 0x80, 0x82, 0x89, 0x9C,
		0x9E, 0xC2, 0xD4, 0xDC, 0xE2, 0xF4, 0xFC,
	};


	if ( ( opCode & illegalOpFormat ) == illegalOpFormat )
		return true;

	for ( byte op = 0; op < numIllegalOps; ++op )
	{
		if ( opCode == illegalOps[op] )
			return true;
	}

	return false;
}


inline bool Cpu6502::Step()
{
	static half brkPt = 0x377e;
	static bool enablePrinting = true;

	const half instrBegin = PC;

	const byte curByte = GetMemory( instrBegin );

#if DEBUG_MODE == 1
	if ( PC == brkPt )
	{
		cout << "Breakpoint Hit" << endl;
		enablePrinting = true;
	}

	debugAddr.str( std::string() );
	string regStr;
	RegistersToString( *this, regStr );
#endif // #if DEBUG_MODE == 1

	PC++;

	if ( static_cast<OpCode>(curByte) == OpCode::BRK )
		return false;

	const InstructionMapTuple& pair = InstructionMap[curByte];

	const byte operands		= pair.operands;
	InstrParams params;

	if ( operands >= 1 )
	{
		params.param0 = GetMemory( PC++ );
	}

	if ( operands == 2 )
	{
		params.param1 = GetMemory( PC++ );
	}

	Instruction instruction = pair.instr;

	params.getAddr = pair.addrFunc;
	( this->*instruction )( params );

	//DEBUG_CPU_LOG

	if ( enablePrinting ) {
		
			int disassemblyBytes[6] = { curByte, params.param0, params.param1,'\0' }; 
			stringstream hexString; 
			stringstream logLine; 
			if ( operands == 1 ) 
				hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0] << " " << setw( 2 ) << disassemblyBytes[1]; 
			else if ( operands == 2 ) 
				hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0] << " " << setw( 2 ) << disassemblyBytes[1] << " " << setw( 2 ) << disassemblyBytes[2]; 
			else 
				hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0]; 
				logLine << uppercase << setfill( '0' ) << setw( 4 ) << hex << instrBegin << setfill( ' ' ) << "  " << setw( 10 ) << left << hexString.str() << pair.mnemonic << " " << setw( 28 ) << left << debugAddr.str() << right << regStr;
				logFile << logLine.str() << endl; 
				//cout << logLine.str() << endl;
	}

	return true;
}


bool Cpu6502::Run()
{
#if DEBUG_MODE == 1
	logFile.open( "tomTendo.log" );
#endif // #if DEBUG_MODE == 1

	bool isRunning = true;

	while ( isRunning )
	{
		isRunning = Step();

		// HACK
		static bool forceSetPPUFlag = false;
		if( forceSetPPUFlag )
			memory[0x2002] = 0x80;
	}

#if DEBUG_MODE == 1
	logFile.close();
#endif // #if DEBUG_MODE == 1

	return true;
}