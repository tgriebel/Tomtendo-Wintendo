
#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <assert.h>
#include "types_common.h"
#include "utility.h"
#include "6502.h"
#include "debug.h"
#include "bitmap.h"

using namespace std;
using namespace chrono;
using namespace this_thread;


inline void SetProcessorStatus( const StatusBit bit, const bool toggleOn )
{
	cpu.P &= ~bit;

	if ( !toggleOn )
		return;

	cpu.P |= ( bit | STATUS_UNUSED | STATUS_BREAK );
}


inline bool CheckSign( const half checkValue )
{
	return ( checkValue & 0x0080 );
}


inline bool CheckCarry( const half checkValue )
{
	return ( checkValue > 0x00ff );
}


inline bool CheckZero( const half checkValue )
{
	return ( checkValue == 0 );
}


byte Instr::SetStatusBit( const InstrParams& params )
{
	SetProcessorStatus( static_cast<StatusBit>( params.param0 ), true );

	return params.cycles;
}


byte Instr::ClearStatusBit( const InstrParams& params )
{
	SetProcessorStatus( static_cast<StatusBit>( params.param0 ), false );

	return params.cycles;
}


static inline byte Read( const InstrParams& params )
{
	word address = 0x00;
	byte& M = params.getAddr( params, address );

	return M;
}


static inline void Write( const InstrParams& params, const byte value )
{
	assert( params.getAddr != Instr::Immediate );

	word address = 0x00;
	byte& M = params.getAddr( params, address );

	M = value;

	memoryDebug[address] = value;
}


byte Instr::Compare( const InstrParams& params )
{
	assert( params.reg1 != iNil );

	const byte_signed result = cpu.registers[ params.reg1 ] - Read( params );

	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( result ) );
	SetProcessorStatus( STATUS_CARRY,		!CheckSign( result ) );
	SetProcessorStatus( STATUS_ZERO,		CheckZero( result ) );

	return params.cycles;
}


inline half CombineIndirect( const byte lsb, const byte msb )
{
	const half index = Combine( lsb, msb );

	return ( Combine( cpu.memory[index], cpu.memory[( index + 1 ) & 0xFF] ) );
}


byte& Instr::Indirect( const InstrParams& params, word& address )
{
	address = CombineIndirect( params.param0, params.param1 );

	return cpu.memory[address];
}


byte& Instr::IndexedIndirect( const InstrParams& params, word& address )
{
	address = CombineIndirect( params.param0 + cpu.registers[params.reg0], 0x00 );

	return cpu.memory[address];
}


byte& Instr::IndirectIndexed( const InstrParams& params, word& address )
{
	address = CombineIndirect( params.param0, 0x00 ) + cpu.registers[params.reg0];

	return cpu.memory[address];
}


byte& Instr::Absolute( const InstrParams& params, word& address )
{
	address = Combine( params.param0, params.param1 );

	return cpu.memory[address];
}


byte& Instr::IndexedAbsolute( const InstrParams& params, word& address )
{
	address = Combine( params.param0, params.param1 ) + cpu.registers[ params.reg0 ];

	return cpu.memory[address];
}


byte& Instr::Zero( const InstrParams& params, word& address )
{
	address = Combine( params.param0, 0x00 );

	return cpu.memory[address];
}


byte& Instr::IndexedZero( const InstrParams& params, word& address )
{
	address = Combine( params.param1, 0x00 ) + cpu.registers[params.reg0];

	return cpu.memory[address];
}


byte& Instr::Immediate( const InstrParams& params, word& address )
{
	// This is a bit dirty, but necessary to maintain uniformity, asserts have been placed at lhs usages for now
	return const_cast< InstrParams& >( params ).param0;
}


byte& Instr::Accumulator( const InstrParams& params, word& address )
{
	return cpu.A;
}


byte Instr::Load( const InstrParams& params )
{
	cpu.registers[params.reg1] = Read( params );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.registers[params.reg1] ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.registers[params.reg1] ) );

	return params.cycles;
}


byte Instr::Store( const InstrParams& params )
{
	Write( params, cpu.registers[params.reg1] );

	return params.cycles;
}


byte Instr::Move( const InstrParams& params )
{
	cpu.registers[params.reg1] = cpu.registers[params.reg0];

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.registers[params.reg1] ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.registers[params.reg1] ) );

	return params.cycles;
}


byte Instr::ADC( const InstrParams& params )
{
	const byte& M = Read( params );
	const half result = cpu.A + M + ( cpu.PC & BIT_CARRY );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( result ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( result ) );
	SetProcessorStatus( STATUS_OVERFLOW,	!( CheckSign( cpu.A ^ M ) && CheckSign( cpu.A ^ result ) ) );
	SetProcessorStatus( STATUS_CARRY,		CheckCarry( result ) );

	cpu.A = result & 0xFF;

	return params.cycles;
}


byte Instr::SBC( const InstrParams& params )
{
	const byte& M = Read( params );
	const half result = cpu.A - M - ~( cpu.PC & BIT_CARRY );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( result ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( result ) );
	SetProcessorStatus( STATUS_OVERFLOW,	CheckSign( cpu.A ^ result ) && CheckSign( cpu.A ^ M ) );
	SetProcessorStatus( STATUS_CARRY,		!CheckCarry( result ) );

	cpu.A = result & 0xFF;

	return params.cycles;
}


byte Instr::IncReg( const InstrParams& params )
{
	cpu.registers[params.reg0]++;

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.registers[params.reg0] ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.registers[params.reg0] ) );

	return params.cycles;
}


byte Instr::DecReg( const InstrParams& params )
{
	cpu.registers[params.reg0]--;

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.registers[params.reg0] ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.registers[params.reg0] ) );

	return params.cycles;
}


byte Instr::INC( const InstrParams& params )
{
	const byte& M = Read( params );

	Write( params, M + 1 ); 

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.registers[params.reg0] ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.registers[params.reg0] ) );

	return params.cycles;
}


byte Instr::DEC( const InstrParams& params )
{
	const byte& M = Read( params );

	Write( params, M - 1 );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.registers[params.reg0] ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.registers[params.reg0] ) );

	return params.cycles;
}


void PushToStack( const byte value )
{
	cpu.memory[Cpu6502::stackbase + cpu.SP] = value;
	cpu.SP--;
}


byte PopFromStack()
{
	cpu.SP++;
	return cpu.memory[Cpu6502::stackbase + cpu.SP];
}


byte Instr::Push( const InstrParams& params )
{
	PushToStack( cpu.registers[params.reg0] );

	return params.cycles;
}


byte Instr::Pop( const InstrParams& params )
{
	cpu.registers[params.reg0] = PopFromStack();

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.registers[params.reg0] ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.registers[params.reg0] ) );

	return params.cycles;
}


byte Instr::PLP( const InstrParams& params )
{
	cpu.P = STATUS_UNUSED | PopFromStack();

	return params.cycles;
}


byte Instr::NOP( const InstrParams& params )
{
	return params.cycles;
}


byte Instr::ASL( const InstrParams& params )
{
	const byte& M = Read( params );

	SetProcessorStatus( STATUS_CARRY,		M & 0x80 );
	Write( params, M << 1 );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( M ) );
	SetProcessorStatus( STATUS_ZERO,		CheckZero( M ) );

	return params.cycles;
}


byte Instr::LSR( const InstrParams& params )
{
	const byte& M = Read( params );

	SetProcessorStatus( STATUS_CARRY,		M & 0x01 );
	Write( params, M >> 1 );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( M ) );
	SetProcessorStatus( STATUS_ZERO,		CheckZero( M ) );

	return params.cycles;
}


byte Instr::AND( const InstrParams& params )
{
	cpu.A &= Read( params );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.A ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.A ) );

	return params.cycles;
}


byte Instr::BIT( const InstrParams& params )
{
	const byte& M = Read( params );

	SetProcessorStatus( STATUS_ZERO,		!( cpu.A & M ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( M ) );
	SetProcessorStatus( STATUS_OVERFLOW,	M & 0x40 );

	return params.cycles;
}


byte Instr::EOR( const InstrParams& params )
{
	cpu.A ^= Read( params );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.A ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.A ) );

	return params.cycles;
}


byte Instr::ORA( const InstrParams& params )
{
	cpu.A |= Read( params );

	SetProcessorStatus( STATUS_ZERO,		CheckZero( cpu.A ) );
	SetProcessorStatus( STATUS_NEGATIVE,	CheckSign( cpu.A ) );

	return params.cycles;
}


byte Instr::JMP( const InstrParams& params )
{
	cpu.PC = Combine( params.param0, params.param1 );

	return params.cycles;
}


byte Instr::JMPI( const InstrParams& params )
{
	cpu.PC = CombineIndirect( params.param0, params.param1 );

	return params.cycles;
}


byte Instr::JSR( const InstrParams& params )
{
	half retAddr = cpu.PC - 1;

	PushToStack( retAddr & 0xFF );
	PushToStack( ( retAddr >> 8 ) & 0xFF );

	cpu.PC = Combine( params.param0, params.param1 );

	return params.cycles;
}


byte Instr::BRK( const InstrParams& params )
{
	assert( 0 );
	SetProcessorStatus( STATUS_BREAK, true );

	return params.cycles;
}


byte Instr::RTS( const InstrParams& params )
{
	cpu.PC = 1 + Combine( PopFromStack(), PopFromStack() );

	return params.cycles;
}


byte Instr::RTI( const InstrParams& params )
{
	cpu.P	= STATUS_UNUSED | PopFromStack();
	cpu.PC	= PopFromStack();

	return params.cycles;
}


inline void Branch( const InstrParams& params, const bool takeBranch )
{
	if ( takeBranch )
	{
		cpu.PC += static_cast< byte_signed >( params.param0 );
	}
	else
	{
		// cycles
	}
}


byte Instr::BranchOnSet( const InstrParams& params )
{
	Branch( params, ( cpu.P & params.statusFlag ) );

	return params.cycles;
}


byte Instr::BranchOnClear( const InstrParams& params )
{
	Branch( params, !( cpu.P & params.statusFlag ) );

	return params.cycles;
}


byte Instr::ROL( const InstrParams& params )
{
	const byte temp = ( cpu.P & STATUS_CARRY );

	cpu.registers[params.reg0] <<= 1;
	cpu.P = ( cpu.registers[params.reg0] & 0x01 ) << 7;
	cpu.registers[params.reg0] |= temp;

	return params.cycles;
}


byte Instr::ROR( const InstrParams& params )
{
	const byte temp = ( cpu.P & STATUS_CARRY ) << 7;

	cpu.registers[params.reg0] >>= 1;
	cpu.P = cpu.registers[params.reg0] & 0x01;
	cpu.registers[params.reg0] |= temp;

	return params.cycles;
}


bool IsIllegalOp( const byte opCode )
{
	if ( ( opCode & illegalOpFormat ) == illegalOpFormat )
		return true;

	for ( byte op = 0; op < numIllegalOps; ++op )
	{
		if ( opCode == illegalOps[op] )
			return true;
	}

	return false;
}





int main()
{
	cout << "Building Instruction Map" << endl;

	for ( int instr = 0; instr < 256; ++instr )
	{
		OpCodeMap opCode;
		AddrMapTuple addrCode = { 0x00, "",	"", NULL, iNil, 0, 0 };

		bool found = false;

		if ( IsIllegalOp( instr ) )
			continue;

		for ( byte op = 0; op < numOpCodes; ++op )
		{
			if ( ( instr & opMap[op].format ) == opMap[op].byteCode )
			{
				opCode = opMap[op];

				found = ( opCode.byteCode == instr );

				if ( opMap[op].format == ADDR_OP )
				{
					for ( int addr = 0; addr < numAddrModes; ++addr )
					{
						if ( ( instr & 0x1F ) == addrMap[addr].byteCode )
						{
							addrCode = addrMap[addr];
							found = instr == ( opCode.byteCode | addrCode.byteCode );
							break;
						}
					}
				}

				if ( found )
					break;
			}
		}
		
		if ( !found )
			continue;

		instructionMap[instr].instr = opCode.instr;
		instructionMap[instr].addrFunc = addrCode.addrFunc;

		if ( addrCode.addrFunc )
		{
			instructionMap[instr].params.reg0 = addrCode.reg;
			instructionMap[instr].params.reg1 = opCode.reg0;
		}
		else
		{
			instructionMap[instr].params.reg0 = opCode.reg0;
			instructionMap[instr].params.reg1 = opCode.reg1;
		}

		if ( instructionMap[instr].params.reg0 == iX && instructionMap[instr].params.reg0 == instructionMap[instr].params.reg1 )
		{
			instructionMap[instr].params.reg0 = iY;
		}

		instructionMap[instr].operands = opCode.operands + addrCode.operands;
		instructionMap[instr].params.getAddr = addrCode.addrFunc;
		instructionMap[instr].params.statusFlag = opCode.bit;

		instructionMap[instr].cycles = opCode.cycles + addrCode.cycles;

		strcpy_s( instructionMap[instr].mnemonic,	opCode.mnemonic );
		strcpy_s( instructionMap[instr].descriptor, addrCode.descriptor );

		strcpy_s( disassemblerMap[instr].opCode, opCode.mnemonic );
		strcpy_s( disassemblerMap[instr].addrFormat, addrCode.format );
		disassemblerMap[instr].length = opCode.operands + addrCode.operands + 1;
	}

	// PrintInstructionMap( instructionMap );
	// TODO: fill instruction with NOP on missing instr
	cout << "Loading Program" << endl;

	ifstream programFile;
	programFile.open( "6502_functional_test.bin", ios::binary );
	std::string str( ( istreambuf_iterator< char >( programFile ) ), istreambuf_iterator< char >() );
	programFile.close();

	NesCart cart;
	LoadNesFile( "testNes.nes", cart );

	cpu.Reset();

	Bitmap image( 32, 32, 0x00 );

	//memcpy( cpu.memory + cpu.codebase, cart.rom, 1024 );
	memcpy( cpu.memory + cpu.codebase, str.c_str(), str.length() );

	cout << "Executing Program" << endl;

	nanoseconds clockDuration( 599 );

	long long cycleCount = 0;

	while ( true )
	{
		++cycleCount;

		int disassemblyBytes[6] = { '\0','\0','\0','\0' };

		half instrBegin = cpu.PC;

		byte curByte = cpu.memory[instrBegin];

		cpu.PC++;

		disassemblyBytes[0] = curByte;

		if ( curByte == BRK )
			break;

		InstructionMapTuple pair = instructionMap[curByte];

		byte operands = pair.operands;

		if ( operands >= 1 )
		{
			pair.params.param0 = cpu.memory[cpu.PC++];
			disassemblyBytes[1] = pair.params.param0;
		}

		if ( operands == 2 )
		{
			pair.params.param1 = cpu.memory[cpu.PC++];
			disassemblyBytes[2] = pair.params.param1;
		}

		pair.instr( pair.params );

		char addrString[32];
		char hexString[32];

		memset( addrString, 0, 32 );
		memset( hexString, 0, 32 );

		word addr = ( pair.params.param1 << 8 ) | pair.params.param0;

		sprintf_s( addrString, disassemblerMap[curByte].addrFormat, addr );

		if ( operands == 1 )
			sprintf_s( hexString, "%02x %02x   ", disassemblyBytes[0], disassemblyBytes[1] );
		else if ( operands == 2 )
			sprintf_s( hexString, "%02x %02x %02x", disassemblyBytes[0], disassemblyBytes[1], disassemblyBytes[2] );
		else
			sprintf_s( hexString, "%02x      ", disassemblyBytes[0] );

		cout << hex << instrBegin << " | " << hexString << " | " << disassemblerMap[curByte].opCode << " " << setw( 8 ) << addrString << " | ";

		PrintRegisters( cpu );

		cout << endl;

		// cycle
	}

	uint colorIndex[] = {	0x000000FF, 0xFFFFFFFF,	0x880000FF, 0xAAFFEEFF,
							0xCC44CCFF, 0x00CC55FF, 0x0000AAFF, 0xEEEE77FF,
							0xDD8855FF, 0x664400FF, 0xFF7777FF, 0x333333FF,
							0x777777FF, 0xAAFF66FF, 0x0088FFFF, 0xBBBBBBFF };

	for ( std::map< half, byte>::iterator it = memoryDebug.begin(); it != memoryDebug.end(); ++it )
	{
		const uint x = ( it->first - 0x200 ) % 32;
		const uint y = ( it->first - 0x200 ) / 32;

		image.setPixel( x, 31 - y, colorIndex[it->second%16] );
		// cout << it->first << " => " << it->second << '\n';
	}

	image.write( "output.bmp" );

	cout << "Finished. Memory writes: " << memoryDebug.size() << endl;

	PrintRegisters( cpu );

	char c;
	cin >> c;
	return 0;
}

