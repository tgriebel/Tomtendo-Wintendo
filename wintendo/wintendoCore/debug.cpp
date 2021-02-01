
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include "debug.h"
#include "mos6502.h"

void OpDebugInfo::ToString( std::string& buffer, bool registerDebug )
{
	std::stringstream debugStream;

	int disassemblyBytes[6] = { byteCode, op0, op1,'\0' };
	stringstream hexString;

	if ( operands == 1 )
	{
		hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0] << " " << setw( 2 ) << disassemblyBytes[1];
	}
	else if ( operands == 2 )
	{
		hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0] << " " << setw( 2 ) << disassemblyBytes[1] << " " << setw( 2 ) << disassemblyBytes[2];
	}
	else
	{
		hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0];
	}

	debugStream << uppercase << setfill( '0' ) << setw( 4 ) << hex << instrBegin << setfill( ' ' ) << "  " << setw( 10 ) << left << hexString.str() << mnemonic << " ";
	
	const addrMode_t mode = static_cast<addrMode_t>( addrMode );

	switch ( mode )
	{
	default:
	case addrMode_t::Absolute:
		debugStream << uppercase << "$" << setfill( '0' ) << setw( 4 ) << address;
		debugStream << " = " << setfill( '0' ) << setw( 2 ) << hex << static_cast<uint32_t>( memValue );
		break;

	case addrMode_t::Zero:
		debugStream << uppercase << "$" << setfill( '0' ) << setw( 2 ) << address;
		debugStream << " = " << setfill( '0' ) << setw( 2 ) << hex << static_cast<uint32_t>( memValue );
		break;

	case addrMode_t::IndexedAbsoluteX:
	case addrMode_t::IndexedAbsoluteY:
		debugStream << uppercase << "$" << setw( 4 ) << hex << targetAddress << ",";
		debugStream << isXReg ? "X" : "Y";
		debugStream << setfill( '0' ) << " @ " << setw( 4 ) << hex << address;
		debugStream << " = " << setw( 2 ) << hex << static_cast<uint32_t>( memValue );
		break;

	case addrMode_t::IndexedZeroX:
	case addrMode_t::IndexedZeroY:
		debugStream << uppercase << "$" << setw( 2 ) << hex << targetAddress << ",";
		debugStream << isXReg ? "X" : "Y";
		debugStream << setfill( '0' ) << " @ " << setw( 2 ) << hex << address;
		debugStream << " = " << setw( 2 ) << hex << static_cast<uint32_t>( memValue );
		break;

	case addrMode_t::Immediate:
		debugStream << uppercase << "#$" << setfill( '0' ) << setw( 2 ) << hex << static_cast<uint32_t>( memValue );
		break;

	case addrMode_t::IndirectIndexed:
		debugStream << uppercase << "($" << setfill( '0' ) << setw( 2 ) << static_cast<uint32_t>( operand ) << "),Y = ";
		debugStream << setw( 4 ) << hex << address;
		debugStream << " @ " << setw( 4 ) << hex << offset << " = " << setw( 2 ) << hex << static_cast<uint32_t>( memValue );
		break;

	case addrMode_t::IndexedIndirect:
		debugStream << uppercase << "($" << setfill( '0' ) << setw( 2 ) << static_cast<uint32_t>( operand ) << ",X) @ ";
		debugStream << setw( 2 ) << static_cast<uint32_t>( targetAddress );
		debugStream << " = " << setw( 4 ) << address << " = " << setw( 2 ) << static_cast<uint32_t>( memValue );
		break;

	case addrMode_t::Accumulator:
		debugStream << "A";
		break;

	case addrMode_t::Jmp:
		debugStream << uppercase << setfill( '0' ) << "$" << setw( 2 ) << hex << address;
		break;

	case addrMode_t::JmpIndirect:
		debugStream << uppercase << setfill( '0' ) << "($" << setw( 4 ) << hex << offset;
		debugStream << ") = " << setw( 4 ) << hex << address;
		break;

	case addrMode_t::Jsr:
		debugStream << uppercase << "$" << setfill( '0' ) << setw( 2 ) << hex << address;
		break;

	case addrMode_t::Branch:
		debugStream << uppercase << "$" << setfill( '0' ) << setw( 2 ) << hex << address;
		break;
	}

	if ( registerDebug )
	{
		debugStream << setfill( ' ' ) << setw( 28 ) << right;
		debugStream << uppercase << "A:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( regInfo.A ) << setw( 1 ) << " ";
		debugStream << uppercase << "X:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( regInfo.X ) << setw( 1 ) << " ";
		debugStream << uppercase << "Y:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( regInfo.Y ) << setw( 1 ) << " ";
		debugStream << uppercase << "P:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( regInfo.P ) << setw( 1 ) << " ";
		debugStream << uppercase << "SP:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( regInfo.SP ) << setw( 1 ) << " ";
		debugStream << uppercase << "PPU:" << setfill( ' ' ) << setw( 3 ) << dec << ppuCycles.count() << "," << setw( 3 ) << ( curScanline + 1 ) << " ";
		debugStream << uppercase << "CYC:" << dec << ( 7 + cpuCycles.count() ) << "\0"; // 7 is to match the init value from the nintendolator log
	}

	buffer += debugStream.str();
}