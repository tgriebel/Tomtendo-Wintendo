#pragma once

#include <map>
#include <sstream>
#include "common.h"

typedef byte StatusBit;

struct InstrParams;
struct Cpu6502;
struct OpCodeMap;
struct AddrMapTuple;
struct InstructionMapTuple;
struct DisassemblerMapTuple;
class NesSystem;

typedef byte( Cpu6502::* Instruction )( const InstrParams& params );
typedef byte&( Cpu6502::* AddrFunction )( const InstrParams& params, word& outValue );


enum StatusBitIndex
{
	BIT_CARRY = 0,
	BIT_ZERO = 1,
	BIT_INTERRUPT = 2,
	BIT_DECIMAL = 3,
	BIT_BREAK = 4,
	BIT_UNUSED = 5,
	BIT_OVERFLOW = 6,
	BIT_NEGATIVE = 7,
};


const StatusBit	STATUS_NONE			= 0x00;
const StatusBit	STATUS_ALL			= 0xFF;
const StatusBit	STATUS_CARRY		= ( 1 << BIT_CARRY );
const StatusBit	STATUS_ZERO			= ( 1 << BIT_ZERO );
const StatusBit	STATUS_INTERRUPT	= ( 1 << BIT_INTERRUPT );
const StatusBit	STATUS_DECIMAL		= ( 1 << BIT_DECIMAL );
const StatusBit	STATUS_BREAK		= ( 1 << BIT_BREAK );
const StatusBit	STATUS_UNUSED		= ( 1 << BIT_UNUSED );
const StatusBit	STATUS_OVERFLOW		= ( 1 << BIT_OVERFLOW );
const StatusBit	STATUS_NEGATIVE		= ( 1 << BIT_NEGATIVE );


enum RegisterIndex : byte
{
	iX = 0,
	iY = 1,
	iA = 2,
	iSP = 3,
	iP = 4,
	iNil,
	regCount = iNil,
};


enum OpCode : byte
{
	BRK = 0x00,
	JSR = 0x20,
	RTI = 0x40,
	RTS = 0x60,

	BPL = 0x10,
	BMI = 0x30,
	BVC = 0x50,
	BVS = 0x70,
	BCC = 0x90,
	BCS = 0xB0,
	BNE = 0xD0,
	BEQ = 0xF0,

	PHP = 0x08,
	PLP = 0x28,
	PHA = 0x48,
	PLA = 0x68,
	DEY = 0x88,
	TAY = 0xA8,
	INY = 0xC8,
	INX = 0xE8,

	CLC = 0x18,
	SEC = 0x38,
	CLI = 0x58,
	SEI = 0x78,
	TYA = 0x98,
	CLV = 0xB8,
	CLD = 0xD8,
	SED = 0xF8,

	TXA = 0x8A,
	TXS = 0x9A,
	TAX = 0xAA,
	TSX = 0xBA,
	DEX = 0xCA,
	Nop = 0xEA,

	JMP	= 0x4C,
	JMPI = 0x6C,

	BIT = 0x20,
	STY = 0x80,
	LDY = 0xA0,
	CPY = 0xC0,
	CPX = 0xE0,

	ORA = 0x01,
	AND = 0x21,
	EOR = 0x41,
	ADC = 0x61,
	STA = 0x81,
	LDA = 0xA1,
	CMP = 0xC1,
	SBC = 0xE1,

	ASL = 0x02,
	ROL = 0x22,
	LSR = 0x42,
	ROR = 0x62,
	STX = 0x82,
	LDX = 0xA2,
	DEC = 0xC2,
	INC = 0xE2,
};


enum FormatCode
{
	MATH_OP		= 0x01,
	STACK_OP	= 0x02,
	JUMP_OP		= 0x03,
	SYSTEM_OP	= 0x04,
	REGISTER_OP	= 0x05,
	COMPARE_OP	= 0x06,
	STATUS_OP	= 0x07,
	BRANCH_OP	= 0x08,
	ADDR_OP		= 0x09,
	LOAD_OP		= 0x10,
	STORE_OP	= 0x11,
	BITWISE_OP	= 0x12,
	ILLEGAL		= 0xFF,
};


struct InstrParams
{
	byte			param0;
	byte			param1;
	byte			cycles;
	AddrFunction	getAddr;

	InstrParams() : cycles( 0 ), getAddr( NULL ), param0( 0 ), param1( 0 ) {}
};


struct InstructionMapTuple
{
	char			mnemonic[16];
	byte			byteCode;
	FormatCode		format;
	Instruction		instr;
	AddrFunction	addrFunc;
	byte			operands;
	byte			cycles;
};


struct Cpu6502
{
	half nmiVector;
	half irqVector;
	half resetVector;

	NesSystem* system;

	cpuCycle_t cycle;

#if DEBUG_ADDR == 1
	std::stringstream debugAddr;
	std::ofstream logFile;
#endif

	bool forceStop;
	half forceStopAddr;

	bool resetTriggered;
	bool interruptTriggered;

	byte regs[regCount];

	byte& X		= regs[iX];
	byte& Y		= regs[iY];
	byte& A		= regs[iA];
	byte& SP	= regs[iSP];
	byte& P		= regs[iP];
	half PC;

	void Reset()
	{
		PC = resetVector;

		X = 0;
		Y = 0;
		A = 0;
		SP = 0xFD;//0xFF;

		P = 0x24;//STATUS_UNUSED | STATUS_BREAK;

		cycle = cpuCycle_t(0);
	}

	cpuCycle_t Cpu6502::Exec();
	bool Step( const cpuCycle_t targetCycles );
	void SetFlags( const StatusBit bit, const bool toggleOn );
	void Push( const byte value );
	byte Pull();
	void PushHalf( const half value );
	half PullHalf();

	byte Illegal( const InstrParams& params );
	byte STA( const InstrParams& params );
	byte STX( const InstrParams& params );
	byte STY( const InstrParams& params );
	byte LDA( const InstrParams& params );
	byte LDX( const InstrParams& params );
	byte LDY( const InstrParams& params );
	byte TXS( const InstrParams& params );
	byte TAX( const InstrParams& params );
	byte TAY( const InstrParams& params );
	byte TSX( const InstrParams& params );
	byte TYA( const InstrParams& params );
	byte TXA( const InstrParams& params );
	byte INX( const InstrParams& params );
	byte INY( const InstrParams& params );
	byte DEX( const InstrParams& params );
	byte DEY( const InstrParams& params );
	byte INC( const InstrParams& params );
	byte DEC( const InstrParams& params );
	byte ADC( const InstrParams& params );
	byte SBC( const InstrParams& params );
	byte CMP( const InstrParams& params );
	byte CPX( const InstrParams& params );
	byte CPY( const InstrParams& params );
	byte PHP( const InstrParams& params );
	byte PHA( const InstrParams& params );
	byte PLA( const InstrParams& params );
	byte PLP( const InstrParams& params );
	byte SEC( const InstrParams& params );
	byte SEI( const InstrParams& params );
	byte SED( const InstrParams& params );
	byte CLI( const InstrParams& params );
	byte CLC( const InstrParams& params );
	byte CLV( const InstrParams& params );
	byte CLD( const InstrParams& params );

	byte ASL( const InstrParams& params );
	byte AND( const InstrParams& params );
	byte BIT( const InstrParams& params );
	byte EOR( const InstrParams& params );
	byte LSR( const InstrParams& params );
	byte ORA( const InstrParams& params );
	byte ROL( const InstrParams& params );
	byte ROR( const InstrParams& params );

	byte BRK( const InstrParams& params );
	byte JMP( const InstrParams& params );
	byte JMPI( const InstrParams& params );
	byte JSR( const InstrParams& params );
	byte RTS( const InstrParams& params );
	byte RTI( const InstrParams& params );

	byte BMI( const InstrParams& params );
	byte BVS( const InstrParams& params );
	byte BCS( const InstrParams& params );
	byte BEQ( const InstrParams& params );
	byte BPL( const InstrParams& params );
	byte BVC( const InstrParams& params );
	byte BCC( const InstrParams& params );
	byte BNE( const InstrParams& params );

	byte NMI( const InstrParams& params );
	byte IRQ( const InstrParams& params );

	byte NOP( const InstrParams& params );
	 
	byte& Absolute( const InstrParams& params, word& outValue );
	byte& Zero( const InstrParams& params, word& outValue );
	byte& Immediate( const InstrParams& params, word& outValue );
	byte& IndexedIndirect( const InstrParams& params, word& address );
	byte& IndirectIndexed( const InstrParams& params, word& outValue );
	byte& Accumulator( const InstrParams& params, word& outValue );

	byte& IndexedAbsolute( const InstrParams& params, word& address, const byte& reg );
	byte& IndexedAbsoluteX( const InstrParams& params, word& address );
	byte& IndexedAbsoluteY( const InstrParams& params, word& address );

	byte& IndexedZero( const InstrParams& params, word& address, const byte& reg );
	byte& IndexedZeroX( const InstrParams& params, word& address );
	byte& IndexedZeroY( const InstrParams& params, word& address );

private:
	static bool CheckSign( const half checkValue );
	static bool CheckCarry( const half checkValue );
	static bool CheckZero( const half checkValue );
	static bool CheckOverflow( const half src, const half temp, const byte finalValue );
	static bool IsIllegalOp( const byte opCode );

	void SetAluFlags( const half value );

	half CombineIndirect( const byte lsb, const byte msb, const word wrap );

	void Branch( const InstrParams& params, const bool takeBranch );
	byte& Read( const InstrParams& params );
	void Write( const InstrParams& params, const byte value );
};


const half NumInstructions = 256;
static const InstructionMapTuple InstructionMap[NumInstructions] =
{
	{ "BRK",	BRK,	SYSTEM_OP,	&Cpu6502::BRK,		nullptr,					0, 7 },	//	00
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::IndexedIndirect,	1, 6 },	//	01
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	02
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	03
	{ "*NOP",	Nop,	ILLEGAL,	&Cpu6502::NOP,		nullptr,					1, 0 },	//	04
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::Zero,				1, 3 },	//	05
	{ "ASL",	ASL,	BITWISE_OP,	&Cpu6502::ASL,		&Cpu6502::Zero,				1, 5 },	//	06
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	07
	{ "PHP",	PHP,	STACK_OP,	&Cpu6502::PHP,		nullptr,					0, 3 },	//	08
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::Immediate,		1, 2 },	//	09
	{ "ASL",	ASL,	BITWISE_OP,	&Cpu6502::ASL,		&Cpu6502::Accumulator,		0, 2 },	//	0A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	0B
	{ "*NOP",	Nop,	ILLEGAL,	&Cpu6502::NOP,		nullptr,					2, 0 },	//	0C
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::Absolute,			2, 4 },	//	0D
	{ "ASL",	ASL,	BITWISE_OP,	&Cpu6502::ASL,		&Cpu6502::Absolute,			2, 6 },	//	0E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	0F
	{ "BPL",	BPL,	BRANCH_OP,	&Cpu6502::BPL,		nullptr,					1, 2 },	//	10
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::IndirectIndexed,	1, 5 },	//	11
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	// 	12
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	13
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	14
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::IndexedZeroX,		1, 4 },	//	15
	{ "ASL",	ASL,	BITWISE_OP,	&Cpu6502::ASL,		&Cpu6502::IndexedZeroX,		1, 6 },	//	16
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	17
	{ "CLC",	CLC,	COMPARE_OP,	&Cpu6502::CLC,		nullptr,					0, 2 },	//	18
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	19
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	1A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	1B
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	1C
	{ "ORA",	ORA,	BITWISE_OP,	&Cpu6502::ORA,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	1D
	{ "ASL",	ASL,	BITWISE_OP,	&Cpu6502::ASL,		&Cpu6502::IndexedAbsoluteX,	2, 7 },	//	1E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	1F
	{ "JSR",	JSR,	JUMP_OP,	&Cpu6502::JSR,		nullptr,					2, 6 },	//	20
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::IndexedIndirect,	1, 6 },	//	21
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	22
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	23
	{ "BIT",	BIT,	BITWISE_OP,	&Cpu6502::BIT,		&Cpu6502::Zero,				1, 3 },	//	24
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::Zero,				1, 3 },	// 	25
	{ "ROL",	ROL,	BITWISE_OP,	&Cpu6502::ROL,		&Cpu6502::Zero,				1, 5 },	//	26
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	27
	{ "PLP",	PLP,	STACK_OP,	&Cpu6502::PLP,		nullptr,					0, 4 },	//	28
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::Immediate,		1, 2 },	//	29
	{ "ROL",	ROL,	BITWISE_OP,	&Cpu6502::ROL,		&Cpu6502::Accumulator,		0, 2 },	//	2A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	2B
	{ "BIT",	BIT,	BITWISE_OP,	&Cpu6502::BIT,		&Cpu6502::Absolute,			2, 4 },	//	2C
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::Absolute,			2, 4 },	//	2D
	{ "ROL",	ROL,	BITWISE_OP,	&Cpu6502::ROL,		&Cpu6502::Absolute,			2, 6 },	//	2E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	2F
	{ "BMI",	BMI,	BRANCH_OP,	&Cpu6502::BMI,		nullptr,					1, 2 },	//	30
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::IndirectIndexed,	1, 5 },	//	31
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	32
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	33
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	34
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::IndexedZeroX,		1, 4 },	//	35
	{ "ROL",	ROL,	BITWISE_OP,	&Cpu6502::ROL,		&Cpu6502::IndexedZeroX,		1, 6 },	//	36
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	37
	{ "SEC",	SEC,	STATUS_OP,	&Cpu6502::SEC,		nullptr,					0, 2 },	//	38
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	39
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	3A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	3B
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	3C
	{ "AND",	AND,	BITWISE_OP,	&Cpu6502::AND,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	3D
	{ "ROL",	ROL,	BITWISE_OP,	&Cpu6502::ROL,		&Cpu6502::IndexedAbsoluteX,	2, 7 },	//	3E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	3F
	{ "RTI",	RTI,	JUMP_OP,	&Cpu6502::RTI,		nullptr,					0, 6 },	//	40
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::IndexedIndirect,	1, 6 },	//	41
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	42
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	43
	{ "*NOP",	Nop,	ILLEGAL,	&Cpu6502::NOP,		nullptr,					1, 0 },	//	44
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::Zero,				1, 3 },	//	45
	{ "LSR",	LSR,	BITWISE_OP,	&Cpu6502::LSR,		&Cpu6502::Zero,				1, 5 },	//	46
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	47
	{ "PHA",	PHA,	STACK_OP,	&Cpu6502::PHA,		nullptr,					0, 3 },	//	48
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::Immediate,		1, 2 },	//	49
	{ "LSR",	LSR,	BITWISE_OP,	&Cpu6502::LSR,		&Cpu6502::Accumulator,		0, 2 },	//	4A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	4B
	{ "JMP",	JMP,	BITWISE_OP,	&Cpu6502::JMP,		nullptr,					2, 3 },	//	4C
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::Absolute,			2, 4 },	//	4D
	{ "LSR",	LSR,	BITWISE_OP,	&Cpu6502::LSR,		&Cpu6502::Absolute,			2, 6 },	//	4E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	4F
	{ "BVC",	BVC,	ILLEGAL,	&Cpu6502::BVC,		nullptr,					1, 2 },	//	50
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::IndirectIndexed,	1, 5 },	//	51
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	52
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	53
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	54
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::IndexedZeroX,		1, 4 },	//	55
	{ "LSR",	LSR,	LOAD_OP,	&Cpu6502::LSR,		&Cpu6502::IndexedZeroX,		1, 6 },	//	56
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	57
	{ "CLI",	CLI,	STATUS_OP,	&Cpu6502::CLI,		nullptr,					0, 2 },	//	58
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	59
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	5A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	5B
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	5C
	{ "EOR",	EOR,	BITWISE_OP,	&Cpu6502::EOR,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	5D
	{ "LSR",	LSR,	BITWISE_OP,	&Cpu6502::LSR,		&Cpu6502::IndexedAbsoluteX,	2, 7 },	//	5E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	5F
	{ "RTS",	RTS,	JUMP_OP,	&Cpu6502::RTS,		nullptr,					0, 6 },	//	60
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::IndexedIndirect,	1, 6 },	//	61
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	62
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	63
	{ "*NOP",	Nop,	ILLEGAL,	&Cpu6502::NOP,		nullptr,					1, 0 },	//	64
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::Zero,				1, 3 },	//	65
	{ "ROR",	ROR,	BITWISE_OP,	&Cpu6502::ROR,		&Cpu6502::Zero,				1, 5 },	//	66
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	67
	{ "PLA",	PLA,	STACK_OP,	&Cpu6502::PLA,		nullptr,					0, 4 },	//	68
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::Immediate,		1, 2 },	//	69
	{ "ROR",	ROR,	BITWISE_OP,	&Cpu6502::ROR,		&Cpu6502::Accumulator,		0, 2 },	//	6A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	6B
	{ "JMP",	JMPI,	JUMP_OP,	&Cpu6502::JMPI,		nullptr,					2, 5 },	//	6C
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::Absolute,			2, 4 },	//	6D
	{ "ROR",	ROR,	BITWISE_OP,	&Cpu6502::ROR,		&Cpu6502::Absolute,			2, 6 },	//	6E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	6F
	{ "BVS",	BVS,	BRANCH_OP,	&Cpu6502::BVS,		nullptr,					1, 2 },	//	70
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::IndirectIndexed,	1, 5 },	//	71
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	72
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	73
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	74
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::IndexedZeroX,		1, 4 },	//	75
	{ "ROR",	ROR,	BITWISE_OP,	&Cpu6502::ROR,		&Cpu6502::IndexedZeroX,		1, 6 },	//	76
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	77
	{ "SEI",	SEI,	STATUS_OP,	&Cpu6502::SEI,		nullptr,					0, 2 },	//	78
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	79
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	7A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	7B
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	7C
	{ "ADC",	ADC,	MATH_OP,	&Cpu6502::ADC,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	7D
	{ "ROR",	ROR,	BITWISE_OP,	&Cpu6502::ROR,		&Cpu6502::IndexedAbsoluteX,	2, 7 },	//	7E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	7F
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	80
	{ "STA",	STA,	STORE_OP,	&Cpu6502::STA,		&Cpu6502::IndexedIndirect,	1, 6 },	//	81
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	82
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	83
	{ "STY",	STY,	STORE_OP,	&Cpu6502::STY,		&Cpu6502::Zero,				1, 3 },	//	84
	{ "STA",	STA,	STORE_OP,	&Cpu6502::STA,		&Cpu6502::Zero,				1, 3 },	//	85
	{ "STX",	STX,	STORE_OP,	&Cpu6502::STX,		&Cpu6502::Zero,				1, 3 },	//	86
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	87
	{ "DEY",	DEY,	MATH_OP,	&Cpu6502::DEY,		nullptr,					0, 2 },	//	88
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	89
	{ "TXA",	TXA,	REGISTER_OP,&Cpu6502::TXA,		nullptr,					0, 2 },	//	8A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	8B
	{ "STY",	STY,	STORE_OP,	&Cpu6502::STY,		&Cpu6502::Absolute,			2, 4 },	//	8C
	{ "STA",	STA,	STORE_OP,	&Cpu6502::STA,		&Cpu6502::Absolute,			2, 4 },	//	8D
	{ "STX",	STX,	STORE_OP,	&Cpu6502::STX,		&Cpu6502::Absolute,			2, 4 },	//	8E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	8F
	{ "BCC",	BCC,	BRANCH_OP,	&Cpu6502::BCC,		nullptr,					1, 2 },	//	90
	{ "STA",	STA,	STORE_OP,	&Cpu6502::STA,		&Cpu6502::IndirectIndexed,	1, 6 },	//	91
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	92
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	93
	{ "STY",	STY,	STORE_OP,	&Cpu6502::STY,		&Cpu6502::IndexedZeroX,		1, 4 },	//	94
	{ "STA",	STA,	STORE_OP,	&Cpu6502::STA,		&Cpu6502::IndexedZeroX,		1, 4 },	//	95
	{ "STX",	STX,	STORE_OP,	&Cpu6502::STX,		&Cpu6502::IndexedZeroY,		1, 4 },	//	96
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	97
	{ "TYA",	TYA,	REGISTER_OP,&Cpu6502::TYA,		nullptr,					0, 2 },	//	98
	{ "STA",	STA,	STORE_OP,	&Cpu6502::STA,		&Cpu6502::IndexedAbsoluteY,	2, 5 },	//	99
	{ "TXS",	TXS,	REGISTER_OP,&Cpu6502::TXS,		nullptr,					0, 2 },	//	9A
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	9B
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	9C
	{ "STA",	STA,	STORE_OP,	&Cpu6502::STA,		&Cpu6502::IndexedAbsoluteX,	2, 5 },	//	9D
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	9E
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	9F
	{ "LDY",	LDY,	LOAD_OP,	&Cpu6502::LDY,		&Cpu6502::Immediate,		1, 2 },	//	A0
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::IndexedIndirect,	1, 6 },	//	A1
	{ "LDX",	LDX,	LOAD_OP,	&Cpu6502::LDX,		&Cpu6502::Immediate,		1, 2 },	//	A2
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	A3
	{ "LDY",	LDY,	LOAD_OP,	&Cpu6502::LDY,		&Cpu6502::Zero,				1, 3 },	//	A4
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::Zero,				1, 3 },	//	A5
	{ "LDX",	LDX,	LOAD_OP,	&Cpu6502::LDX,		&Cpu6502::Zero,				1, 3 },	//	A6
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	A7
	{ "TAY",	TAY,	REGISTER_OP,&Cpu6502::TAY,		nullptr,					0, 2 },	//	A8
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::Immediate,		1, 2 },	//	A9
	{ "TAX",	TAX,	REGISTER_OP,&Cpu6502::TAX,		nullptr,					0, 2 },	//	AA
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	AB
	{ "LDY",	LDY,	LOAD_OP,	&Cpu6502::LDY,		&Cpu6502::Absolute,			2, 4 },	//	AC
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::Absolute,			2, 4 },	//	AD
	{ "LDX",	LDX,	LOAD_OP,	&Cpu6502::LDX,		&Cpu6502::Absolute,			2, 4 },	//	AE
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	AF
	{ "BCS",	BCS,	BRANCH_OP,	&Cpu6502::BCS,		nullptr,					1, 2 },	//	B0
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::IndirectIndexed,	1, 5 },	//	B1
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	B2
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	B3
	{ "LDY",	LDY,	LOAD_OP,	&Cpu6502::LDY,		&Cpu6502::IndexedZeroX,		1, 4 },	//	B4
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::IndexedZeroX,		1, 4 },	//	B5
	{ "LDX",	LDX,	LOAD_OP,	&Cpu6502::LDX,		&Cpu6502::IndexedZeroY,		1, 4 },	//	B6
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	B7
	{ "CLV",	CLV,	STATUS_OP,	&Cpu6502::CLV,		nullptr,					0, 2 },	//	B8
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	B9
	{ "TSX",	TSX,	REGISTER_OP,&Cpu6502::TSX,		nullptr,					0, 2 },	//	BA
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	BB
	{ "LDY",	LDY,	LOAD_OP,	&Cpu6502::LDY,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	BC
	{ "LDA",	LDA,	LOAD_OP,	&Cpu6502::LDA,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	BD
	{ "LDX",	LDX,	LOAD_OP,	&Cpu6502::LDX,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	BE
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	BF
	{ "CPY",	CPY,	COMPARE_OP,	&Cpu6502::CPY,		&Cpu6502::Immediate,		1, 2 },	//	C0
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::IndexedIndirect,	1, 6 },	//	C1
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	C2
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	C3
	{ "CPY",	CPY,	COMPARE_OP,	&Cpu6502::CPY,		&Cpu6502::Zero,				1, 3 },	//	C4
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::Zero,				1, 3 },	//	C5
	{ "DEC",	DEC,	MATH_OP,	&Cpu6502::DEC,		&Cpu6502::Zero,				1, 5 },	//	C6
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	C7
	{ "INY",	INY,	MATH_OP,	&Cpu6502::INY,		nullptr,					0, 2 },	//	C8
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::Immediate,		1, 2 },	//	C9
	{ "DEX",	DEX,	MATH_OP,	&Cpu6502::DEX,		nullptr,					0, 2 },	//	CA
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	CB
	{ "CPY",	CPY,	COMPARE_OP,	&Cpu6502::CPY,		&Cpu6502::Absolute,			2, 4 },	//	CC
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::Absolute,			2, 4 },	//	CD
	{ "DEC",	DEC,	MATH_OP,	&Cpu6502::DEC,		&Cpu6502::Absolute,			2, 6 },	//	CE
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	CF
	{ "BNE",	BNE,	BRANCH_OP,	&Cpu6502::BNE,		nullptr,					1, 2 },	//	D0
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::IndirectIndexed,	1, 5 },	//	D1
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	D2
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	D3
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	D4
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::IndexedZeroX,		1, 4 },	//	D5
	{ "DEC",	DEC,	MATH_OP,	&Cpu6502::DEC,		&Cpu6502::IndexedZeroX,		1, 6 },	//	D6
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	D7
	{ "CLD",	CLD,	STATUS_OP,	&Cpu6502::CLD,		nullptr,					0, 2 },	//	D8
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	D9
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	DA
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	DB
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	DC
	{ "CMP",	CMP,	COMPARE_OP,	&Cpu6502::CMP,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	DD
	{ "DEC",	DEC,	MATH_OP,	&Cpu6502::DEC,		&Cpu6502::IndexedAbsoluteX,	2, 7 },	//	DE
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	DF
	{ "CPX",	CPX,	COMPARE_OP,	&Cpu6502::CPX,		&Cpu6502::Immediate,		1, 2 },	//	E0
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::IndexedIndirect,	1, 6 },	//	E1
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	E2
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	E3
	{ "CPX",	CPX,	COMPARE_OP,	&Cpu6502::CPX,		&Cpu6502::Zero,				1, 3 },	//	E4
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::Zero,				1, 3 },	//	E5
	{ "INC",	INC,	MATH_OP,	&Cpu6502::INC,		&Cpu6502::Zero,				1, 5 },	//	E6
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	E7
	{ "INX",	INX,	MATH_OP,	&Cpu6502::INX,		nullptr,					0, 2 },	//	E8
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::Immediate,		1, 2 },	//	E9
	{ "NOP",	Nop,	SYSTEM_OP,	&Cpu6502::NOP,		nullptr,					0, 2 },	//	EA
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	EB
	{ "CPX",	CPX,	COMPARE_OP,	&Cpu6502::CPX,		&Cpu6502::Absolute,			2, 4 },	//	EC
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::Absolute,			2, 4 },	//	ED
	{ "INC",	INC,	MATH_OP,	&Cpu6502::INC,		&Cpu6502::Absolute,			2, 6 },	//	EE
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	EF
	{ "BEQ",	BEQ,	BRANCH_OP,	&Cpu6502::BEQ,		nullptr,					1, 2 },	//	F0
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::IndirectIndexed,	1, 5 },	//	F1
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	F2
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	F3
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	F4
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::IndexedZeroX,		1, 4 },	//	F5
	{ "INC",	INC,	MATH_OP,	&Cpu6502::INC,		&Cpu6502::IndexedZeroX,		1, 6 },	//	F6
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	F7
	{ "SED",	SED,	STATUS_OP,	&Cpu6502::SED,		nullptr,					0, 2 },	//	F8
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::IndexedAbsoluteY,	2, 4 },	//	F9
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	FA
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	FB
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	FC
	{ "SBC",	SBC,	MATH_OP,	&Cpu6502::SBC,		&Cpu6502::IndexedAbsoluteX,	2, 4 },	//	FD
	{ "INC",	INC,	MATH_OP,	&Cpu6502::INC,		&Cpu6502::IndexedAbsoluteX,	2, 7 },	//	FE
	{ "???",	Nop,	ILLEGAL,	&Cpu6502::Illegal,	nullptr,					0, 0 },	//	FF
};