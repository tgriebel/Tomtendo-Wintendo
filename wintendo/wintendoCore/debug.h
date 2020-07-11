#pragma once

#if DEBUG_ADDR == 1
#define DEBUG_ADDR_INDEXED_ZERO if( logFrameCount > 0 ) \
	{ \
		instrDebugInfo& dbgInfo = dbgMetrics.back(); \
		dbgInfo.addrMode = ( &reg == &X ) ? AddrMode::IndexedAbsoluteX : AddrMode::IndexedAbsoluteY; \
		dbgInfo.address = address; \
		dbgInfo.targetAddress = targetAddress; \
		dbgInfo.memValue = system->GetMemory( address ); \
		dbgInfo.isXReg = ( &reg == &X ); \
	}

#define DEBUG_ADDR_INDEXED_ABS if( logFrameCount > 0 ) \
	{ \
		instrDebugInfo& dbgInfo = dbgMetrics.back(); \
		dbgInfo.addrMode = ( &reg == &X ) ? AddrMode::IndexedAbsoluteX : AddrMode::IndexedAbsoluteY; \
		dbgInfo.address = address; \
		dbgInfo.targetAddress = targetAddress; \
		dbgInfo.memValue = system->GetMemory( address ); \
		dbgInfo.isXReg = ( &reg == &X ); \
	}

#define DEBUG_ADDR_ZERO if( cpu.logFrameCount > 0 ) \
	{ \
		instrDebugInfo& dbgInfo = cpu.dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::Zero; \
		dbgInfo.address = address; \
		dbgInfo.memValue = cpu.system->GetMemory( address ); \
	}

#define DEBUG_ADDR_ABS if( cpu.logFrameCount > 0 ) \
	{ \
		instrDebugInfo& dbgInfo = cpu.dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::Absolute; \
		dbgInfo.address = address; \
		dbgInfo.memValue = cpu.system->GetMemory( address ); \
	}

#define DEBUG_ADDR_IMMEDIATE if( cpu.logFrameCount > 0 ) \
	{ \
		instrDebugInfo& dbgInfo = cpu.dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::Immediate; \
		dbgInfo.address = address; \
		dbgInfo.memValue = cpu.ReadOperand(0); \
	}

#define DEBUG_ADDR_INDIRECT_INDEXED if( cpu.logFrameCount > 0 ) \
	{ \
		instrDebugInfo& dbgInfo = cpu.dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::IndirectIndexed; \
		dbgInfo.address = address; \
		dbgInfo.memValue = cpu.system->GetMemory( offset ); \
		dbgInfo.operand = cpu.ReadOperand( 0 ); \
		dbgInfo.offset = offset; \
	}

#define DEBUG_ADDR_INDEXED_INDIRECT if( cpu.logFrameCount > 0 ) \
	{ \
		instrDebugInfo& dbgInfo = cpu.dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::IndexedIndirect; \
		dbgInfo.address = address; \
		dbgInfo.memValue = cpu.system->GetMemory( address ); \
		dbgInfo.operand = cpu.ReadOperand( 0 ); \
		dbgInfo.targetAddress = targetAddress; \
	}

#define DEBUG_ADDR_ACCUMULATOR if( cpu.logFrameCount > 0 ) { \
		instrDebugInfo& dbgInfo = cpu.dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::Accumulator; \
	}

#define DEBUG_ADDR_JMP if( logFrameCount > 0 ) { \
		instrDebugInfo& dbgInfo = dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::Jmp; \
		dbgInfo.address = PC; \
	}

#define DEBUG_ADDR_JMPI if( logFrameCount > 0 ) { \
		instrDebugInfo& dbgInfo = dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::JmpIndirect; \
		dbgInfo.offset = addr0; \
		dbgInfo.address = PC; \
	}

#define DEBUG_ADDR_JSR if( logFrameCount > 0 ) { \
		instrDebugInfo& dbgInfo = dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::Jsr; \
		dbgInfo.address = PC; \
	}

#define DEBUG_ADDR_BRANCH if( logFrameCount > 0 ) { \
		instrDebugInfo& dbgInfo = dbgMetrics.back(); \
		dbgInfo.addrMode = AddrMode::Branch; \
		dbgInfo.address = branchedPC; \
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