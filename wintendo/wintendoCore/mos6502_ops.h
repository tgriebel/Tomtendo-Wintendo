//////////////////////////////////////////////////////////////////////////////////
//																				//
// WARNING: THIS FILE IS MEANT TO BE USED AS A PLAIN-TEXT PREPROCESSOR INCLUDE	//
//																				//
//////////////////////////////////////////////////////////////////////////////////

OP_DEF( SEC )
{
	P.bit.c = 1;
}


OP_DEF( SEI )
{
	P.bit.i = 1;
}


OP_DEF( SED )
{
	P.bit.d = 1;
}


OP_DEF( CLC )
{
	P.bit.c = 0;
}


OP_DEF( CLI )
{
	P.bit.i = 0;
}


OP_DEF( CLV )
{
	P.bit.v = 0;
}


OP_DEF( CLD )
{
	P.bit.d = 0;
}


OP_DEF( CMP )
{
	const uint16_t result = ( A - Read<AddrModeT>() );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );
}


OP_DEF( CPX )
{
	const uint16_t result = ( X - Read<AddrModeT>() );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );
}


OP_DEF( CPY )
{
	const uint16_t result = ( Y - Read<AddrModeT>() );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );
}


OP_DEF( LDA )
{
	A = Read<AddrModeT>();
	SetAluFlags( A );
}


OP_DEF( LDX )
{
	X = Read<AddrModeT>();
	SetAluFlags( X );
}


OP_DEF( LDY )
{
	Y = Read<AddrModeT>();
	SetAluFlags( Y );
}


OP_DEF( STA )
{
	Write<AddrModeT>( A );
}


OP_DEF( STX )
{
	Write<AddrModeT>( X );
}


OP_DEF( STY )
{
	Write<AddrModeT>( Y );
}


OP_DEF( TXS )
{
	SP = X;
}


OP_DEF( TXA )
{
	A = X;
	SetAluFlags( A );
}


OP_DEF( TYA )
{
	A = Y;
	SetAluFlags( A );
}


OP_DEF( TAX )
{
	X = A;
	SetAluFlags( X );
}


OP_DEF( TAY )
{
	Y = A;
	SetAluFlags( Y );
}


OP_DEF( TSX )
{
	X = SP;
	SetAluFlags( X );
}


OP_DEF( ADC )
{
	// http://nesdev.com/6502.txt, "INSTRUCTION OPERATION - ADC"
	const uint8_t M = Read<AddrModeT>();
	const uint16_t src = A;
	const uint16_t carry = ( P.bit.c ) ? 1 : 0;
	const uint16_t temp = A + M + carry;

	A = ( temp & 0xFF );

	P.bit.z = CheckZero( temp );
	P.bit.v = CheckOverflow( M, temp, A );
	SetAluFlags( A );

	P.bit.c = ( temp > 0xFF );
}


OP_DEF( SBC )
{
	uint8_t M = Read<AddrModeT>();
	const uint16_t carry = ( P.bit.c ) ? 0 : 1;
	const uint16_t result = A - M - carry;

	SetAluFlags( result );

	P.bit.v = ( CheckSign( A ^ result ) && CheckSign( A ^ M ) );
	P.bit.c = !CheckCarry( result );

	A = result & 0xFF;
}


OP_DEF( INX )
{
	++X;
	SetAluFlags( X );
}

OP_DEF( INY )
{
	++Y;
	SetAluFlags( Y );
}


OP_DEF( DEX )
{
	--X;
	SetAluFlags( X );
}


OP_DEF( DEY )
{
	--Y;
	SetAluFlags( Y );
}


OP_DEF( INC )
{
	const uint8_t result = Read<AddrModeT>() + 1;

	Write<AddrModeT>( result );
	SetAluFlags( result );
}


OP_DEF( DEC )
{
	const uint8_t result = Read<AddrModeT>() - 1;

	Write<AddrModeT>( result );
	SetAluFlags( result );
}


OP_DEF( PHP )
{
	Push( P.byte | STATUS_UNUSED | STATUS_BREAK );
}


OP_DEF( PHA )
{
	Push( A );
}


OP_DEF( PLA )
{
	A = Pull();
	SetAluFlags( A );
}


OP_DEF( PLP )
{
	// https://wiki.nesdev.com/w/index.php/Status_flags
	const uint8_t status = ~STATUS_BREAK & Pull();
	P.byte = status | ( P.byte & STATUS_BREAK ) | STATUS_UNUSED;
}


OP_DEF( NOP )
{

}


OP_DEF( ASL )
{
	uint8_t M = Read<AddrModeT>();

	P.bit.c = !!( M & 0x80 );
	M <<= 1;
	Write<AddrModeT>( M );
	SetAluFlags( M );
}


OP_DEF( LSR )
{
	uint8_t M = Read<AddrModeT>();

	P.bit.c = ( M & 0x01 );
	M >>= 1;
	Write<AddrModeT>( M );
	SetAluFlags( M );
}


OP_DEF( AND )
{
	A &= Read<AddrModeT>();
	SetAluFlags( A );
}

OP_DEF( BIT )
{
	const uint8_t M = Read<AddrModeT>();

	P.bit.z = !( A & M );
	P.bit.n = CheckSign( M );
	P.bit.v = !!( M & 0x40 );
}


OP_DEF( EOR )
{
	A ^= Read<AddrModeT>();
	SetAluFlags( A );
}


OP_DEF( ORA )
{
	A |= Read<AddrModeT>();
	SetAluFlags( A );
}


OP_DEF( JMP )
{
	PC = ReadAddressOperand();

	DEBUG_ADDR_JMP
}


OP_DEF( JMPI )
{
	const uint16_t addr = ReadAddressOperand();
	PC = JumpImmediateAddr( addr );

	DEBUG_ADDR_JMPI
}


OP_DEF( JSR )
{
	uint16_t retAddr = PC + 1;

	Push( ( retAddr >> 8 ) & 0xFF );
	Push( retAddr & 0xFF );

	PC = ReadAddressOperand();

	DEBUG_ADDR_JSR
}


OP_DEF( BRK )
{
	interruptRequestNMI = true;
}


OP_DEF( RTS )
{
	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	PC = 1 + Combine( loByte, hiByte );
}


OP_DEF( RTI )
{
	PLP<AddrModeT>();

	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	PC = Combine( loByte, hiByte );
}


OP_DEF( BMI )
{
	Branch( P.bit.n );
}


OP_DEF( BVS )
{
	Branch( P.bit.v );
}


OP_DEF( BCS )
{
	Branch( P.bit.c );
}


OP_DEF( BEQ )
{
	Branch( P.bit.z );
}


OP_DEF( BPL )
{
	Branch( !P.bit.n );
}


OP_DEF( BVC )
{
	Branch( !P.bit.v );
}


OP_DEF( BCC )
{
	Branch( !P.bit.c );
}


OP_DEF( BNE )
{
	Branch( !P.bit.z );
}


OP_DEF( ROL )
{
	uint16_t temp = Read<AddrModeT>() << 1;
	temp = ( P.bit.c ) ? temp | 0x0001 : temp;

	P.bit.c = CheckCarry( temp );

	temp &= 0xFF;

	SetAluFlags( temp );

	Write<AddrModeT>( temp & 0xFF );
}


OP_DEF( ROR )
{
	uint16_t temp = ( P.bit.c ) ? Read<AddrModeT>() | 0x0100 : Read<AddrModeT>();

	P.bit.c = ( temp & 0x01 );

	temp >>= 1;

	SetAluFlags( temp );

	Write<AddrModeT>( temp & 0xFF );
}


OP_DEF( Illegal )
{
	assert( 0 );
}


OP_DEF( SKB )
{
	PC += 1;
}


OP_DEF( SKW )
{
	PC += 2;
}


inline void BuildOpLUT()
{
	OP_ADDR( 0x00,	BRK,		None,				0, 7 )
	OP_ADDR( 0x01,	ORA,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0x02,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x03,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x04,	SKB,		None,				0, 3 )
	OP_ADDR( 0x05,	ORA,		Zero,				1, 3 )
	OP_ADDR( 0x06,	ASL,		Zero,				1, 5 )
	OP_ADDR( 0x07,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x08,	PHP,		None,				0, 3 )
	OP_ADDR( 0x09,	ORA,		Immediate,			1, 2 )
	OP_ADDR( 0x0A,	ASL,		Accumulator,		0, 2 )
	OP_ADDR( 0x0B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x0C,	SKW,		None,				0, 4 )
	OP_ADDR( 0x0D,	ORA,		Absolute,			2, 4 )
	OP_ADDR( 0x0E,	ASL,		Absolute,			2, 6 )
	OP_ADDR( 0x0F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x10,	BPL,							1, 2 )
	OP_ADDR( 0x11,	ORA,		IndirectIndexed,	1, 5 )
	OP_ADDR( 0x12,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x13,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x14,	SKB,		None,				0, 4 )
	OP_ADDR( 0x15,	ORA,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0x16,	ASL,		IndexedZeroX,		1, 6 )
	OP_ADDR( 0x17,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x18,	CLC,		None,				0, 2 )
	OP_ADDR( 0x19,	ORA,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0x1A,	NOP,		None,				0, 2 )
	OP_ADDR( 0x1B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x1C,	SKW,		None,				0, 4 )
	OP_ADDR( 0x1D,	ORA,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0x1E,	ASL,		IndexedAbsoluteX,	2, 7 )
	OP_ADDR( 0x1F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x20,	JSR,							2, 6 )
	OP_ADDR( 0x21,	AND,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0x22,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x23,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x24,	BIT,		Zero,				1, 3 )
	OP_ADDR( 0x25,	AND,		Zero,				1, 3 )
	OP_ADDR( 0x26,	ROL,		Zero,				1, 5 )
	OP_ADDR( 0x27,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x28,	PLP,		None,				0, 4 )
	OP_ADDR( 0x29,	AND,		Immediate,			1, 2 )
	OP_ADDR( 0x2A,	ROL,		Accumulator,		0, 2 )
	OP_ADDR( 0x2B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x2C,	BIT,		Absolute,			2, 4 )
	OP_ADDR( 0x2D,	AND,		Absolute,			2, 4 )
	OP_ADDR( 0x2E,	ROL,		Absolute,			2, 6 )
	OP_ADDR( 0x2F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x30,	BMI,							1, 2 )
	OP_ADDR( 0x31,	AND,		IndirectIndexed,	1, 5 )
	OP_ADDR( 0x32,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x33,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x34,	SKB,		None,				0, 4 )
	OP_ADDR( 0x35,	AND,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0x36,	ROL,		IndexedZeroX,		1, 6 )
	OP_ADDR( 0x37,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x38,	SEC,		None,				0, 2 )
	OP_ADDR( 0x39,	AND,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0x3A,	NOP,		None,				0, 2 )
	OP_ADDR( 0x3B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x3C,	SKW,		None,				0, 4 )
	OP_ADDR( 0x3D,	AND,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0x3E,	ROL,		IndexedAbsoluteX,	2, 7 )
	OP_ADDR( 0x3F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x40,	RTI,							0, 6 )
	OP_ADDR( 0x41,	EOR,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0x42,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x43,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x44,	SKB,		None,				0, 3 )
	OP_ADDR( 0x45,	EOR,		Zero,				1, 3 )
	OP_ADDR( 0x46,	LSR,		Zero,				1, 5 )
	OP_ADDR( 0x47,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x48,	PHA,		None,				0, 3 )
	OP_ADDR( 0x49,	EOR,		Immediate,			1, 2 )
	OP_ADDR( 0x4A,	LSR,		Accumulator,		0, 2 )
	OP_ADDR( 0x4B,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x4C,	JMP,							2, 3 )
	OP_ADDR( 0x4D,	EOR,		Absolute,			2, 4 )
	OP_ADDR( 0x4E,	LSR,		Absolute,			2, 6 )
	OP_ADDR( 0x4F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x50,	BVC,							1, 2 )
	OP_ADDR( 0x51,	EOR,		IndirectIndexed,	1, 5 )
	OP_ADDR( 0x52,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x53,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x54,	SKB,		None,				0, 4 )
	OP_ADDR( 0x55,	EOR,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0x56,	LSR,		IndexedZeroX,		1, 6 )
	OP_ADDR( 0x57,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x58,	CLI,		None,				0, 2 )
	OP_ADDR( 0x59,	EOR,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0x5A,	NOP,		None,				0, 2 )
	OP_ADDR( 0x5B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x5C,	SKW,		None,				0, 4 )
	OP_ADDR( 0x5D,	EOR,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0x5E,	LSR,		IndexedAbsoluteX,	2, 7 )
	OP_ADDR( 0x5F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x60,	RTS,							0, 6 )
	OP_ADDR( 0x61,	ADC,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0x62,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x63,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x64,	SKB,		None,				0, 3 )
	OP_ADDR( 0x65,	ADC,		Zero,				1, 3 )
	OP_ADDR( 0x66,	ROR,		Zero,				1, 5 )
	OP_ADDR( 0x67,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x68,	PLA,		None,				0, 4 )
	OP_ADDR( 0x69,	ADC,		Immediate,			1, 2 )
	OP_ADDR( 0x6A,	ROR,		Accumulator,		0, 2 )
	OP_ADDR( 0x6B,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x6C,	JMPI,							2, 5 )
	OP_ADDR( 0x6D,	ADC,		Absolute,			2, 4 )
	OP_ADDR( 0x6E,	ROR,		Absolute,			2, 6 )
	OP_ADDR( 0x6F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x70,	BVS,							1, 2 )
	OP_ADDR( 0x71,	ADC,		IndirectIndexed,	1, 5 )
	OP_ADDR( 0x72,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x73,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x74,	SKB,		None,				0, 4 )
	OP_ADDR( 0x75,	ADC,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0x76,	ROR,		IndexedZeroX,		1, 6 )
	OP_ADDR( 0x77,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x78,	SEI,		None,				0, 2 )
	OP_ADDR( 0x79,	ADC,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0x7A,	NOP,		None,				0, 2 )
	OP_ADDR( 0x7B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x7C,	SKW,		None,				0, 4 )
	OP_ADDR( 0x7D,	ADC,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0x7E,	ROR,		IndexedAbsoluteX,	2, 7 )
	OP_ADDR( 0x7F,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x80,	SKB,		None,				0, 2 )
	OP_ADDR( 0x81,	STA,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0x82,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x83,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x84,	STY,		Zero,				1, 3 )
	OP_ADDR( 0x85,	STA,		Zero,				1, 3 )
	OP_ADDR( 0x86,	STX,		Zero,				1, 3 )
	OP_ADDR( 0x87,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x88,	DEY,		None,				0, 2 )
	OP_ADDR( 0x89,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x8A,	TXA,		None,				0, 2 )
	OP_ADDR( 0x8B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x8C,	STY,		Absolute,			2, 4 )
	OP_ADDR( 0x8D,	STA,		Absolute,			2, 4 )
	OP_ADDR( 0x8E,	STX,		Absolute,			2, 4 )
	OP_ADDR( 0x8F,	Illegal,	None,				0, 0 )
	OP_JUMP( 0x90,	BCC,							1, 2 )
	OP_ADDR( 0x91,	STA,		IndirectIndexed,	1, 6 )
	OP_ADDR( 0x92,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x93,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x94,	STY,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0x95,	STA,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0x96,	STX,		IndexedZeroY,		1, 4 )
	OP_ADDR( 0x97,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x98,	TYA,		None,				0, 2 )
	OP_ADDR( 0x99,	STA,		IndexedAbsoluteY,	2, 5 )
	OP_ADDR( 0x9A,	TXS,		None,				0, 2 )
	OP_ADDR( 0x9B,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x9C,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x9D,	STA,		IndexedAbsoluteX,	2, 5 )
	OP_ADDR( 0x9E,	Illegal,	None,				0, 0 )
	OP_ADDR( 0x9F,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xA0,	LDY,		Immediate,			1, 2 )
	OP_ADDR( 0xA1,	LDA,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0xA2,	LDX,		Immediate,			1, 2 )
	OP_ADDR( 0xA3,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xA4,	LDY,		Zero,				1, 3 )
	OP_ADDR( 0xA5,	LDA,		Zero,				1, 3 )
	OP_ADDR( 0xA6,	LDX,		Zero,				1, 3 )
	OP_ADDR( 0xA7,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xA8,	TAY,		None,				0, 2 )
	OP_ADDR( 0xA9,	LDA,		Immediate,			1, 2 )
	OP_ADDR( 0xAA,	TAX,		None,				0, 2 )
	OP_ADDR( 0xAB,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xAC,	LDY,		Absolute,			2, 4 )
	OP_ADDR( 0xAD,	LDA,		Absolute,			2, 4 )
	OP_ADDR( 0xAE,	LDX,		Absolute,			2, 4 )
	OP_ADDR( 0xAF,	Illegal,	None,				0, 0 )
	OP_JUMP( 0xB0,	BCS,							1, 2 )
	OP_ADDR( 0xB1,	LDA,		IndirectIndexed,	1, 5 )
	OP_ADDR( 0xB2,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xB3,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xB4,	LDY,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0xB5,	LDA,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0xB6,	LDX,		IndexedZeroY,		1, 4 )
	OP_ADDR( 0xB7,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xB8,	CLV,		None,				0, 2 )
	OP_ADDR( 0xB9,	LDA,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0xBA,	TSX,		None,				0, 2 )
	OP_ADDR( 0xBB,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xBC,	LDY,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0xBD,	LDA,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0xBE,	LDX,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0xBF,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xC0,	CPY,		Immediate,			1, 2 )
	OP_ADDR( 0xC1,	CMP,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0xC2,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xC3,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xC4,	CPY,		Zero,				1, 3 )
	OP_ADDR( 0xC5,	CMP,		Zero,				1, 3 )
	OP_ADDR( 0xC6,	DEC,		Zero,				1, 5 )
	OP_ADDR( 0xC7,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xC8,	INY,		None,				0, 2 )
	OP_ADDR( 0xC9,	CMP,		Immediate,			1, 2 )
	OP_ADDR( 0xCA,	DEX,		None,				0, 2 )
	OP_ADDR( 0xCB,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xCC,	CPY,		Absolute,			2, 4 )
	OP_ADDR( 0xCD,	CMP,		Absolute,			2, 4 )
	OP_ADDR( 0xCE,	DEC,		Absolute,			2, 6 )
	OP_ADDR( 0xCF,	Illegal,	None,				0, 0 )
	OP_JUMP( 0xD0,	BNE,							1, 2 )
	OP_ADDR( 0xD1,	CMP,		IndirectIndexed,	1, 5 )
	OP_ADDR( 0xD2,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xD3,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xD4,	SKB,		None,				0, 4 )
	OP_ADDR( 0xD5,	CMP,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0xD6,	DEC,		IndexedZeroX,		1, 6 )
	OP_ADDR( 0xD7,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xD8,	CLD,		None,				0, 2 )
	OP_ADDR( 0xD9,	CMP,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0xDA,	NOP,		None,				0, 2 )
	OP_ADDR( 0xDB,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xDC,	SKW,		None,				0, 4 )
	OP_ADDR( 0xDD,	CMP,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0xDE,	DEC,		IndexedAbsoluteX,	2, 7 )
	OP_ADDR( 0xDF,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xE0,	CPX,		Immediate,			1, 2 )
	OP_ADDR( 0xE1,	SBC,		IndexedIndirect,	1, 6 )
	OP_ADDR( 0xE2,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xE3,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xE4,	CPX,		Zero,				1, 3 )
	OP_ADDR( 0xE5,	SBC,		Zero,				1, 3 )
	OP_ADDR( 0xE6,	INC,		Zero,				1, 5 )
	OP_ADDR( 0xE7,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xE8,	INX,		None,				0, 2 )
	OP_ADDR( 0xE9,	SBC,		Immediate,			1, 2 )
	OP_ADDR( 0xEA,	NOP,		None,				0, 2 )
	OP_ADDR( 0xEB,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xEC,	CPX,		Absolute,			2, 4 )
	OP_ADDR( 0xED,	SBC,		Absolute,			2, 4 )
	OP_ADDR( 0xEE,	INC,		Absolute,			2, 6 )
	OP_ADDR( 0xEF,	Illegal,	None,				0, 0 )
	OP_JUMP( 0xF0,	BEQ,							1, 2 )
	OP_ADDR( 0xF1,	SBC,		IndirectIndexed,	1, 5 )
	OP_ADDR( 0xF2,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xF3,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xF4,	SKB,		None,				0, 4 )
	OP_ADDR( 0xF5,	SBC,		IndexedZeroX,		1, 4 )
	OP_ADDR( 0xF6,	INC,		IndexedZeroX,		1, 6 )
	OP_ADDR( 0xF7,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xF8,	SED,		None,				0, 2 )
	OP_ADDR( 0xF9,	SBC,		IndexedAbsoluteY,	2, 4 )
	OP_ADDR( 0xFA,	NOP,		None,				0, 2 )
	OP_ADDR( 0xFB,	Illegal,	None,				0, 0 )
	OP_ADDR( 0xFC,	SKW,		None,				0, 4 )
	OP_ADDR( 0xFD,	SBC,		IndexedAbsoluteX,	2, 4 )
	OP_ADDR( 0xFE,	INC,		IndexedAbsoluteX,	2, 7 )
	OP_ADDR( 0xFF,	Illegal,	None,				0, 0 )
}