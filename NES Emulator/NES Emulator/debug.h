#pragma once

#if DEBUG_ADDR == 1
#define DEBUG_ADDR_INDEXED_ZERO { uint8_t& value = system->GetMemory( address ); \
	debugAddr.str( std::string() ); \
	debugAddr << uppercase << "$" << setw( 2 ) << hex << targetAddresss << ","; \
	debugAddr << ( ( &reg == &X ) ? "X" : "Y" ); \
	debugAddr << setfill( '0' ) << " @ " << setw( 2 ) << hex << address; \
	debugAddr << " = " << setw( 2 ) << hex << static_cast< uint32_t >( value ); }

#define DEBUG_ADDR_INDEXED_ABS { uint8_t& value = system->GetMemory( address ); \
	debugAddr.str( std::string() ); \
	debugAddr << uppercase << "$" << setw( 4 ) << hex << targetAddresss << ","; \
	debugAddr << ( ( &reg == &X ) ? "X" : "Y" ); \
	debugAddr << setfill( '0' ) << " @ " << setw( 4 ) << hex << address; \
	debugAddr << " = " << setw( 2 ) << hex << static_cast< uint32_t >( value ); }

#define DEBUG_ADDR_ZERO { uint8_t& value = system->GetMemory( address ); \
	debugAddr.str( std::string() ); \
	debugAddr << uppercase << "$" << setfill( '0' ) << setw( 2 ) << address; \
	debugAddr << " = " << setfill( '0' ) << setw( 2 ) << hex << static_cast< uint32_t >( value ); }

#define DEBUG_ADDR_ABS { uint8_t& value = system->GetMemory( address ); \
	debugAddr.str( std::string() ); \
	debugAddr << uppercase << "$" << setfill( '0' ) << setw( 4 ) << address; \
	debugAddr << " = " << setfill( '0' ) << setw( 2 ) << hex << static_cast< uint32_t >( value ); }

#define DEBUG_ADDR_IMMEDIATE { debugAddr.str( std::string() ); \
	debugAddr << uppercase << "#$" << setfill( '0' ) << setw( 2 ) << hex << static_cast< uint32_t >( value ); }

#define DEBUG_ADDR_INDIRECT_INDEXED { uint8_t& value = system->GetMemory( offset ); \
	debugAddr.str( std::string() ); \
	debugAddr << uppercase << "($" << setfill( '0' ) << setw( 2 ) << static_cast< uint32_t >( params.param0 ) << "),Y = "; \
	debugAddr << setw( 4 ) << hex << address; \
	debugAddr << " @ " << setw( 4 ) << hex << offset << " = " << setw( 2 ) << hex << static_cast< uint32_t >( value ); }

#define DEBUG_ADDR_INDEXED_INDIRECT { uint8_t& value = system->GetMemory( address ); \
	debugAddr.str( std::string() ); \
	debugAddr << uppercase << "($" << setfill( '0' ) << setw( 2 ) << static_cast< uint32_t >( params.param0 ) << ",X) @ "; \
	debugAddr << setw( 2 ) << static_cast< uint32_t >( targetAddress ); \
	debugAddr << " = " << setw( 4 ) << address << " = " << setw( 2 ) << static_cast< uint32_t >( value ); }

#define DEBUG_ADDR_ACCUMULATOR { debugAddr.str( std::string() ); \
	debugAddr << "A"; }

#define DEBUG_ADDR_JMP { debugAddr.str( std::string() ); \
	debugAddr << uppercase << setfill( '0' ) << "$" << setw( 2 ) << hex << PC; }

#define DEBUG_ADDR_JMPI { debugAddr.str( std::string() ); \
	debugAddr << uppercase << setfill( '0' ) << "($" << setw( 4 ) << hex << addr0; \
	debugAddr << ") = " << setw( 4 ) << hex << PC; }

#define DEBUG_ADDR_JSR { debugAddr.str( std::string() ); \
	debugAddr << uppercase << "$" << setfill( '0' ) << setw( 2 ) << hex << PC; }

#define DEBUG_ADDR_BRANCH { debugAddr.str( std::string() ); \
	debugAddr << uppercase << "$" << setfill( '0' ) << setw( 2 ) << hex << branchedPC; }

#define DEBUG_CPU_LOG if ( enablePrinting ) { \
	int disassemblyBytes[6] = { curbyte, params.param0, params.param1,'\0' }; \
	stringstream hexString; \
	stringstream logLine; \
	if ( operands == 1 ) \
		hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0] << " " << setw( 2 ) << disassemblyBytes[1]; \
	else if ( operands == 2 ) \
		hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0] << " " << setw( 2 ) << disassemblyBytes[1] << " " << setw( 2 ) << disassemblyBytes[2]; \
	else \
		hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << disassemblyBytes[0]; \
	logLine << uppercase << setfill( '0' ) << setw( 4 ) << hex << instrBegin << setfill( ' ' ) << "  " << setw( 10 ) << left << hexString.str() << pair.mnemonic << " " << setw( 28 ) << left << debugAddr.str() << right << regStr; \
	logFile << logLine.str() << endl; \
	if( printToOutput ){ std::cout << logLine.str() << endl; } \
	}
#else //  #if DEBUG_ADDR == 1
#define DEBUG_ADDR_INDEXED_ZERO 0;
#define DEBUG_ADDR_INDEXED_ABS 0;
#define DEBUG_ADDR_ZERO 0;
#define DEBUG_ADDR_ABS 0;
#define DEBUG_ADDR_IMMEDIATE 0;
#define DEBUG_ADDR_INDIRECT_INDEXED 0;
#define DEBUG_ADDR_INDEXED_INDIRECT  0;
#define DEBUG_ADDR_ACCUMULATOR 0;
#define DEBUG_ADDR_JMP 0;
#define DEBUG_ADDR_JMPI 0;
#define DEBUG_ADDR_JSR 0;
#define DEBUG_ADDR_BRANCH 0;
#define DEBUG_CPU_LOG 0;
#endif // #else // #if DEBUG_ADDR == 1