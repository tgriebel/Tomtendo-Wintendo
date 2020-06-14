#pragma once

#include <map>
#include <sstream>
#include "common.h"

typedef uint8_t StatusBit;

struct Cpu6502;
struct OpCodeMap;
struct AddrMapTuple;
struct InstructionMapTuple;
struct DisassemblerMapTuple;
class wtSystem;
struct CpuAddrInfo;

enum struct AddrMode : uint8_t
{
	None,
	Absolute,
	Zero,
	Immediate,
	IndexedIndirect,
	IndirectIndexed,
	Accumulator,
	IndexedAbsoluteX,
	IndexedAbsoluteY,
	IndexedZeroX,
	IndexedZeroY,
};

#define OP_DECL(name)	template <class AddrMode> \
						uint8_t name##();
#define OP_DEF(name)	template <class AddrMode> \
						uint8_t Cpu6502::##name()

#define ADDR_MODE_DECL(name)	struct AddrMode##name \
								{ \
									static const AddrMode addrMode = AddrMode::##name; \
									Cpu6502& cpu; \
									AddrMode##name( Cpu6502& cpui ) : cpu( cpui ) {}; \
									inline void operator()( CpuAddrInfo& addrInfo ); \
								};

#define ADDR_MODE_DEF(name)	void Cpu6502::AddrMode##name::operator()( CpuAddrInfo& addrInfo )

#define _OP_ADDR(num,name,address,ops,advance,cycles)	case num: \
												{ \
													opCode = num; \
													retVal = cpuCycle_t( cycles + Cpu6502::##name<AddrMode##address>() ); \
													mnemonic = #name; \
													operands = ops; \
													AdvanceProgram(advance); \
												} break;
#define OP_ADDR(num,name,address,ops,cycles) _OP_ADDR(num,name,address,ops,ops,cycles)
#define OP(num,name,ops,cycles) _OP_ADDR(num,name,None,ops,ops,cycles)
#define OP_JMP(num,name,ops,cycles) _OP_ADDR(num,name,None,ops,0,cycles)
#define OP_ILLEGAL(num) case num: { ; } break;


const StatusBit	STATUS_NONE			= 0x00;
const StatusBit	STATUS_ALL			= ~STATUS_NONE;
const StatusBit	STATUS_CARRY		= ( 1 << 0 );
const StatusBit	STATUS_ZERO			= ( 1 << 1 );
const StatusBit	STATUS_INTERRUPT	= ( 1 << 2 );
const StatusBit	STATUS_DECIMAL		= ( 1 << 3 );
const StatusBit	STATUS_BREAK		= ( 1 << 4 );
const StatusBit	STATUS_UNUSED		= ( 1 << 5 );
const StatusBit	STATUS_OVERFLOW		= ( 1 << 6 );
const StatusBit	STATUS_NEGATIVE		= ( 1 << 7 );


union ProcessorStatus
{
	struct ProcessorStatusSemantic
	{
		uint8_t c : 1;
		uint8_t z : 1;
		uint8_t i : 1;
		uint8_t d : 1;

		uint8_t u : 1;
		uint8_t b : 1;
		uint8_t v : 1;
		uint8_t n : 1;
	} bit;

	uint8_t byte;
};


struct CpuAddrInfo
{
	uint32_t	addr;
	uint8_t		offset;
	uint8_t		value;
	bool		isAccumulator;
	bool		isImmediate;
};


struct CpuDebugMetrics
{
	uint32_t loadCnt;
	uint32_t storeCnt;
};


struct Cpu6502
{
	static const uint32_t InvalidAddress = ~0x00;

	uint16_t nmiVector;
	uint16_t irqVector;
	uint16_t resetVector;

	wtSystem* system;

	cpuCycle_t cycle;
	cpuCycle_t instructionCycles;

	uint8_t opCode;

#if DEBUG_ADDR == 1
	std::stringstream debugAddr;
	std::ofstream logFile;
	bool printToOutput = false;
	int logFrameCount = 30;
#endif
	CpuDebugMetrics dbgMetrics;

	bool forceStop = false;

	bool interruptTriggered;
	bool oamInProcess;

	uint8_t X;
	uint8_t Y;
	uint8_t A;
	uint8_t SP;
	ProcessorStatus P;
	uint16_t PC;

	void Reset()
	{
		PC = resetVector;

		X = 0;
		Y = 0;
		A = 0;
		SP = 0xFD;

		P.byte = 0;
		P.bit.i = 1;
		P.bit.b = 1;

		instructionCycles = cpuCycle_t( 0 );
		cycle = cpuCycle_t(0); // FIXME? Test log starts cycles at 7. Is there a BRK at power up?
	}

	Cpu6502()
	{
		Reset();
	}

	bool Step( const cpuCycle_t& nextCycle );

//private:

	OP_DECL( Illegal )
	OP_DECL( STA )
	OP_DECL( STX )
	OP_DECL( STY )
	OP_DECL( LDA )
	OP_DECL( LDX )
	OP_DECL( LDY )
	OP_DECL( TXS )
	OP_DECL( TAX )
	OP_DECL( TAY )
	OP_DECL( TSX )
	OP_DECL( TYA )
	OP_DECL( TXA )
	OP_DECL( INX )
	OP_DECL( INY )
	OP_DECL( DEX )
	OP_DECL( DEY )
	OP_DECL( INC )
	OP_DECL( DEC )
	OP_DECL( ADC )
	OP_DECL( SBC )
	OP_DECL( CMP )
	OP_DECL( CPX )
	OP_DECL( CPY )
	OP_DECL( PHP )
	OP_DECL( PHA )
	OP_DECL( PLA )
	OP_DECL( PLP )
	OP_DECL( SEC )
	OP_DECL( SEI )
	OP_DECL( SED )
	OP_DECL( CLI )
	OP_DECL( CLC )
	OP_DECL( CLV )
	OP_DECL( CLD )

	OP_DECL( ASL )
	OP_DECL( AND )
	OP_DECL( BIT )
	OP_DECL( EOR )
	OP_DECL( LSR )
	OP_DECL( ORA )
	OP_DECL( ROL )
	OP_DECL( ROR )

	OP_DECL( BRK )
	OP_DECL( JMP )
	OP_DECL( JMPI )
	OP_DECL( JSR )
	OP_DECL( RTS )
	OP_DECL( RTI )

	OP_DECL( BMI )
	OP_DECL( BVS )
	OP_DECL( BCS )
	OP_DECL( BEQ )
	OP_DECL( BPL )
	OP_DECL( BVC )
	OP_DECL( BCC )
	OP_DECL( BNE )

	OP_DECL( NOP )

	OP_DECL( SKB )
	OP_DECL( SKW )

	ADDR_MODE_DECL( None )
	ADDR_MODE_DECL( Absolute )
	ADDR_MODE_DECL( Zero )
	ADDR_MODE_DECL( Immediate )
	ADDR_MODE_DECL( IndexedIndirect )
	ADDR_MODE_DECL( IndirectIndexed )
	ADDR_MODE_DECL( Accumulator )
	ADDR_MODE_DECL( IndexedAbsoluteX )
	ADDR_MODE_DECL( IndexedAbsoluteY )
	ADDR_MODE_DECL( IndexedZeroX )
	ADDR_MODE_DECL( IndexedZeroY )

	cpuCycle_t Exec();

	static bool CheckSign( const uint16_t checkValue );
	static bool CheckCarry( const uint16_t checkValue );
	static bool CheckZero( const uint16_t checkValue );
	static bool CheckOverflow( const uint16_t src, const uint16_t temp, const uint8_t finalValue );

	uint8_t NMI();
	uint8_t IRQ();

	void IndexedAbsolute( const uint8_t& reg, CpuAddrInfo& addrInfo );
	void IndexedZero( const uint8_t& reg, CpuAddrInfo& addrInfo );

	void Push( const uint8_t value );
	uint8_t Pull();
	void PushByte( const uint16_t value );
	uint16_t PullWord();

	void AdvanceProgram( const uint16_t places );
	uint8_t ReadOperand( const uint16_t offset ) const;
	uint16_t ReadAddressOperand() const;

	void SetAluFlags( const uint16_t value );

	uint16_t CombineIndirect( const uint8_t lsb, const uint8_t msb, const uint32_t wrap );

	uint8_t AddressCrossesPage( const uint16_t address, const uint16_t offset );
	uint8_t Branch( const bool takeBranch );
	
	template <class AddrFunctor>
	uint8_t Read();

	template <class AddrFunctor>
	void Write( const uint8_t value );

	cpuCycle_t LookupFunction( const uint16_t instrBegin, const uint8_t byteCode );

	void DebugPrintIndexZero( const uint8_t& reg, const uint32_t address, const uint32_t targetAddresss );
};