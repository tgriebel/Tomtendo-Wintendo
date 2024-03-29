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

#include "../../stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <bitset>
#include "../common.h"
#include "../debug.h"
#include "../processors/mos6502.h"
#include "../system/NesSystem.h"

using namespace std;

template <class AddrFunctor>
FORCE_INLINE uint8_t Cpu6502::Read( opState_t& opState )
{
	AddrFunctor(*this)( opState );
	return opState.addrInfo.value;
}


template <class AddrFunctor>
FORCE_INLINE void Cpu6502::Write( opState_t& opState, const uint8_t value )
{
	AddrFunctor( *this )( opState );

	if ( AddrFunctor::addrMode == addrMode_t::Accumulator )	{
		A = value;
	} else {
		system->WriteMemory( opState.addrInfo.addr, opState.addrInfo.offset, value );
	}
}


ADDR_MODE_DEF( None )
{
	opState.addrInfo = cpuAddrInfo_t( Cpu6502::InvalidAddress, 0, 0, 0 );
}


ADDR_MODE_DEF( IndexedIndirect )
{
	const uint8_t	targetAddress = ( opState.op0 + cpu.X );
	const uint32_t	address = cpu.CombineIndirect( targetAddress, 0x00, wtSystem::ZeroPageWrap );
	const uint8_t	value = cpu.system->ReadMemory( address );

	opState.addrInfo = cpuAddrInfo_t( address, targetAddress, 0, value );
	DEBUG_ADDR_INDEXED_INDIRECT( opState.addrInfo )
}


ADDR_MODE_DEF( IndirectIndexed )
{
	const uint32_t	address	= cpu.CombineIndirect( opState.op0, 0x00, wtSystem::ZeroPageWrap );
	const uint16_t	targetAddr	= ( address + cpu.Y ) % wtSystem::MemoryWrap;
	const uint8_t	value	= cpu.system->ReadMemory( targetAddr );

	opState.opCycles += cpuCycle_t( cpu.AddressCrossesPage( opState, address, cpu.Y ) ); // TODO: make consistent
	opState.addrInfo = cpuAddrInfo_t( address, targetAddr, cpu.Y, value );

	DEBUG_ADDR_INDIRECT_INDEXED( opState.addrInfo )
}


ADDR_MODE_DEF( Absolute )
{
	const uint32_t	address = cpu.ReadAddressOperand( opState );
	const uint8_t	value = cpu.system->ReadMemory( address );

	opState.addrInfo = cpuAddrInfo_t( address, 0, 0, value );
	DEBUG_ADDR_ABS( opState.addrInfo )
}


ADDR_MODE_DEF( IndexedAbsoluteX )
{
	cpu.IndexedAbsolute( opState, cpu.X );
}


ADDR_MODE_DEF( IndexedAbsoluteY )
{
	cpu.IndexedAbsolute( opState, cpu.Y );
}


ADDR_MODE_DEF( Zero )
{
	const uint16_t	targetAddresss	= opState.op0;
	const uint32_t	address			= ( targetAddresss & wtSystem::ZeroPageEnd );
	const uint8_t	value			= cpu.system->ReadZeroPage( address );
	
	opState.addrInfo = cpuAddrInfo_t( address, targetAddresss, 0, value );
	DEBUG_ADDR_ZERO( opState.addrInfo )
}


ADDR_MODE_DEF( IndexedZeroX )
{
	cpu.IndexedZero( opState, cpu.X );
}


ADDR_MODE_DEF( IndexedZeroY )
{
	cpu.IndexedZero( opState, cpu.Y );
}


ADDR_MODE_DEF( Immediate )
{
	const uint8_t value = opState.op0;

	opState.addrInfo = cpuAddrInfo_t( Cpu6502::InvalidAddress, 0, 0, value );
	DEBUG_ADDR_IMMEDIATE( opState.addrInfo )
}


ADDR_MODE_DEF( Accumulator )
{
	opState.addrInfo = cpuAddrInfo_t( Cpu6502::InvalidAddress, 0, 0, cpu.A );
	DEBUG_ADDR_ACCUMULATOR( opState.addrInfo )
}


uint16_t Cpu6502::JumpImmediateAddr( const uint16_t addr )
{
	// Hardware bug - http://wiki.nesdev.com/w/index.php/Errata
	if ( ( addr & 0xFF ) == 0xFF )
	{
		return ( Combine( system->ReadMemory( addr ), system->ReadMemory( addr & 0xFF00 ) ) );
	}
	else
	{
		return ( Combine( system->ReadMemory( addr ), system->ReadMemory( addr + 1 ) ) );
	}
}


void Cpu6502::Push( const uint8_t value )
{
	system->GetStack() = value;
	SP--;
}


void Cpu6502::PushWord( const uint16_t value )
{
	Push( ( value >> 8 ) & 0xFF );
	Push( value & 0xFF );
}


uint8_t Cpu6502::Pull()
{
	SP++;
	return system->GetStack();
}


uint16_t Cpu6502::PullWord()
{
	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	return Combine( loByte, hiByte );
}


void Cpu6502::Branch( opState_t& opState, const bool takeBranch )
{
	uint8_t cycles = 0;
	const int8_t	offset		= static_cast<int8_t>( opState.op0 );
	const uint16_t	branchedPC	= PC + offset + 1;

	if ( takeBranch )
	{
		cycles = 1 + AddressCrossesPage( opState, ( PC + 1 ), offset );

		PC = ( PC + offset + 1 );
	}
	else
	{
		PC += 1;
	}

	opState.opCycles += cpuCycle_t( cycles );

	DEBUG_ADDR_BRANCH( branchedPC )
}


void Cpu6502::SetAluFlags( const uint16_t value )
{
	P.bit.z = CheckZero( value );
	P.bit.n = CheckSign( value );
}


bool Cpu6502::CheckSign( const uint16_t checkValue )
{
	return ( checkValue & 0x0080 );
}


bool Cpu6502::CheckCarry( const uint16_t checkValue )
{
	return ( checkValue > 0x00ff );
}


bool Cpu6502::CheckZero( const uint16_t checkValue )
{
	return ( checkValue == 0 );
}


bool Cpu6502::CheckOverflow( const uint16_t src, const uint16_t temp, const uint8_t finalValue )
{
	const uint8_t signedBound = 0x80;
	return CheckSign( finalValue ^ src ) && CheckSign( temp ^ src ) && !CheckCarry( temp );
}


uint8_t Cpu6502::AddressCrossesPage( opState_t& opState, const uint16_t address, const uint16_t offset )
{
	const uint8_t	cycle			= static_cast<uint8_t>( opState.extraCycle );
	const uint32_t	targetAddress	= ( address + offset ) % wtSystem::MemoryWrap;
	const bool		crossesPage		= ( ( targetAddress & 0xFF00 ) != ( address & 0xFF00 ) );
	return ( crossesPage ? cycle : 0 );
}


uint16_t Cpu6502::CombineIndirect( const uint8_t lsb, const uint8_t msb, const uint32_t wrap )
{
	const uint16_t	address	= Combine( lsb, msb );
	const uint8_t	loByte	= system->ReadMemory( address % wrap );
	const uint8_t	hiByte	= system->ReadMemory( ( address + 1 ) % wrap );
	const uint16_t	value	= Combine( loByte, hiByte );

	return value;
}


void Cpu6502::IndexedZero( opState_t& opState, const uint8_t& reg )
{
	const uint16_t	targetAddress	= opState.op0;
	const uint32_t	address			= ( targetAddress + reg ) & wtSystem::ZeroPageEnd;
	const uint8_t	value			= system->ReadZeroPage( address );

	opState.addrInfo = cpuAddrInfo_t( address, targetAddress, 0, value );
	DEBUG_ADDR_INDEXED_ZERO( opState.addrInfo )
}


void Cpu6502::IndexedAbsolute( opState_t& opState, const uint8_t& reg )
{
	const uint16_t	targetAddress	= ReadAddressOperand( opState );
	const uint32_t	address			= ( targetAddress + reg ) % wtSystem::MemoryWrap;
	const uint8_t	value			= system->ReadMemory( address );

	opState.opCycles += cpuCycle_t( AddressCrossesPage( opState, targetAddress, reg ) ); // TODO: make consistent
	opState.addrInfo = cpuAddrInfo_t( address, targetAddress, 0, value );

	DEBUG_ADDR_INDEXED_ABS( opState.addrInfo )
}


void Cpu6502::NMI( const uint16_t addr )
{
	PushWord( PC - 1 );
	// http://wiki.nesdev.com/w/index.php/CPU_status_flag_behavior
	Push( P.byte | STATUS_BREAK | STATUS_UNUSED );

	P.bit.i = 1;

	PC = addr;
}


void Cpu6502::IRQ()
{
	if( P.bit.i ) {
		return;
	}
	NMI( irqVector );
}


void Cpu6502::AdvancePC( const uint16_t places )
{
	PC += places;
}


uint8_t Cpu6502::ReadOperand( const uint16_t offset ) const
{
	return system->ReadMemory( PC + offset );
}


uint16_t Cpu6502::ReadAddressOperand( struct opState_t& opState ) const
{
	const uint8_t loByte = opState.op0;
	const uint8_t hiByte = opState.op1;

	return Combine( loByte, hiByte );
}


cpuCycle_t Cpu6502::Exec()
{
	cpuCycle_t opCycle = cpuCycle_t( 0 );

	if ( oamInProcess )
	{
		// http://wiki.nesdev.com/w/index.php/PPU_registers#OAMDMA
		if ( ( cycle.count() % 2 ) == 0 ) {
			opCycle += cpuCycle_t( 514 );
		} else {
			opCycle += cpuCycle_t( 513 );
		}
		oamInProcess = false;

		return opCycle;
	}
	else if ( dmcTransfer )
	{
		return cpuCycle_t( 1 );
	}

	const uint16_t instrAddr = PC;

	const uint8_t curbyte = system->ReadMemory( instrAddr );

	AdvancePC( 1 );

	if ( interruptRequestNMI )
	{
		NMI( irqAddr );
		interruptRequestNMI = false;
		if ( IsTraceLogOpen() )
		{
			OpDebugInfo& instrDbg = dbgLog.NewLine();
			instrDbg.nmi = true;
			instrDbg.cpuCycles = cycle.count();
		}
		return opCycle;
	}
	else if( interruptRequest )
	{
		IRQ();
		interruptRequest = false;
		if ( IsTraceLogOpen() )
		{
			OpDebugInfo& instrDbg = dbgLog.NewLine();
			instrDbg.irq = true;
			instrDbg.cpuCycles = cycle.count();
		}
		return opCycle;
	}

	return OpExec( instrAddr, curbyte );
}


bool Cpu6502::Step( const cpuCycle_t& nextCycle )
{
	while ( ( cycle < nextCycle ) && !halt )
	{
		cycle += cpuCycle_t( Exec() );
	}

	return !halt;
}


void Cpu6502::RegisterSystem( wtSystem* sys )
{
	system = sys;
}


bool Cpu6502::IsTraceLogOpen() const
{
	return ( logFrameCount > 0 );
}


void Cpu6502::StartTraceLog( const uint32_t frameCount )
{
	logFrameCount = static_cast<int32_t>( frameCount );
	logFrameTotal = logFrameCount;

	dbgLog.Reset( logFrameTotal );
}


void Cpu6502::StopTraceLog()
{
	logFrameCount = logFrameTotal;
}

cpuCycle_t Cpu6502::OpExec( const uint16_t instrAddr, const uint8_t byteCode )
{
	const opInfo_t& op = opLUT[ byteCode ];

	opState_t opState;
	opState.opCode		= byteCode;
	opState.opCycles	= cpuCycle_t( op.baseCycles );
	opState.extraCycle	= op.extraCycle;
	opState.op0			= system->ReadMemory( instrAddr + 1 );
	opState.op1			= system->ReadMemory( instrAddr + 2 );

#if DEBUG_ADDR == 1
	if ( IsTraceLogOpen() ) // TODO: deep log and shallow log. Basic logging is very fast
	{
		OpDebugInfo& instrDbg = dbgLog.NewLine();
		instrDbg.loadCnt		= 0;
		instrDbg.storeCnt		= 0;
		instrDbg.byteCode		= opState.opCode;
		instrDbg.regInfo		= { X, Y, A, SP, (uint8_t)P.byte, PC };
		instrDbg.instrBegin		= instrAddr;
		instrDbg.cpuCycles		= cycle.count();
		instrDbg.ppuCycles		= system->GetPPU().GetCycle().count() % PPU::ScanlineCycles;
		instrDbg.curScanline	= system->GetPPU().GetScanline();
		instrDbg.instrCycles	= opState.opCycles.count();
		instrDbg.op0			= opState.op0;
		instrDbg.op1			= opState.op1;
		instrDbg.mnemonic		= op.mnemonic;
		instrDbg.operands		= op.operands;
		instrDbg.isIllegal		= op.illegal;
		instrDbg.opType			= (uint8_t)op.type;
		instrDbg.addrMode		= (uint8_t)op.addrMode;
	}
#endif

	if( false )
	{
		// FIXME: linker error when removed? why?
		BuildOpLUT();
	}

	(this->*( op.func ))( opState );
	AdvancePC( op.pcInc );

	return opState.opCycles;
}