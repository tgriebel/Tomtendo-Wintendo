#pragma once

#include <iostream>
#include <bitset>
#include <map>
#include "types_common.h"
#include "6502.h"
using namespace std;

std::map<half, byte> memoryDebug;

void RecordMemoryWrite( const half address, const byte value )
{
	memoryDebug[address] = value;
}

void PrintRegisters( const Cpu6502& cpu )
{
	cout << "A: " << hex << static_cast<int>( cpu.A ) << "\t";
	cout << "X: " << hex << static_cast<int>( cpu.X ) << "\t";
	cout << "Y: " << hex << static_cast<int>( cpu.Y ) << "\t";
	cout << "SP: " << hex << static_cast<int>( cpu.SP ) << "\t";
	cout << "PC: " << hex << static_cast<int>( cpu.PC ) << "\t";

	bitset<8> p( cpu.P );

	cout << "\tP: NV-BDIZC" << "\t" << p;
}


void PrintInstructionMap( const InstructionMapTuple instructionMap[] )
{
	cout << "Instruction | Opcode | Address Mode | Cycles " << endl;

	for ( int instr = 0; instr < 256; ++instr )
	{
		if ( instructionMap[instr].mnemonic[0] )
			cout << hex << instr << " " << instructionMap[instr].mnemonic << " " << instructionMap[instr].descriptor << " " << (int)instructionMap[instr].cycles << endl;
		else
			cout << hex << instr << endl;
	}
}