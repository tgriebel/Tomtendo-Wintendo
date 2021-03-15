
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include "debug.h"
#include "mos6502.h"

static void PrintHex( std::stringstream& debugStream, const uint32_t value, const uint32_t charCount, const bool markup )
{
	std::ios_base::fmtflags flags = debugStream.flags();
	if( markup ) {
		debugStream << "$";
	}
	debugStream << setfill( '0' ) << setw( charCount ) << uppercase << hex << right << value << left;
	debugStream.setf( flags );
}


static std::string GetOpName( const OpDebugInfo& info )
{
	std::string name;
	if( info.isIllegal ) {
		name += "*";
	} else {
		name += " ";
	}

	if( ( info.opType == (uint8_t)opType_t::SKB ) || ( info.opType == (uint8_t)opType_t::SKW ) )
	{
		name += "NOP";
	}
	else
	{
		name += info.mnemonic[ 0 ];
		name += info.mnemonic[ 1 ];
		name += info.mnemonic[ 2 ];
	}
	return name;
}


void OpDebugInfo::ToString( std::string& buffer, const bool registerDebug, const bool cycleDebug ) const
{
	std::stringstream debugStream;

	if( nmi )
	{
		debugStream << "[NMI - Cycle:" << cpuCycles << "]";
		buffer +=  debugStream.str();
		return;
	}

	if ( irq )
	{
		debugStream << "[IRQ - Cycle:" << cpuCycles << "]";
		buffer += debugStream.str();
		return;
	}

	int disassemblyBytes[6] = { byteCode, op0, op1,'\0' };
	stringstream hexString;

	if ( operands == 1 )
	{
		PrintHex( hexString, disassemblyBytes[ 0 ], 2, false );
		hexString << " ";
		PrintHex( hexString, disassemblyBytes[ 1 ], 2, false );
	}
	else if ( operands == 2 )
	{
		PrintHex( hexString, disassemblyBytes[ 0 ], 2, false );
		hexString << " ";
		PrintHex( hexString, disassemblyBytes[ 1 ], 2, false );
		hexString << " ";
		PrintHex( hexString, disassemblyBytes[ 2 ], 2, false );
	}
	else
	{
		PrintHex( hexString, disassemblyBytes[ 0 ], 2, false );
	}

	PrintHex( debugStream, instrBegin, 4, false );
	debugStream << setfill( ' ' ) << "  " << setw( 9 ) << left << hexString.str();
	debugStream << GetOpName( *this ) << " ";
	
	const addrMode_t mode = static_cast<addrMode_t>( addrMode );

	const bool isXReg = ( mode == addrMode_t::IndexedAbsoluteX ) || ( mode == addrMode_t::IndexedZeroX );

	switch ( mode )
	{
	default: break;
	case addrMode_t::Absolute:
		PrintHex( debugStream, address, 4, true );
		debugStream << " = ";
		PrintHex( debugStream, static_cast<uint32_t>( memValue ), 2, false );
		break;

	case addrMode_t::Zero:
		PrintHex( debugStream, address, 2, true );
		debugStream << " = ";
		PrintHex( debugStream, static_cast<uint32_t>( memValue ), 2, false );
		break;

	case addrMode_t::IndexedAbsoluteX:
	case addrMode_t::IndexedAbsoluteY:
		PrintHex( debugStream, targetAddress, 4, true );
		debugStream << "," << ( isXReg ? "X" : "Y" ) << " @ ";
		PrintHex( debugStream, address, 4, false );
		debugStream << " = ";
		PrintHex( debugStream, static_cast<uint32_t>( memValue ), 2, false );
		break;

	case addrMode_t::IndexedZeroX:
	case addrMode_t::IndexedZeroY:
		PrintHex( debugStream, targetAddress, 2, true );
		debugStream << "," << ( isXReg ? "X" : "Y" ) << " @ ";
		PrintHex( debugStream, address, 2, false );
		debugStream << " = ";
		PrintHex( debugStream, static_cast<uint32_t>( memValue ), 2, false );
		break;

	case addrMode_t::Immediate:
		debugStream << "#";
		PrintHex( debugStream, static_cast<uint32_t>( memValue ), 2, true );
		break;

	case addrMode_t::IndirectIndexed:
		debugStream << "(";
		PrintHex( debugStream, static_cast<uint32_t>( op0 ), 2, true );
		debugStream << "),Y = ";
		PrintHex( debugStream, address, 4, false );
		debugStream << " @ ";
		PrintHex( debugStream, targetAddress, 4, false );
		debugStream << " = ";
		PrintHex( debugStream, static_cast<uint32_t>( memValue ), 2, false );
		break;

	case addrMode_t::IndexedIndirect:
		debugStream << "(";
		PrintHex( debugStream, static_cast<uint32_t>( op0 ), 2, true );
		debugStream << ",X) @ ";
		PrintHex( debugStream, static_cast<uint32_t>( targetAddress ), 2, false );
		debugStream << " = ";
		PrintHex( debugStream, address, 4, false );
		debugStream << " = ";
		PrintHex( debugStream, static_cast<uint32_t>( memValue ), 2, false );
		break;

	case addrMode_t::Accumulator:
		debugStream << "A";
		break;

	case addrMode_t::Jmp:
		PrintHex( debugStream, address, 2, true );
		break;

	case addrMode_t::JmpIndirect:
		debugStream << "(";
		PrintHex( debugStream, offset, 4, true );
		debugStream << ") = ";
		PrintHex( debugStream, address, 4, false );
		break;

	case addrMode_t::Jsr:
		PrintHex( debugStream, address, 4, true );
		break;

	case addrMode_t::Branch:
		PrintHex( debugStream, address, 2, true );
		break;
	}

	if ( registerDebug )
	{
		debugStream.seekg( 0, ios::end );
		const size_t alignment = 50;
		const size_t width = ( alignment - debugStream.tellg() );
		debugStream.seekg( 0, ios::beg );

		debugStream << setfill( ' ' ) << setw( width ) << right;
		debugStream << uppercase << "A:";
		PrintHex( debugStream, static_cast<int>( regInfo.A ), 2, false );
		debugStream << uppercase << " X:";
		PrintHex( debugStream, static_cast<int>( regInfo.X ), 2, false );
		debugStream << uppercase << " Y:";
		PrintHex( debugStream, static_cast<int>( regInfo.Y ), 2, false );
		debugStream << uppercase << " P:";
		PrintHex( debugStream, static_cast<int>( regInfo.P ), 2, false );
		debugStream << uppercase << " SP:";
		PrintHex( debugStream, static_cast<int>( regInfo.SP ), 2, false );
	}

	if( cycleDebug )
	{
		debugStream << uppercase << " PPU:" << setfill( ' ' ) << setw( 3 ) <<  dec << right << curScanline;
		debugStream << uppercase << "," << setfill( ' ' ) << setw( 3 ) << dec << ppuCycles;
		debugStream << uppercase << " CYC:" << dec << cpuCycles << "\0";
	}

	buffer += debugStream.str();
}


void wtLog::Reset( const uint32_t targetCount )
{
	log.clear();
	totalCount = ( 1 + targetCount );
	frameIx = 0;
	log.resize( totalCount );
	for ( size_t frameIndex = 0; frameIndex < totalCount; ++frameIndex ) {
		log[ frameIndex ].reserve( 10000 );
	}
	NewFrame();
}


void wtLog::NewFrame()
{
	if( IsFull() ) {
		return;
	}

	++frameIx;
	log[ frameIx ].reserve( 10000 );
}


OpDebugInfo& wtLog::NewLine()
{
	log[ frameIx ].resize( log[ frameIx ].size() + 1 );
	return GetLogLine();
}


const logFrame_t& wtLog::GetLogFrame( const uint32_t frameNum ) const
{
	const uint32_t frameIndex = ( frameNum >= totalCount ) ? 0 : frameNum;
	return log[ frameIndex ];
}


OpDebugInfo& wtLog::GetLogLine()
{
	return log[ frameIx ].back();
}


uint32_t wtLog::GetRecordCount() const
{
	return ( totalCount == 0 ) ? 0 : ( frameIx + 1 );
}


bool wtLog::IsFull() const
{
	assert( totalCount > 0 );
	return ( ( totalCount <= 1 ) || ( frameIx >= ( totalCount - 1 ) ) );
}


bool wtLog::IsFinished() const
{
	// This needs to be improved, but sufficient for now
	// Right now it's a weak condition
	return ( IsFull() && log[ frameIx ].size() > 0 );
}


void wtLog::ToString( std::string& buffer, const uint32_t frameBegin, const uint32_t frameEnd, const bool registerDebug ) const
{
	for( uint32_t i = ( frameBegin + 1 ); i <= frameEnd; ++i )
	{
		for ( const OpDebugInfo& dbgInfo : GetLogFrame( i ) )
		{
			dbgInfo.ToString( buffer, true, false );
			buffer += "\n";
		}
	}
}