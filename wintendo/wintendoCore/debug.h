#pragma once

#include <string>
#include <sstream>

#include "common.h"

#if DEBUG_ADDR == 1
#define DEBUG_ADDR_INDEXED_ZERO if( logFrameCount > 0 )									\
	{																					\
		OpDebugInfo& dbgInfo	= dbgMetrics.back();									\
		dbgInfo.addrMode		= static_cast<uint8_t>( ( &reg == &X ) ? addrMode_t::IndexedAbsoluteX : addrMode_t::IndexedAbsoluteY );	\
		dbgInfo.address			= address;												\
		dbgInfo.targetAddress	= targetAddress;										\
		dbgInfo.memValue		= system->ReadMemory( address );						\
		dbgInfo.isXReg			= ( &reg == &X );										\
	}

#define DEBUG_ADDR_INDEXED_ABS if( logFrameCount > 0 )									\
	{																					\
		OpDebugInfo& dbgInfo	= dbgMetrics.back();									\
		dbgInfo.addrMode		= static_cast<uint8_t>( ( &reg == &X ) ? addrMode_t::IndexedAbsoluteX : addrMode_t::IndexedAbsoluteY );	\
		dbgInfo.address			= address;												\
		dbgInfo.targetAddress	= targetAddress;										\
		dbgInfo.memValue		= system->ReadMemory( address );						\
		dbgInfo.isXReg			= ( &reg == &X );										\
	}

#define DEBUG_ADDR_ZERO if( cpu.logFrameCount > 0 )										\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgMetrics.back();								\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::Zero );				\
		dbgInfo.address			= address;												\
		dbgInfo.memValue		= cpu.system->ReadMemory( address );					\
	}

#define DEBUG_ADDR_ABS if( cpu.logFrameCount > 0 )										\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgMetrics.back();								\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::Absolute );			\
		dbgInfo.address			= address;												\
		dbgInfo.memValue		= cpu.system->ReadMemory( address );					\
	}

#define DEBUG_ADDR_IMMEDIATE if( cpu.logFrameCount > 0 )								\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgMetrics.back();								\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::Immediate );		\
		dbgInfo.address			= address;												\
		dbgInfo.memValue		= cpu.ReadOperand(0);									\
	}

#define DEBUG_ADDR_INDIRECT_INDEXED if( cpu.logFrameCount > 0 )							\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgMetrics.back();								\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::IndirectIndexed );	\
		dbgInfo.address			= address;												\
		dbgInfo.memValue		= cpu.system->ReadMemory( offset );						\
		dbgInfo.operand			= cpu.ReadOperand( 0 );									\
		dbgInfo.offset			= offset;												\
	}

#define DEBUG_ADDR_INDEXED_INDIRECT if( cpu.logFrameCount > 0 )							\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgMetrics.back();								\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::IndexedIndirect );	\
		dbgInfo.address			= address;												\
		dbgInfo.memValue		= cpu.system->ReadMemory( address );					\
		dbgInfo.operand			= cpu.ReadOperand( 0 );									\
		dbgInfo.targetAddress	= targetAddress;										\
	}

#define DEBUG_ADDR_ACCUMULATOR if( cpu.logFrameCount > 0 ) {							\
		OpDebugInfo& dbgInfo	= cpu.dbgMetrics.back();								\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::Accumulator );		\
	}

#define DEBUG_ADDR_JMP if( logFrameCount > 0 ) {										\
		OpDebugInfo& dbgInfo	= dbgMetrics.back();									\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::Jmp );				\
		dbgInfo.address			= PC;													\
	}

#define DEBUG_ADDR_JMPI if( logFrameCount > 0 ) {										\
		OpDebugInfo& dbgInfo	= dbgMetrics.back();									\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::JmpIndirect );		\
		dbgInfo.offset			= addr;													\
		dbgInfo.address			= PC;													\
	}

#define DEBUG_ADDR_JSR if( logFrameCount > 0 ) {										\
		OpDebugInfo& dbgInfo	= dbgMetrics.back();									\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::Jsr );				\
		dbgInfo.address			= PC;													\
	}

#define DEBUG_ADDR_BRANCH if( logFrameCount > 0 ) {										\
		OpDebugInfo& dbgInfo	= dbgMetrics.back();									\
		dbgInfo.addrMode		= static_cast<uint8_t>( addrMode_t::Branch );			\
		dbgInfo.address			= branchedPC;											\
	}

#define DEBUG_CPU_LOG 0;

#else //  #if DEBUG_ADDR == 1
#define DEBUG_ADDR_INDEXED_ZERO		{}
#define DEBUG_ADDR_INDEXED_ABS		{}
#define DEBUG_ADDR_ZERO				{}
#define DEBUG_ADDR_ABS				{}
#define DEBUG_ADDR_IMMEDIATE		{}
#define DEBUG_ADDR_INDIRECT_INDEXED	{}
#define DEBUG_ADDR_INDEXED_INDIRECT	{}
#define DEBUG_ADDR_ACCUMULATOR		{}
#define DEBUG_ADDR_JMP				{}
#define DEBUG_ADDR_JMPI				{}
#define DEBUG_ADDR_JSR				{}
#define DEBUG_ADDR_BRANCH			{}
#define DEBUG_CPU_LOG				{}
#endif // #else // #if DEBUG_ADDR == 1


struct regDebugInfo_t
{
	uint8_t			X;
	uint8_t			Y;
	uint8_t			A;
	uint8_t			SP;
	uint8_t			P;
	uint16_t		PC;
};


class OpDebugInfo
{
public:

	uint32_t		loadCnt;
	uint32_t		storeCnt;

	uint8_t			addrMode;
	bool			isXReg;
	uint8_t			memValue;
	uint8_t			operand;
	uint16_t		address;
	uint16_t		offset;
	uint16_t		targetAddress;
	uint8_t			byteCode;
	uint16_t		instrBegin;

	uint8_t			operands;
	uint8_t			op0;
	uint8_t			op1;

	const char*		mnemonic;

	regDebugInfo_t	regInfo;

	int32_t			curScanline;
	cpuCycle_t		cpuCycles;
	ppuCycle_t		ppuCycles;
	cpuCycle_t		instrCycles;
	
	OpDebugInfo()
	{
		loadCnt			= 0;
		storeCnt		= 0;

		addrMode		= 0;
		isXReg			= false;
		memValue		= 0;
		operand			= 0;
		address			= 0;
		offset			= 0;
		targetAddress	= 0;
		instrBegin		= 0;
		curScanline		= 0;
		cpuCycles		= cpuCycle_t( 0 );
		ppuCycles		= ppuCycle_t( 0 );
		instrCycles		= cpuCycle_t( 0 );

		op0				= 0;
		op1				= 0;

		mnemonic		= "";
		operands		= 0;
		byteCode		= 0;

		regInfo			= { 0, 0, 0, 0, 0, 0 };
	}

	void ToString( std::string& buffer, bool registerDebug = true );
};