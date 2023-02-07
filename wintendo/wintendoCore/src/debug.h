/*
* MIT License
*
* Copyright( c ) 2017-2021 Thomas Griebel
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this softwareand associated documentation files( the "Software" ), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright noticeand this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

#include <string>
#include <sstream>

#include "common.h"

#if DEBUG_ADDR == 1
#define DEBUG_ADDR_INDEXED_ZERO( _info )												\
	if( IsTraceLogOpen() )																\
	{																					\
		OpDebugInfo& dbgInfo	= dbgLog.GetLogLine();									\
		dbgInfo.address			= _info.addr;											\
		dbgInfo.targetAddress	= _info.targetAddr;										\
		dbgInfo.memValue		= _info.value;											\
	}

#define DEBUG_ADDR_INDEXED_ABS( _info )													\
	if( IsTraceLogOpen() )																\
	{																					\
		OpDebugInfo& dbgInfo	= dbgLog.GetLogLine();									\
		dbgInfo.address			= _info.addr;											\
		dbgInfo.targetAddress	= _info.targetAddr;										\
		dbgInfo.memValue		= _info.value;											\
	}

#define DEBUG_ADDR_ZERO( _info )														\
	if( cpu.IsTraceLogOpen() )															\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgLog.GetLogLine();								\
		dbgInfo.address			= _info.addr;											\
		dbgInfo.memValue		= _info.value;											\
	}

#define DEBUG_ADDR_ABS( _info )															\
	if( cpu.IsTraceLogOpen() )															\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgLog.GetLogLine();								\
		dbgInfo.address			= _info.addr;											\
		dbgInfo.memValue		= _info.value;											\
	}

#define DEBUG_ADDR_IMMEDIATE( _info )													\
	if( cpu.IsTraceLogOpen() )															\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgLog.GetLogLine();								\
		dbgInfo.address			= _info.addr;											\
		dbgInfo.memValue		= cpu.ReadOperand(0);									\
	}

#define DEBUG_ADDR_INDIRECT_INDEXED( _info )											\
	if( cpu.IsTraceLogOpen() )															\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgLog.GetLogLine();								\
		dbgInfo.address			= _info.addr;											\
		dbgInfo.targetAddress	= _info.targetAddr;										\
		dbgInfo.memValue		= _info.value;											\
		dbgInfo.offset			= _info.offset;											\
	}

#define DEBUG_ADDR_INDEXED_INDIRECT( _info )											\
	if( cpu.IsTraceLogOpen() )															\
	{																					\
		OpDebugInfo& dbgInfo	= cpu.dbgLog.GetLogLine();								\
		dbgInfo.address			= _info.addr;											\
		dbgInfo.memValue		= _info.value;											\
		dbgInfo.targetAddress	= _info.targetAddr;										\
	}

#define DEBUG_ADDR_ACCUMULATOR( _info )													\
	if( cpu.IsTraceLogOpen() )															\
	{																					\
	}

#define DEBUG_ADDR_JMP( _PC )															\
	if( IsTraceLogOpen() )																\
	{																					\
		OpDebugInfo& dbgInfo	= dbgLog.GetLogLine();									\
		dbgInfo.address			= _PC;													\
	}

#define DEBUG_ADDR_JMPI( _offset, _PC )													\
	if( IsTraceLogOpen() )																\
	{																					\
		OpDebugInfo& dbgInfo	= dbgLog.GetLogLine();									\
		dbgInfo.offset			= _offset;												\
		dbgInfo.address			= _PC;													\
	}

#define DEBUG_ADDR_JSR( _PC )															\
	if( IsTraceLogOpen() )																\
	{																					\
		OpDebugInfo& dbgInfo	= dbgLog.GetLogLine();									\
		dbgInfo.address			= _PC;													\
	}

#define DEBUG_ADDR_BRANCH( _PC )														\
	if( IsTraceLogOpen() )																\
	{																					\
		OpDebugInfo& dbgInfo	= dbgLog.GetLogLine();									\
		dbgInfo.address			= _PC;													\
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