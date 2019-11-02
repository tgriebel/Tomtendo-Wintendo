#pragma once

#if DEBUG_ADDR == 1
#define DEBUG_ADDR_INDEXED_ZERO if( cpu.debugFrame ) \
	{ \
		uint8_t& value = cpu.system->GetMemory( address ); \
		cpu.debugAddr.str( std::string() ); \
		cpu.debugAddr << uppercase << "$" << setw( 2 ) << hex << targetAddresss << ","; \
		cpu.debugAddr << ( ( &reg == &X ) ? "X" : "Y" ); \
		cpu.debugAddr << setfill( '0' ) << " @ " << setw( 2 ) << hex << address; \
		cpu.debugAddr << " = " << setw( 2 ) << hex << static_cast< uint32_t >( value ); \
	}

#define DEBUG_ADDR_INDEXED_ABS if( logFrameCount > 0 ) \
	{ \
		uint8_t& value = system->GetMemory( address ); \
		debugAddr.str( std::string() ); \
		debugAddr << uppercase << "$" << setw( 4 ) << hex << targetAddresss << ","; \
		debugAddr << ( ( &reg == &X ) ? "X" : "Y" ); \
		debugAddr << setfill( '0' ) << " @ " << setw( 4 ) << hex << address; \
		debugAddr << " = " << setw( 2 ) << hex << static_cast< uint32_t >( value ); \
	}

#define DEBUG_ADDR_ZERO if( cpu.logFrameCount > 0 ) \
	{ \
		uint8_t& value = cpu.system->GetMemory( address ); \
		cpu.debugAddr.str( std::string() ); \
		cpu.debugAddr << uppercase << "$" << setfill( '0' ) << setw( 2 ) << address; \
		cpu.debugAddr << " = " << setfill( '0' ) << setw( 2 ) << hex << static_cast< uint32_t >( value ); \
	}

#define DEBUG_ADDR_ABS if( cpu.logFrameCount > 0 ) \
	{ \
		uint8_t& value = cpu.system->GetMemory( address ); \
		cpu.debugAddr.str( std::string() ); \
		cpu.debugAddr << uppercase << "$" << setfill( '0' ) << setw( 4 ) << address; \
		cpu.debugAddr << " = " << setfill( '0' ) << setw( 2 ) << hex << static_cast< uint32_t >( value ); \
	}

#define DEBUG_ADDR_IMMEDIATE if( cpu.logFrameCount > 0 ) \
	{ \
		cpu.debugAddr.str( std::string() ); \
		cpu.debugAddr << uppercase << "#$" << setfill( '0' ) << setw( 2 ) << hex << static_cast< uint32_t >( value ); \
	}

#define DEBUG_ADDR_INDIRECT_INDEXED if( cpu.logFrameCount > 0 ) \
	{ \
		uint8_t& value = cpu.system->GetMemory( offset ); \
		cpu.debugAddr.str( std::string() ); \
		cpu.debugAddr << uppercase << "($" << setfill( '0' ) << setw( 2 ) << static_cast< uint32_t >( cpu.ReadOperand(0) ) << "),Y = "; \
		cpu.debugAddr << setw( 4 ) << hex << address; \
		cpu.debugAddr << " @ " << setw( 4 ) << hex << offset << " = " << setw( 2 ) << hex << static_cast< uint32_t >( value ); \
	}

#define DEBUG_ADDR_INDEXED_INDIRECT if( cpu.logFrameCount > 0 ) \
	{ \
		uint8_t& value = cpu.system->GetMemory( address ); \
		cpu.debugAddr.str( std::string() ); \
		cpu.debugAddr << uppercase << "($" << setfill( '0' ) << setw( 2 ) << static_cast< uint32_t >( cpu.ReadOperand(0) ) << ",X) @ "; \
		cpu.debugAddr << setw( 2 ) << static_cast< uint32_t >( targetAddress ); \
		cpu.debugAddr << " = " << setw( 4 ) << address << " = " << setw( 2 ) << static_cast< uint32_t >( value ); \
	}

#define DEBUG_ADDR_ACCUMULATOR if( cpu.logFrameCount > 0 ) { \
		cpu.debugAddr.str( std::string() ); \
		cpu.debugAddr << "A"; \
	}

#define DEBUG_ADDR_JMP if( logFrameCount > 0 ) { \
		debugAddr.str( std::string() ); \
		debugAddr << uppercase << setfill( '0' ) << "$" << setw( 2 ) << hex << PC; \
	}

#define DEBUG_ADDR_JMPI if( logFrameCount > 0 ) { \
		debugAddr.str( std::string() ); \
		debugAddr << uppercase << setfill( '0' ) << "($" << setw( 4 ) << hex << addr0; \
		debugAddr << ") = " << setw( 4 ) << hex << PC; \
	}

#define DEBUG_ADDR_JSR if( logFrameCount > 0 ) { \
		debugAddr.str( std::string() ); \
		debugAddr << uppercase << "$" << setfill( '0' ) << setw( 2 ) << hex << PC; \
	}

#define DEBUG_ADDR_BRANCH if( logFrameCount > 0 ) { \
		debugAddr.str( std::string() ); \
		debugAddr << uppercase << "$" << setfill( '0' ) << setw( 2 ) << hex << branchedPC; \
	}

#define DEBUG_CPU_LOG 0;

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