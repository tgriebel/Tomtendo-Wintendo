#pragma once

#include <map>
#include <sstream>
#include "common.h"

typedef uint8_t StatusBit;

struct InstrParams;
struct Cpu6502;
struct OpCodeMap;
struct AddrMapTuple;
struct InstructionMapTuple;
struct DisassemblerMapTuple;
class NesSystem;

typedef uint8_t( Cpu6502::* Instruction_depr )( const InstrParams& params );
typedef uint8_t&( Cpu6502::* AddrFunction )( const InstrParams& params, uint32_t& outValue );

/*
#define OP_DECL(name)	template<class AddressFunc> \
						struct _##name : Instruction \
						{ \
							uint32_t operator()( const InstrParams& params, Cpu6502& cpu ) const; \
						};
*/

#define OP_DECL(name) uint8_t name##( const InstrParams& params );

/*
#define OP_DEF(name)	template<class AddressFunc> \
						uint32_t Cpu6502::_##name<AddressFunc>::operator()( const InstrParams& params, Cpu6502& cpu ) const
*/

#define OP_DEF(name) uint8_t Cpu6502::##name( const InstrParams& params )


/*
#define ADDR_MODE_DECL(name)	struct name##S \
								{ \
									inline uint8_t operator()( const InstrParams& params, uint32_t& address ) const; \
								};
*/

#define ADDR_MODE_DECL(name) uint8_t& name##( const InstrParams& params, uint32_t& outValue );

#define ADDR_MODE_DEF(name) uint8_t& Cpu6502::##name( const InstrParams& params, uint32_t& address )

#define OP_ADDR(name,address,ops,cycles) { #name, &Cpu6502::##name, nullptr, &Cpu6502::##address, ops, cycles },
//#define OP_ADDR(name,address,ops,cycles) { #name, name, &Cpu6502::##name, &name##i<Cpu6502::##address##S>, nullptr, ops, cycles },
#define OP(name,ops,cycles) { #name, &Cpu6502::##name, nullptr, nullptr, ops, cycles },
#define OP_ILLEGAL() { "???", &Cpu6502::Illegal, nullptr, nullptr, 0, 0 },

struct Instruction
{
	virtual uint32_t operator()( const InstrParams& params, Cpu6502& cpu ) const = 0;
};


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


struct InstrParams
{
	uint8_t			param0;
	uint8_t			param1;
	AddrFunction	getAddr;
	Cpu6502*		cpu;

	InstrParams() : cpu(NULL), getAddr( NULL ), param0( 0 ), param1( 0 ) {}
};


struct InstructionMapTuple
{
	char			mnemonic[16];
	Instruction_depr instr;
	const Instruction*const	addrFunctor;
	AddrFunction	addrFunc;
	uint8_t			operands;
	uint8_t			cycles;
};


struct Cpu6502
{
	static const uint32_t InvalidAddress = ~0x00;

	uint16_t nmiVector;
	uint16_t irqVector;
	uint16_t resetVector;

	NesSystem* system;

	cpuCycle_t cycle;
	cpuCycle_t instructionCycles;

#if DEBUG_ADDR == 1
	std::stringstream debugAddr;
	std::ofstream logFile;
	bool printToOutput;
#endif

	bool forceStop;
	uint16_t forceStopAddr;

	bool resetTriggered;
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

		cycle = cpuCycle_t(0);
	}

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

	bool Step( const cpuCycle_t nextCycle );

private:
	cpuCycle_t Exec();

	static bool CheckSign( const uint16_t checkValue );
	static bool CheckCarry( const uint16_t checkValue );
	static bool CheckZero( const uint16_t checkValue );
	static bool CheckOverflow( const uint16_t src, const uint16_t temp, const uint8_t finalValue );

	uint8_t NMI();
	uint8_t IRQ();

	uint8_t& IndexedAbsolute( const InstrParams& params, uint32_t& address, const uint8_t& reg );
	uint8_t& IndexedZero( const InstrParams& params, uint32_t& address, const uint8_t& reg );

	void Push( const uint8_t value );
	uint8_t Pull();
	void PushByte( const uint16_t value );
	uint16_t PullWord();

	uint8_t PullProgramByte();
	uint8_t PeekProgramByte() const;

	void SetAluFlags( const uint16_t value );

	uint16_t CombineIndirect( const uint8_t lsb, const uint8_t msb, const uint32_t wrap );

	uint8_t AddPageCrossCycles( const uint16_t address );
	uint8_t Branch( const InstrParams& params, const bool takeBranch );
	
	template< class AddressFunc >
	uint8_t& Read( const InstrParams& params )
	{
		uint32_t address = 0x00;

		uint8_t& M = AddressFunc()( params, address );

		return M;
	}
	uint8_t& Read( const InstrParams& params );
	void Write( const InstrParams& params, const uint8_t value );

	void DebugIndexZero( const uint8_t& reg, const uint32_t address, const uint32_t targetAddresss );
};


//template<class AddressFunc>
//static constexpr Cpu6502::_LDA<AddressFunc> LDAi = Cpu6502::_LDA<AddressFunc>();


const uint16_t NumInstructions = 256;
static const InstructionMapTuple InstructionMap[NumInstructions] =
{
	OP( BRK, 0, 7 )								//	00
	OP_ADDR( ORA, IndexedIndirect, 1, 6 )		//	01
	OP_ILLEGAL()								//	02
	OP_ILLEGAL()								//	03
	OP_ILLEGAL()								//	04
	OP_ADDR( ORA, Zero, 1, 3 )					//	05
	OP_ADDR( ASL, Zero, 1, 5 )					//	06
	OP_ILLEGAL()								//	07
	OP( PHP, 0, 3 )								//	08
	OP_ADDR( ORA, Immediate, 1, 2 )				//	09
	OP_ADDR( ASL, Accumulator, 0, 2 )			//	0A
	OP_ILLEGAL()								//	0B
	OP_ILLEGAL()								//	0C
	OP_ADDR( ORA, Absolute, 2, 4 )				//	0D
	OP_ADDR( ASL, Absolute, 2, 6 )				//	0E
	OP_ILLEGAL()								//	0F
	OP( BPL, 1, 2 )								//	10
	OP_ADDR( ORA, IndirectIndexed, 1, 5 )		//	11
	OP_ILLEGAL()								// 	12
	OP_ILLEGAL()								//	13
	OP_ILLEGAL()								//	14
	OP_ADDR( ORA, IndexedZeroX, 1, 4 )			//	15
	OP_ADDR( ASL, IndexedZeroX, 1, 6 )			//	16
	OP_ILLEGAL()								//	17
	OP( CLC, 0, 2 )								//	18
	OP_ADDR( ORA, IndexedAbsoluteY, 2, 4 )		//	19
	OP_ILLEGAL()								//	1A
	OP_ILLEGAL()								//	1B
	OP_ILLEGAL()								//	1C
	OP_ADDR( ORA, IndexedAbsoluteX, 2, 4 )		//	1D
	OP_ADDR( ASL, IndexedAbsoluteX, 2, 7 )		//	1E
	OP_ILLEGAL()								//	1F
	OP( JSR, 2, 6 )								//	20
	OP_ADDR( AND, IndexedIndirect, 1, 6 )		//	21
	OP_ILLEGAL()								//	22
	OP_ILLEGAL()								//	23
	OP_ADDR( BIT, Zero, 1, 3 )					//	24
	OP_ADDR( AND, Zero, 1, 3 )					//	25
	OP_ADDR( ROL, Zero, 1, 5 )					//	26
	OP_ILLEGAL()								//	27
	OP( PLP, 0, 4 )								//	28
	OP_ADDR( AND, Immediate, 1, 2 )				//	29
	OP_ADDR( ROL, Accumulator, 0, 2 )			//	2A
	OP_ILLEGAL()								//	2B
	OP_ADDR( BIT, Absolute, 2, 4 )				//	2C
	OP_ADDR( AND, Absolute, 2, 4 )				//	2D
	OP_ADDR( ROL, Absolute, 2, 6 )				//	2E
	OP_ILLEGAL()								//	2F
	OP( BMI, 1, 2 )								//	30
	OP_ADDR( AND, IndirectIndexed, 1, 5 )		//	31
	OP_ILLEGAL()								//	32
	OP_ILLEGAL()								//	33
	OP_ILLEGAL()								//	34
	OP_ADDR( AND, IndexedZeroX, 1, 4 )			//	35
	OP_ADDR( ROL, IndexedZeroX, 1, 6 )			//	36
	OP_ILLEGAL()								//	37
	OP( SEC, 0, 2 )								//	38
	OP_ADDR( AND, IndexedAbsoluteY, 2, 4 )		//	39
	OP_ILLEGAL()								//	3A
	OP_ILLEGAL()								//	3B
	OP_ILLEGAL()								//	3C
	OP_ADDR( AND, IndexedAbsoluteX, 2, 4 )		//	3D
	OP_ADDR( ROL, IndexedAbsoluteX, 2, 7 )		//	3E
	OP_ILLEGAL()								//	3F
	OP( RTI, 0, 6 )								//	40
	OP_ADDR( EOR, IndexedIndirect, 1, 6 )		//	41
	OP_ILLEGAL()								//	42
	OP_ILLEGAL()								//	43
	OP_ILLEGAL()								//	44
	OP_ADDR( EOR, Zero, 1, 3 )					//	45
	OP_ADDR( LSR, Zero, 1, 5 )					//	46
	OP_ILLEGAL()								//	47
	OP( PHA, 0, 3 )								//	48
	OP_ADDR( EOR, Immediate, 1, 2 )				//	49
	OP_ADDR( LSR, Accumulator, 0, 2 )			//	4A
	OP_ILLEGAL()								//	4B
	OP( JMP, 2, 3 )								//	4C
	OP_ADDR( EOR, Absolute, 2, 4 )				//	4D
	OP_ADDR( LSR, Absolute, 2, 6 )				//	4E
	OP_ILLEGAL()								//	4F	
	OP( BVC, 1, 2 )								//	50
	OP_ADDR( EOR, IndirectIndexed, 1, 5 )		//	51
	OP_ILLEGAL()								//	52
	OP_ILLEGAL()								//	53
	OP_ILLEGAL()								//	54	
	OP_ADDR( EOR, IndexedZeroX, 1, 4 )			//	55
	OP_ADDR( LSR, IndexedZeroX, 1, 6 )			//	56
	OP_ILLEGAL()								//	57
	OP( CLI, 0, 2 )								//	58
	OP_ADDR( EOR, IndexedAbsoluteY, 2, 4 )		//	59
	OP_ILLEGAL()								//	5A
	OP_ILLEGAL()								//	5B
	OP_ILLEGAL()								//	5C	
	OP_ADDR( EOR, IndexedAbsoluteX, 2, 4 )		//	5D
	OP_ADDR( LSR, IndexedAbsoluteX, 2, 7 )		//	5E
	OP_ILLEGAL()								//	5F
	OP( RTS, 0, 6 )								//	60
	OP_ADDR( ADC, IndexedIndirect, 1, 6 )		//	61
	OP_ILLEGAL()								//	62
	OP_ILLEGAL()								//	63
	OP_ILLEGAL()								//	64
	OP_ADDR( ADC, Zero, 1, 3 )					//	65
	OP_ADDR( ROR, Zero, 1, 5 )					//	66
	OP_ILLEGAL()								//	67
	OP( PLA, 0, 4 )								//	68
	OP_ADDR( ADC, Immediate, 1, 2 )				//	69
	OP_ADDR( ROR, Accumulator, 0, 2 )			//	6A
	OP_ILLEGAL()								//	6B	
	OP( JMPI, 2, 5 )							//	6C
	OP_ADDR( ADC, Absolute, 2, 4 )				//	6D
	OP_ADDR( ROR, Absolute, 2, 6 )				//	6E
	OP_ILLEGAL()								//	6F
	OP( BVS, 1, 2 )								//	70
	OP_ADDR( ADC, IndirectIndexed, 1, 5 )		//	71
	OP_ILLEGAL()								//	72
	OP_ILLEGAL()								//	73
	OP_ILLEGAL()								//	74
	OP_ADDR( ADC, IndexedZeroX, 1, 4 )			//	75
	OP_ADDR( ROR, IndexedZeroX, 1, 6 )			//	76
	OP_ILLEGAL()								//	77
	OP( SEI, 0, 2 )								//	78
	OP_ADDR( ADC, IndexedAbsoluteY, 2, 4 )		//	79
	OP_ILLEGAL()								//	7A
	OP_ILLEGAL()								//	7B
	OP_ILLEGAL()								//	7C
	OP_ADDR( ADC, IndexedAbsoluteX, 2, 4 )		//	7D
	OP_ADDR( ROR, IndexedAbsoluteX, 2, 7 )		//	7E
	OP_ILLEGAL()								//	7F
	OP_ILLEGAL()								//	80
	OP_ADDR( STA, IndexedIndirect, 1, 6 )		//	81
	OP_ILLEGAL()								//	82
	OP_ILLEGAL()								//	83	
	OP_ADDR( STY, Zero, 1, 3 )					//	84
	OP_ADDR( STA, Zero, 1, 3 )					//	85
	OP_ADDR( STX, Zero, 1, 3 )					//	86
	OP_ILLEGAL()								//	87
	OP( DEY, 0, 2 )								//	88
	OP_ILLEGAL()								//	89
	OP( TXA, 0, 2 )								//	8A
	OP_ILLEGAL()								//	8B
	OP_ADDR( STY, Absolute, 2, 4 )				//	8C
	OP_ADDR( STA, Absolute, 2, 4 )				//	8D
	OP_ADDR( STX, Absolute, 2, 4 )				//	8E
	OP_ILLEGAL()								//	8F
	OP( BCC, 1, 2 )								//	90
	OP_ADDR( STA, IndirectIndexed, 1, 6 )		//	91
	OP_ILLEGAL()								//	92
	OP_ILLEGAL()								//	93
	OP_ADDR( STY, IndexedZeroX, 1, 4 )			//	94
	OP_ADDR( STA, IndexedZeroX, 1, 4 )			//	95
	OP_ADDR( STX, IndexedZeroY, 1, 4 )			//	96
	OP_ILLEGAL()								//	97	
	OP( TYA, 0, 2 )								//	98
	OP_ADDR( STA, IndexedAbsoluteY, 2, 5 )		//	99
	OP( TXS, 0, 2 )								//	9A
	OP_ILLEGAL()								//	9B
	OP_ILLEGAL()								//	9C
	OP_ADDR( STA, IndexedAbsoluteX, 2, 5 )		//	9D
	OP_ILLEGAL()								//	9E
	OP_ILLEGAL()								//	9F
	OP_ADDR( LDY, Immediate, 1, 2 )				//	A0
	OP_ADDR( LDA, IndexedIndirect, 1, 6 )		//	A1
	OP_ADDR( LDX, Immediate, 1, 2 )				//	A2
	OP_ILLEGAL()								//	A3
	OP_ADDR( LDY, Zero, 1, 3 )					//	A4
	OP_ADDR( LDA, Zero, 1, 3 )					//	A5
	OP_ADDR( LDX, Zero, 1, 3 )					//	A6
	OP_ILLEGAL()								//	A7
	OP( TAY, 0, 2 )								//	A8
	OP_ADDR( LDA, Immediate, 1, 2 )				//	A9
	OP( TAX, 0, 2 )								//	AA
	OP_ILLEGAL()								//	AB
	OP_ADDR( LDY, Absolute, 2, 4 )				//	AC
	OP_ADDR( LDA, Absolute, 2, 4 )				//	AD
	OP_ADDR( LDX, Absolute, 2, 4 )				//	AE
	OP_ILLEGAL()								//	AF
	OP( BCS, 1, 2 )								//	B0
	OP_ADDR( LDA, IndirectIndexed, 1, 5 )		//	B1
	OP_ILLEGAL()								//	B2
	OP_ILLEGAL()								//	B3
	OP_ADDR( LDY, IndexedZeroX, 1, 4 )			//	B4
	OP_ADDR( LDA, IndexedZeroX, 1, 4 )			//	B5
	OP_ADDR( LDX, IndexedZeroY, 1, 4 )			//	B6
	OP_ILLEGAL()								//	B7
	OP( CLV, 0, 2 )								//	B8
	OP_ADDR( LDA, IndexedAbsoluteY, 2, 4 )		//	B9
	OP( TSX, 0, 2 )								//	BA
	OP_ILLEGAL()								//	BB
	OP_ADDR( LDY, IndexedAbsoluteX, 2, 4 )		//	BC
	OP_ADDR( LDA, IndexedAbsoluteX, 2, 4 )		//	BD
	OP_ADDR( LDX, IndexedAbsoluteY, 2, 4 )		//	BE
	OP_ILLEGAL()								//	BF
	OP_ADDR( CPY, Immediate, 1, 2 )				//	C0
	OP_ADDR( CMP, IndexedIndirect, 1, 6 )		//	C1
	OP_ILLEGAL()								//	C2
	OP_ILLEGAL()								//	C3	
	OP_ADDR( CPY, Zero, 1, 3 )					//	C4
	OP_ADDR( CMP, Zero, 1, 3 )					//	C5
	OP_ADDR( DEC, Zero, 1, 5 )					//	C6
	OP_ILLEGAL()								//	C7
	OP( INY, 0, 2 )								//	C8
	OP_ADDR( CMP, Immediate, 1, 2 )				//	C9
	OP( DEX, 0, 2 )								//	CA
	OP_ILLEGAL()								//	CB
	OP_ADDR( CPY, Absolute, 2, 4 )				//	CC
	OP_ADDR( CMP, Absolute, 2, 4 )				//	CD
	OP_ADDR( DEC, Absolute, 2, 6 )				//	CE
	OP_ILLEGAL()								//	CF
	OP( BNE, 1, 2 )								//	D0
	OP_ADDR( CMP, IndirectIndexed, 1, 5 )		//	D1
	OP_ILLEGAL()								//	D2
	OP_ILLEGAL()								//	D3
	OP_ILLEGAL()								//	D4
	OP_ADDR( CMP, IndexedZeroX, 1, 4 )			//	D5
	OP_ADDR( DEC, IndexedZeroX, 1, 6 )			//	D6
	OP_ILLEGAL()								//	D7
	OP( CLD, 0, 2 )								//	D8
	OP_ADDR( CMP, IndexedAbsoluteY, 2, 4 )		//	D9
	OP_ILLEGAL()								//	DA
	OP_ILLEGAL()								//	DB
	OP_ILLEGAL()								//	DC
	OP_ADDR( CMP, IndexedAbsoluteX, 2, 4 )		//	DD
	OP_ADDR( DEC, IndexedAbsoluteX, 2, 7 )		//	DE
	OP_ILLEGAL()								//	DF
	OP_ADDR( CPX, Immediate, 1, 2 )				//	E0
	OP_ADDR( SBC, IndexedIndirect, 1, 6 )		//	E1
	OP_ILLEGAL()								//	E2
	OP_ILLEGAL()								//	E3
	OP_ADDR( CPX, Zero, 1, 3 )					//	E4
	OP_ADDR( SBC, Zero, 1, 3 )					//	E5
	OP_ADDR( INC, Zero, 1, 3 )					//	E6
	OP_ILLEGAL()								//	E7
	OP( INX, 0, 2 )								//	E8
	OP_ADDR( SBC, Immediate, 1, 2 )				//	E9
	OP( NOP, 0, 2 )								//	EA
	OP_ILLEGAL()								//	EB
	OP_ADDR( CPX, Absolute, 2, 4 )				//	EC
	OP_ADDR( SBC, Absolute, 2, 4 )				//	ED
	OP_ADDR( INC, Absolute, 2, 6 )				//	EE
	OP_ILLEGAL()								//	EF
	OP( BEQ, 1, 2 )								//	F0
	OP_ADDR( SBC, IndirectIndexed, 1, 5 )		//	F1
	OP_ILLEGAL()								//	F2
	OP_ILLEGAL()								//	F3
	OP_ILLEGAL()								//	F4
	OP_ADDR( SBC, IndexedZeroX, 1, 4 )			//	F5
	OP_ADDR( INC, IndexedZeroX, 1, 6 )			//	F6
	OP_ILLEGAL()								//	F7
	OP( SED, 0, 2 )								//	F8
	OP_ADDR( SBC, IndexedAbsoluteY, 2, 4 )		//	F9
	OP_ILLEGAL()								//	FA
	OP_ILLEGAL()								//	FB
	OP_ILLEGAL()								//	FC
	OP_ADDR( SBC, IndexedAbsoluteX, 2, 4 )		//	FD
	OP_ADDR( INC, IndexedAbsoluteX, 2, 7 )		//	FE
	OP_ILLEGAL()								//	FF
};