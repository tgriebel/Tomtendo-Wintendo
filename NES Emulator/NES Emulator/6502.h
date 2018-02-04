#pragma once

#include "types_common.h"

typedef byte			StatusBit;

struct InstrParams;

typedef byte( *Instruction )( const InstrParams& params );
typedef byte& ( *AddrFunction )( const InstrParams& params, word& outValue );


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


const StatusBit	STATUS_NONE = 0x00;
const StatusBit	STATUS_ALL = 0xFF;
const StatusBit	STATUS_CARRY = ( 1 << BIT_CARRY );
const StatusBit	STATUS_ZERO = ( 1 << BIT_ZERO );
const StatusBit	STATUS_INTERRUPT = ( 1 << BIT_INTERRUPT );
const StatusBit	STATUS_DECIMAL = ( 1 << BIT_DECIMAL );
const StatusBit	STATUS_BREAK = ( 1 << BIT_BREAK );
const StatusBit	STATUS_UNUSED = ( 1 << BIT_UNUSED );
const StatusBit	STATUS_OVERFLOW = ( 1 << BIT_OVERFLOW );
const StatusBit	STATUS_NEGATIVE = ( 1 << BIT_NEGATIVE );


enum RegisterIndex
{
	iX = 0,
	iY = 1,
	iA = 2,
	iSP = 3,
	iP = 4,
	iNil,
	regCount = iNil,
};


struct InstrParams
{
	RegisterIndex	reg0;
	RegisterIndex	reg1;
	byte			param0;
	byte			param1;
	byte			cycles;
	StatusBit		statusFlag;
	AddrFunction	getAddr;
};

namespace Instr
{
	byte Store( const InstrParams& params );
	byte Load( const InstrParams& params );
	byte Move( const InstrParams& params );
	byte IncReg( const InstrParams& params );
	byte DecReg( const InstrParams& params );
	byte INC( const InstrParams& params );
	byte DEC( const InstrParams& params );
	byte ADC( const InstrParams& params );
	byte SBC( const InstrParams& params );
	byte Compare( const InstrParams& params );
	byte Push( const InstrParams& params );
	byte Pop( const InstrParams& params );
	byte PLP( const InstrParams& params );
	byte SetStatusBit( const InstrParams& params );
	byte ClearStatusBit( const InstrParams& params );

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

	byte BranchOnSet( const InstrParams& params );
	byte BranchOnClear( const InstrParams& params );

	byte NOP( const InstrParams& params );

	byte& Absolute( const InstrParams& params, word& outValue );
	byte& IndexedAbsolute( const InstrParams& params, word& outValue );
	byte& Zero( const InstrParams& params, word& outValue );
	byte& IndexedZero( const InstrParams& params, word& outValue );
	byte& Immediate( const InstrParams& params, word& outValue );
	byte& Indirect( const InstrParams& params, word& outValue );
	byte& IndexedIndirect( const InstrParams& params, word& outValue );
	byte& IndirectIndexed( const InstrParams& params, word& outValue );
	byte& Accumulator( const InstrParams& params, word& outValue );
}


struct Cpu6502
{
	static const half codebase = 0x0600;
	static const half stackbase = 0x0100;
	static const half virtualMemory = 0xFFFF;
	static const half ram = 0x0800;

	byte memory[virtualMemory];
	byte registers[regCount];

	byte& X = registers[iX];
	byte& Y = registers[iY];
	byte& A = registers[iA];
	byte& SP = registers[iSP];
	byte& P = registers[iP];
	half PC;

	void Reset()
	{
		memset( memory, 0, virtualMemory );
		SP = 0xFF;
		PC = codebase;

		X = 0;
		Y = 0;
		A = 0;

		P = STATUS_UNUSED | STATUS_BREAK;
	}
};

Cpu6502 cpu;


enum OpCode
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

	JMP = 0x4C,
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


// For aaabbbcc instructions, these code the bbbcc portion.
// The number suffix in the address enum stands for the cc bits. No op-codes end in 11.
enum AddrMode
{
	IMM0 = 0x00,
	ZP0 = 0x04,
	ABS0 = 0x0C,
	ZPX0 = 0x14,
	ABSX0 = 0x1C,

	ZPIX1 = 0x01,
	ZP1 = 0x05,
	IMM1 = 0x09,
	ABS1 = 0x0D,
	ZPY1 = 0x11,
	ZPX1 = 0x15,
	ABSY1 = 0x19,
	ABSX1 = 0x1D,

	IMM2 = 0x02,
	ZP2 = 0x06,
	ACC2 = 0x0A,
	ABS2 = 0x0E,
	ZPX2 = 0x16,
	ABSX2 = 0x1E,
};


enum FormatCode
{
	ILLEGAL = 0xFF,
	MATH_OP = 0xFF,
	STACK_OP = 0xFF,
	JUMP_OP = 0xFF,
	SYSTEM_OP = 0xFF,
	REGISTER_OP = 0xFF,
	BRANCH_OP = 0xF0,
	ADDR_OP = 0xE3,
	BITWISE_OP = 0xE3,
};


struct OpCodeMap
{
	char			mnemonic[16];
	byte			byteCode;
	FormatCode		format;
	Instruction		instr;
	RegisterIndex	reg0;
	RegisterIndex	reg1;
	StatusBit		bit;
	byte			operands;
	byte			cycles;
};


struct AddrMapTuple
{
	byte			byteCode;
	char			format[16];
	char			descriptor[16];
	AddrFunction	addrFunc;
	RegisterIndex	reg;
	byte			operands;
	byte			cycles;
};

struct DisassemblerMapTuple
{
	char opCode[16];
	char addrFormat[16];
	byte length;
};


struct InstructionMapTuple
{
	byte			cycles;
	byte			operands;
	InstrParams		params;
	Instruction		instr;
	AddrFunction	addrFunc;

	char			mnemonic[16];
	char			descriptor[16];
};

// These must be ordered from highest enum to lowest
const byte numOpCodes = 57;
OpCodeMap opMap[numOpCodes] =
{
	{ "BRK",	BRK,	SYSTEM_OP,		Instr::BRK,				iNil,	iNil,	STATUS_NONE,		0,	7 },
	{ "JSR",	JSR,	JUMP_OP,		Instr::JSR,				iNil,	iNil,	STATUS_NONE,		2,	2 },
	{ "RTI",	RTI,	JUMP_OP,		Instr::RTI,				iNil,	iNil,	STATUS_NONE,		0,	6 },
	{ "RTS",	RTS,	JUMP_OP,		Instr::RTS,				iNil,	iNil,	STATUS_NONE,		0,	6 },
	{ "JMP",	JMP,	JUMP_OP,		Instr::JMP,				iNil,	iNil,	STATUS_NONE,		2,	3 },
	{ "JMP()",	JMPI,	JUMP_OP,		Instr::JMPI,			iNil,	iNil,	STATUS_NONE,		2,	5 },
	{ "PHP",	PHP,	STACK_OP,		Instr::Push,			iP,		iNil,	STATUS_NONE,		0,	3 },
	{ "PLP",	PLP,	STACK_OP,		Instr::PLP,				iNil,	iNil,	STATUS_NONE,		0,	4 },
	{ "PHA",	PHA,	STACK_OP,		Instr::Push,			iA,		iNil,	STATUS_NONE,		0,	3 },
	{ "PLA",	PLA,	STACK_OP,		Instr::Pop,				iA,		iNil,	STATUS_NONE,		0,	4 },
	{ "TXA",	TXA,	REGISTER_OP,	Instr::Move,			iX,		iA,		STATUS_NONE,		0,	2 },
	{ "TYA",	TYA,	REGISTER_OP,	Instr::Move,			iY,		iA,		STATUS_NONE,		0,	2 },
	{ "TXS",	TXS,	REGISTER_OP,	Instr::Move,			iX,		iSP,	STATUS_NONE,		0,	2 },
	{ "TAY",	TAY,	REGISTER_OP,	Instr::Move,			iA,		iY,		STATUS_NONE,		0,	2 },
	{ "TAX",	TAX,	REGISTER_OP,	Instr::Move,			iA,		iX,		STATUS_NONE,		0,	2 },
	{ "TSX",	TSX,	REGISTER_OP,	Instr::Move,			iSP,	iX,		STATUS_NONE,		0,	2 },
	{ "SEC",	SEC,	REGISTER_OP,	Instr::SetStatusBit,	iNil,	iNil,	STATUS_CARRY,		0,	2 },
	{ "SEI",	SEI,	REGISTER_OP,	Instr::SetStatusBit,	iNil,	iNil,	STATUS_INTERRUPT,	0,	2 },
	{ "SED",	SED,	REGISTER_OP,	Instr::SetStatusBit,	iNil,	iNil,	STATUS_DECIMAL,		0,	2 },
	{ "CLI",	CLI,	REGISTER_OP,	Instr::ClearStatusBit,	iNil,	iNil,	STATUS_INTERRUPT,	0,	2 },
	{ "CLC",	CLC,	REGISTER_OP,	Instr::ClearStatusBit,	iNil,	iNil,	STATUS_CARRY,		0,	2 },
	{ "CLV",	CLV,	REGISTER_OP,	Instr::ClearStatusBit,	iNil,	iNil,	STATUS_OVERFLOW,	0,	2 },
	{ "CLD",	CLD,	REGISTER_OP,	Instr::ClearStatusBit,	iNil,	iNil,	STATUS_DECIMAL,		0,	2 },
	{ "INX",	INX,	MATH_OP,		Instr::IncReg,			iX,		iNil,	STATUS_NONE,		0,	2 },
	{ "INY",	INY,	MATH_OP,		Instr::IncReg,			iY,		iNil,	STATUS_NONE,		0,	2 },
	{ "DEX",	DEX,	MATH_OP,		Instr::DecReg,			iX,		iNil,	STATUS_NONE,		0,	2 },
	{ "DEY",	DEY,	MATH_OP,		Instr::DecReg,			iY,		iNil,	STATUS_NONE,		0,	2 },
	{ "NOP",	Nop,	SYSTEM_OP,		Instr::NOP,				iNil,	iNil,	STATUS_NONE,		0,	2 },
	{ "BMI",	BMI,	BRANCH_OP,		Instr::BranchOnSet,		iNil,	iNil,	STATUS_NEGATIVE,	1,	2 },
	{ "BVS",	BVS,	BRANCH_OP,		Instr::BranchOnSet,		iNil,	iNil,	STATUS_OVERFLOW,	1,	2 },
	{ "BCS",	BCS,	BRANCH_OP,		Instr::BranchOnSet,		iNil,	iNil,	STATUS_CARRY,		1,	2 },
	{ "BEQ",	BEQ,	BRANCH_OP,		Instr::BranchOnSet,		iNil,	iNil,	STATUS_ZERO,		1,	2 },
	{ "BPL",	BPL,	BRANCH_OP,		Instr::BranchOnClear,	iNil,	iNil,	STATUS_NEGATIVE,	1,	2 },
	{ "BVC",	BVC,	BRANCH_OP,		Instr::BranchOnClear,	iNil,	iNil,	STATUS_OVERFLOW,	1,	2 },
	{ "BCC",	BCC,	BRANCH_OP,		Instr::BranchOnClear,	iNil,	iNil,	STATUS_CARRY,		1,	2 },
	{ "BNE",	BNE,	BRANCH_OP,		Instr::BranchOnClear,	iNil,	iNil,	STATUS_ZERO,		1,	2 },
	{ "CMP",	CMP,	ADDR_OP,		Instr::Compare,			iA,		iNil,	STATUS_NONE,		0,	0 },
	{ "CPY",	CPY,	ADDR_OP,		Instr::Compare,			iY,		iNil,	STATUS_NONE,		0,	0 },
	{ "CPX",	CPX,	ADDR_OP,		Instr::Compare,			iX,		iNil,	STATUS_NONE,		0,	0 },
	{ "STY",	STY,	ADDR_OP,		Instr::Store,			iY,		iNil,	STATUS_NONE,		0,	0 },
	{ "STA",	STA,	ADDR_OP,		Instr::Store,			iA,		iNil,	STATUS_NONE,		0,	0 },
	{ "STX",	STX,	ADDR_OP,		Instr::Store,			iX,		iNil,	STATUS_NONE,		0,	0 },
	{ "LDY",	LDY,	ADDR_OP,		Instr::Load,			iY,		iNil,	STATUS_NONE,		0,	0 },
	{ "LDA",	LDA,	ADDR_OP,		Instr::Load,			iA,		iNil,	STATUS_NONE,		0,	0 },
	{ "LDX",	LDX,	ADDR_OP,		Instr::Load,			iX,		iNil,	STATUS_NONE,		0,	0 },
	{ "ADC",	ADC,	ADDR_OP,		Instr::ADC,				iA,		iNil,	STATUS_NONE,		0,	0 },
	{ "DEC",	DEC,	ADDR_OP,		Instr::DEC,				iNil,	iNil,	STATUS_NONE,		0,	2 },
	{ "SBC",	SBC,	ADDR_OP,		Instr::SBC,				iA,		iNil,	STATUS_NONE,		0,	0 },
	{ "INC",	INC,	ADDR_OP,		Instr::INC,				iNil,	iNil,	STATUS_NONE,		0,	2 },
	{ "BIT",	BIT,	BITWISE_OP,		Instr::BIT,				iNil,	iNil,	STATUS_NONE,		0,	0 },
	{ "AND",	AND,	BITWISE_OP,		Instr::AND,				iNil,	iNil,	STATUS_NONE,		0,	0 },
	{ "ORA",	ORA,	BITWISE_OP,		Instr::ORA,				iNil,	iNil,	STATUS_NONE,		0,	0 },
	{ "EOR",	EOR,	BITWISE_OP,		Instr::EOR,				iNil,	iNil,	STATUS_NONE,		0,	0 },
	{ "ASL",	ASL,	BITWISE_OP,		Instr::ASL,				iNil,	iNil,	STATUS_NONE,		0,	2 },
	{ "LSR",	LSR,	BITWISE_OP,		Instr::LSR,				iNil,	iNil,	STATUS_NONE,		0,	2 },
	{ "ROL",	ROL,	BITWISE_OP,		Instr::ROL,				iNil,	iNil,	STATUS_NONE,		0,	2 },
	{ "ROR",	ROR,	BITWISE_OP,		Instr::ROR,				iNil,	iNil,	STATUS_NONE,		0,	2 },
};


const byte numAddrModes = 19;
AddrMapTuple addrMap[numAddrModes]
{
	{ ZPIX1,	"($%02x,X)",	"(ZeroPage,X)",		Instr::IndexedIndirect,	iX,		1,	2 },
	{ ZP1,		"$%02x",		"ZeroPage",			Instr::Zero,			iNil,	1,	3 },
	{ IMM1,		"#$%02x",		"Immediate",		Instr::Immediate,		iNil,	1,	2 },
	{ ABS1,		"$%04x",		"Absolute",			Instr::Absolute,		iNil,	2,	4 },
	{ ZPY1,		"($%02x),Y",	"(ZeroPage),Y",		Instr::IndirectIndexed,	iY,		1,	6 },
	{ ZPX1,		"$%02x,X",		"ZeroPage,X",		Instr::IndexedZero,		iX,		1,	4 },
	{ ABSY1,	"$%04x,Y",		"Absolute,Y",		Instr::IndexedAbsolute,	iY,		2,	4 },
	{ ABSX1,	"$%04x,X",		"Absolute,X",		Instr::IndexedAbsolute,	iX,		2,	4 },
	{ IMM2,		"#$%02x",		"Immediate",		Instr::Immediate,		iNil,	1,	2 },
	{ ZP2,		"$%02x",		"ZeroPage",			Instr::Zero,			iNil,	1,	3 },
	{ ACC2,		"",				"Accumulator",		Instr::Accumulator,		iNil,	0,	2 },
	{ ABS2,		"$%04x",		"Absolute",			Instr::Absolute,		iNil,	2,	4 },
	{ ZPX2,		"$%02x,X/Y",	"ZeroPage,X/Y",		Instr::IndexedZero,		iX,		1,	4 },
	{ ABSX2,	"$%04x,X/Y",	"Absolute,X/Y",		Instr::IndexedAbsolute,	iX,		2,	4 },
	{ IMM0,		"#$%02x",		"Immediate",		Instr::Immediate,		iNil,	1,	2 },
	{ ZP0,		"$%02x",		"ZeroPage",			Instr::Zero,			iNil,	1,	3 },
	{ ABS0,		"$%04x",		"Absolute",			Instr::Absolute,		iNil,	2,	4 },
	{ ZPX0,		"$%02x,X",		"ZeroPage,X",		Instr::IndexedZero,		iX,		1,	4 },
	{ ABSX0,	"$%04x",		"Absolute,X",		Instr::IndexedAbsolute,	iX,		2,	4 },

};

// These are explicitly excluded since the instruction map code will generate garbage for these ops
const byte illegalOpFormat = 0x03;
const byte numIllegalOps = 23;
byte illegalOps[numIllegalOps]
{
	0x02, 0x22, 0x34, 0x3C, 0x42, 0x44, 0x54, 0x5C,
	0x62, 0x64, 0x74, 0x7C, 0x80, 0x82, 0x89, 0x9C,
	0x9E, 0xC2, 0xD4, 0xDC, 0xE2, 0xF4, 0xFC,
};


InstructionMapTuple instructionMap[256];
DisassemblerMapTuple disassemblerMap[256];