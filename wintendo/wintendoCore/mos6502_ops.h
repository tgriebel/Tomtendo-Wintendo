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
	const uint16_t result = ( A - Read<AddrModeT>( o ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );
}

OP_DEF( CPX )
{
	const uint16_t result = ( X - Read<AddrModeT>( o ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );
}

OP_DEF( CPY )
{
	const uint16_t result = ( Y - Read<AddrModeT>( o ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );
}

OP_DEF( LDA )
{
	A = Read<AddrModeT>( o );
	SetAluFlags( A );
}

OP_DEF( LDX )
{
	X = Read<AddrModeT>( o );
	SetAluFlags( X );
}

OP_DEF( LDY )
{
	Y = Read<AddrModeT>( o );
	SetAluFlags( Y );
}

OP_DEF( STA )
{
	Write<AddrModeT>( o, A );
}

OP_DEF( STX )
{
	Write<AddrModeT>( o, X );
}

OP_DEF( STY )
{
	Write<AddrModeT>( o, Y );
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
	const uint8_t M = Read<AddrModeT>( o );
	const uint16_t C = ( P.bit.c ) ? 1 : 0;
	const uint16_t result = A + M + C;

	P.bit.v = !CheckSign( A ^ M ) && CheckSign( A ^ result );
	P.bit.z = CheckZero( result & 0xFF );
	P.bit.n = CheckSign( result );
	P.bit.c = CheckCarry( result );

	A = ( result & 0xFF );
}

OP_DEF( SBC )
{
	const uint8_t M = Read<AddrModeT>( o );
	const uint16_t C = ( P.bit.c ) ? 0 : 1;
	const uint16_t result = A - M - C;

	P.bit.v = CheckSign( A ^ result ) && CheckSign( A ^ M );
	P.bit.z = CheckZero( result & 0xFF );
	P.bit.n = CheckSign( result );
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
	const uint8_t result = Read<AddrModeT>( o ) + 1;

	Write<AddrModeT>( o, result );
	SetAluFlags( result );
}

OP_DEF( DEC )
{
	const uint8_t result = Read<AddrModeT>( o ) - 1;

	Write<AddrModeT>( o, result );
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
	uint8_t M = Read<AddrModeT>( o );

	P.bit.c = !!( M & 0x80 );
	M <<= 1;
	Write<AddrModeT>( o, M );
	SetAluFlags( M );
}

OP_DEF( LSR )
{
	uint8_t M = Read<AddrModeT>( o );

	P.bit.c = ( M & 0x01 );
	M >>= 1;
	Write<AddrModeT>( o, M );
	SetAluFlags( M );
}

OP_DEF( AND )
{
	A &= Read<AddrModeT>( o );
	SetAluFlags( A );
}

OP_DEF( BIT )
{
	const uint8_t M = Read<AddrModeT>( o );

	P.bit.z = !( A & M );
	P.bit.n = CheckSign( M );
	P.bit.v = !!( M & 0x40 );
}

OP_DEF( EOR )
{
	A ^= Read<AddrModeT>( o );
	SetAluFlags( A );
}

OP_DEF( ORA )
{
	A |= Read<AddrModeT>( o );
	SetAluFlags( A );
}

OP_DEF( JMP )
{
	PC = ReadAddressOperand( o );
	DEBUG_ADDR_JMP( PC )
}

OP_DEF( JMPI )
{
	const uint16_t addr = ReadAddressOperand( o );
	PC = JumpImmediateAddr( addr );
	DEBUG_ADDR_JMPI( addr, PC )
}

OP_DEF( JSR )
{
	uint16_t retAddr = PC + 1;

	Push( ( retAddr >> 8 ) & 0xFF );
	Push( retAddr & 0xFF );

	PC = ReadAddressOperand( o );

	DEBUG_ADDR_JSR( PC )
}

OP_DEF( BRK )
{
	irqAddr = irqVector;
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
	PLP<AddrModeT>( o );

	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	PC = Combine( loByte, hiByte );
}

OP_DEF( BMI )
{
	Branch( o, P.bit.n );
}

OP_DEF( BVS )
{
	Branch( o, P.bit.v );
}

OP_DEF( BCS )
{
	Branch( o, P.bit.c );
}

OP_DEF( BEQ )
{
	Branch( o, P.bit.z );
}

OP_DEF( BPL )
{
	Branch( o, !P.bit.n );
}

OP_DEF( BVC )
{
	Branch( o, !P.bit.v );
}

OP_DEF( BCC )
{
	Branch( o, !P.bit.c );
}

OP_DEF( BNE )
{
	Branch( o, !P.bit.z );
}

OP_DEF( ROL )
{
	uint16_t temp = Read<AddrModeT>( o ) << 1;
	temp = ( P.bit.c ) ? temp | 0x0001 : temp;

	P.bit.c = CheckCarry( temp );

	temp &= 0xFF;

	SetAluFlags( temp );

	Write<AddrModeT>( o, temp & 0xFF );
}

OP_DEF( ROR )
{
	uint16_t temp = ( P.bit.c ) ? Read<AddrModeT>( o ) | 0x0100 : Read<AddrModeT>( o );

	P.bit.c = ( temp & 0x01 );
	temp >>= 1;
	SetAluFlags( temp );

	Write<AddrModeT>( o, temp & 0xFF );
}

//////////////////////////////////////////////////////////////////////////////////
//																				//
//                                 ILLEGAL OPS	     		                    //
//																				//
//////////////////////////////////////////////////////////////////////////////////
OP_DEF( Illegal )
{
	//	assert( 0 );
	//	halt = true;
}

OP_DEF( SKB )
{
	Read<AddrModeT>( o );
}

OP_DEF( SKW )
{
	Read<AddrModeT>( o );
}

OP_DEF( LAX )
{
	const uint8_t value = Read<AddrModeT>( o );
	A = value;
	X = value;
	SetAluFlags( value );
}

OP_DEF( SAX )
{
	const uint8_t value = ( A & X );
	Write<AddrModeT>( o, value );
}

OP_DEF( DCP )
{
	const uint8_t dec = ( Read<AddrModeT>( o ) - 1 );
	Write<AddrModeT>( o, dec );

	const uint16_t cmp = ( A - dec );
	P.bit.c = !CheckCarry( cmp );
	SetAluFlags( cmp );
}

OP_DEF( ISB )
{
	const uint8_t inc = ( Read<AddrModeT>( o ) + 1 );
	Write<AddrModeT>( o, inc );

	const uint16_t carry = ( P.bit.c ) ? 0 : 1;
	const uint16_t result = A - inc - carry;

	SetAluFlags( result );

	P.bit.v = ( CheckSign( A ^ result ) && CheckSign( A ^ inc ) );
	P.bit.c = !CheckCarry( result );

	A = result & 0xFF;
}

OP_DEF( SLO )
{
	uint8_t M = Read<AddrModeT>( o );

	P.bit.c = !!( M & 0x80 );
	M <<= 1;
	Write<AddrModeT>( o, M );
	A |= M;
	SetAluFlags( A );
}

OP_DEF( RLA )
{
	uint16_t rol = Read<AddrModeT>( o ) << 1;
	rol = ( P.bit.c ) ? rol | 0x0001 : rol;

	P.bit.c = CheckCarry( rol );
	rol &= 0xFF;
	rol = rol & 0xFF;
	Write<AddrModeT>( o, static_cast<uint8_t>( rol ) );

	A &= rol;
	SetAluFlags( A );
}

OP_DEF( SRE )
{
	uint8_t M = Read<AddrModeT>( o );
	P.bit.c = ( M & 0x01 );
	M >>= 1;
	Write<AddrModeT>( o, M );

	A ^= M;
	SetAluFlags( A );
}

OP_DEF( RRA )
{
	uint16_t ror = ( P.bit.c ) ? Read<AddrModeT>( o ) | 0x0100 : Read<AddrModeT>( o );
	P.bit.c = ( ror & 0x01 );
	ror >>= 1;
	ror = ror & 0xFF;
	Write<AddrModeT>( o, static_cast<uint8_t>( ror ) );

	const uint16_t src = A;
	const uint16_t carry = ( P.bit.c ) ? 1 : 0;
	const uint16_t adc = A + ror + carry;

	A = ( adc & 0xFF );

	P.bit.z = CheckZero( adc );
	P.bit.v = CheckOverflow( ror, adc, A );
	SetAluFlags( A );

	P.bit.c = ( adc > 0xFF );
}

inline void BuildOpLUT()
{
	OP_ADDR( 0x00,	BRK,		None,				1, 7, 0 )
	OP_ADDR( 0x01,	ORA,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0x02,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x03,	SLO,		IndexedIndirect,	1, 8, 0 )
	ILLEGAL( 0x04,	SKB,		Zero,				1, 3, 0 )
	OP_ADDR( 0x05,	ORA,		Zero,				1, 3, 0 )
	OP_ADDR( 0x06,	ASL,		Zero,				1, 5, 0 )
	ILLEGAL( 0x07,	SLO,		Zero,				1, 5, 0 )
	OP_ADDR( 0x08,	PHP,		None,				0, 3, 0 )
	OP_ADDR( 0x09,	ORA,		Immediate,			1, 2, 0 )
	OP_ADDR( 0x0A,	ASL,		Accumulator,		0, 2, 0 )
	OP_ADDR( 0x0B,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x0C,	SKW,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x0D,	ORA,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x0E,	ASL,		Absolute,			2, 6, 0 )
	ILLEGAL( 0x0F,	SLO,		Absolute,			2, 6, 0 )
	OP_JUMP( 0x10,	BPL,		Branch,				1, 2, 1 )
	OP_ADDR( 0x11,	ORA,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0x12,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x13,	SLO,		IndirectIndexed,	1, 8, 0 )
	ILLEGAL( 0x14,	SKB,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x15,	ORA,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x16,	ASL,		IndexedZeroX,		1, 6, 0 )
	ILLEGAL( 0x17,	SLO,		IndexedZeroX,		1, 6, 0 )
	OP_ADDR( 0x18,	CLC,		None,				0, 2, 0 )
	OP_ADDR( 0x19,	ORA,		IndexedAbsoluteY,	2, 4, 1 )
	ILLEGAL( 0x1A,	NOP,		None,				0, 2, 0 )
	ILLEGAL( 0x1B,	SLO,		IndexedAbsoluteY,	2, 7, 0 )
	ILLEGAL( 0x1C,	SKW,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x1D,	ORA,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x1E,	ASL,		IndexedAbsoluteX,	2, 7, 1 )
	ILLEGAL( 0x1F,	SLO,		IndexedAbsoluteX,	2, 7, 0 )
	OP_JUMP( 0x20,	JSR,		Jsr,				2, 6, 0 )
	OP_ADDR( 0x21,	AND,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0x22,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x23,	RLA,		IndexedIndirect,	1, 8, 0 )
	OP_ADDR( 0x24,	BIT,		Zero,				1, 3, 0 )
	OP_ADDR( 0x25,	AND,		Zero,				1, 3, 0 )
	OP_ADDR( 0x26,	ROL,		Zero,				1, 5, 0 )
	ILLEGAL( 0x27,	RLA,		Zero,				1, 5, 0 )
	OP_ADDR( 0x28,	PLP,		None,				0, 4, 0 )
	OP_ADDR( 0x29,	AND,		Immediate,			1, 2, 0 )
	OP_ADDR( 0x2A,	ROL,		Accumulator,		0, 2, 0 )
	OP_ADDR( 0x2B,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x2C,	BIT,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x2D,	AND,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x2E,	ROL,		Absolute,			2, 6, 0 )
	ILLEGAL( 0x2F,	RLA,		Absolute,			2, 6, 0 )
	OP_JUMP( 0x30,	BMI,		Branch,				1, 2, 1 )
	OP_ADDR( 0x31,	AND,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0x32,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x33,	RLA,		IndirectIndexed,	1, 8, 0 )
	ILLEGAL( 0x34,	SKB,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x35,	AND,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x36,	ROL,		IndexedZeroX,		1, 6, 0 )
	ILLEGAL( 0x37,	RLA,		IndexedZeroX,		1, 6, 0 )
	OP_ADDR( 0x38,	SEC,		None,				0, 2, 0 )
	OP_ADDR( 0x39,	AND,		IndexedAbsoluteY,	2, 4, 1 )
	ILLEGAL( 0x3A,	NOP,		None,				0, 2, 0 )
	ILLEGAL( 0x3B,	RLA,		IndexedAbsoluteY,	2, 7, 0 )
	ILLEGAL( 0x3C,	SKW,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x3D,	AND,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x3E,	ROL,		IndexedAbsoluteX,	2, 7, 1 )
	ILLEGAL( 0x3F,	RLA,		IndexedAbsoluteX,	2, 7, 0 )
	OP_JUMP( 0x40,	RTI,		Return,				0, 6, 0 )
	OP_ADDR( 0x41,	EOR,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0x42,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x43,	SRE,		IndexedIndirect,	1, 8, 0 )
	ILLEGAL( 0x44,	SKB,		Zero,				1, 3, 0 )
	OP_ADDR( 0x45,	EOR,		Zero,				1, 3, 0 )
	OP_ADDR( 0x46,	LSR,		Zero,				1, 5, 0 )
	ILLEGAL( 0x47,	SRE,		Zero,				1, 5, 0 )
	OP_ADDR( 0x48,	PHA,		None,				0, 3, 0 )
	OP_ADDR( 0x49,	EOR,		Immediate,			1, 2, 0 )
	OP_ADDR( 0x4A,	LSR,		Accumulator,		0, 2, 0 )
	OP_ADDR( 0x4B,	Illegal,	None,				0, 0, 0 )
	OP_JUMP( 0x4C,	JMP,		Jmp,				2, 3, 0 )
	OP_ADDR( 0x4D,	EOR,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x4E,	LSR,		Absolute,			2, 6, 0 )
	ILLEGAL( 0x4F,	SRE,		Absolute,			2, 6, 0 )
	OP_JUMP( 0x50,	BVC,		Branch,				1, 2, 1 )
	OP_ADDR( 0x51,	EOR,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0x52,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x53,	SRE,		IndirectIndexed,	1, 8, 0 )
	ILLEGAL( 0x54,	SKB,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x55,	EOR,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x56,	LSR,		IndexedZeroX,		1, 6, 0 )
	ILLEGAL( 0x57,	SRE,		IndexedZeroX,		1, 6, 0 )
	OP_ADDR( 0x58,	CLI,		None,				0, 2, 0 )
	OP_ADDR( 0x59,	EOR,		IndexedAbsoluteY,	2, 4, 1 )
	ILLEGAL( 0x5A,	NOP,		None,				0, 2, 0 )
	ILLEGAL( 0x5B,	SRE,		IndexedAbsoluteY,	2, 7, 0 )
	ILLEGAL( 0x5C,	SKW,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x5D,	EOR,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x5E,	LSR,		IndexedAbsoluteX,	2, 7, 1 )
	ILLEGAL( 0x5F,	SRE,		IndexedAbsoluteX,	2, 7, 0 )
	OP_JUMP( 0x60,	RTS,		Return,				0, 6, 0 )
	OP_ADDR( 0x61,	ADC,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0x62,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x63,	RRA,		IndexedIndirect,	1, 8, 0 )
	ILLEGAL( 0x64,	SKB,		Zero,				1, 3, 0 )
	OP_ADDR( 0x65,	ADC,		Zero,				1, 3, 0 )
	OP_ADDR( 0x66,	ROR,		Zero,				1, 5, 0 )
	ILLEGAL( 0x67,	RRA,		Zero,				1, 5, 0 )
	OP_ADDR( 0x68,	PLA,		None,				0, 4, 0 )
	OP_ADDR( 0x69,	ADC,		Immediate,			1, 2, 0 )
	OP_ADDR( 0x6A,	ROR,		Accumulator,		0, 2, 0 )
	OP_ADDR( 0x6B,	Illegal,	None,				0, 0, 0 )
	OP_JUMP( 0x6C,	JMPI,		JmpIndirect,		2, 5, 0 )
	OP_ADDR( 0x6D,	ADC,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x6E,	ROR,		Absolute,			2, 6, 0 )
	ILLEGAL( 0x6F,	RRA,		Absolute,			2, 6, 0 )
	OP_JUMP( 0x70,	BVS,		Branch,				1, 2, 1 )
	OP_ADDR( 0x71,	ADC,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0x72,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x73,	RRA,		IndirectIndexed,	1, 8, 0 )
	ILLEGAL( 0x74,	SKB,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x75,	ADC,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x76,	ROR,		IndexedZeroX,		1, 6, 0 )
	ILLEGAL( 0x77,	RRA,		IndexedZeroX,		1, 6, 0 )
	OP_ADDR( 0x78,	SEI,		None,				0, 2, 0 )
	OP_ADDR( 0x79,	ADC,		IndexedAbsoluteY,	2, 4, 1 )
	ILLEGAL( 0x7A,	NOP,		None,				0, 2, 0 )
	ILLEGAL( 0x7B,	RRA,		IndexedAbsoluteY,	2, 7, 0 )
	ILLEGAL( 0x7C,	SKW,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x7D,	ADC,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0x7E,	ROR,		IndexedAbsoluteX,	2, 7, 1 )
	ILLEGAL( 0x7F,	RRA,		IndexedAbsoluteX,	2, 7, 0 )
	ILLEGAL( 0x80,	SKB,		Immediate,			1, 2, 0 )
	OP_ADDR( 0x81,	STA,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0x82,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0x83,	SAX,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0x84,	STY,		Zero,				1, 3, 0 )
	OP_ADDR( 0x85,	STA,		Zero,				1, 3, 0 )
	OP_ADDR( 0x86,	STX,		Zero,				1, 3, 0 )
	ILLEGAL( 0x87,	SAX,		Zero,				1, 3, 0 )
	OP_ADDR( 0x88,	DEY,		None,				0, 2, 0 )
	OP_ADDR( 0x89,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x8A,	TXA,		None,				0, 2, 0 )
	OP_ADDR( 0x8B,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x8C,	STY,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x8D,	STA,		Absolute,			2, 4, 0 )
	OP_ADDR( 0x8E,	STX,		Absolute,			2, 4, 0 )
	ILLEGAL( 0x8F,	SAX,		Absolute,			2, 4, 0 )
	OP_JUMP( 0x90,	BCC,		Branch,				1, 2, 1 )
	OP_ADDR( 0x91,	STA,		IndirectIndexed,	1, 6, 1 )
	OP_ADDR( 0x92,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x93,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x94,	STY,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x95,	STA,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0x96,	STX,		IndexedZeroY,		1, 4, 0 )
	ILLEGAL( 0x97,	SAX,		IndexedZeroY,		1, 4, 0 )
	OP_ADDR( 0x98,	TYA,		None,				0, 2, 0 )
	OP_ADDR( 0x99,	STA,		IndexedAbsoluteY,	2, 5, 0 )
	OP_ADDR( 0x9A,	TXS,		None,				0, 2, 0 )
	OP_ADDR( 0x9B,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x9C,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x9D,	STA,		IndexedAbsoluteX,	2, 5, 0 )
	OP_ADDR( 0x9E,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0x9F,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0xA0,	LDY,		Immediate,			1, 2, 0 )
	OP_ADDR( 0xA1,	LDA,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0xA2,	LDX,		Immediate,			1, 2, 0 )
	ILLEGAL( 0xA3,	LAX,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0xA4,	LDY,		Zero,				1, 3, 0 )
	OP_ADDR( 0xA5,	LDA,		Zero,				1, 3, 0 )
	OP_ADDR( 0xA6,	LDX,		Zero,				1, 3, 0 )
	ILLEGAL( 0xA7,	LAX,		Zero,				1, 3, 0 )
	OP_ADDR( 0xA8,	TAY,		None,				0, 2, 0 )
	OP_ADDR( 0xA9,	LDA,		Immediate,			1, 2, 0 )
	OP_ADDR( 0xAA,	TAX,		None,				0, 2, 0 )
	OP_ADDR( 0xAB,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0xAC,	LDY,		Absolute,			2, 4, 0 )
	OP_ADDR( 0xAD,	LDA,		Absolute,			2, 4, 0 )
	OP_ADDR( 0xAE,	LDX,		Absolute,			2, 4, 0 )
	ILLEGAL( 0xAF,	LAX,		Absolute,			2, 4, 0 )
	OP_JUMP( 0xB0,	BCS,		Branch,				1, 2, 1 )
	OP_ADDR( 0xB1,	LDA,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0xB2,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0xB3,	LAX,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0xB4,	LDY,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0xB5,	LDA,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0xB6,	LDX,		IndexedZeroY,		1, 4, 0 )
	ILLEGAL( 0xB7,	LAX,		IndexedZeroY,		1, 4, 0 )
	OP_ADDR( 0xB8,	CLV,		None,				0, 2, 0 )
	OP_ADDR( 0xB9,	LDA,		IndexedAbsoluteY,	2, 4, 1 )
	OP_ADDR( 0xBA,	TSX,		None,				0, 2, 0 )
	OP_ADDR( 0xBB,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0xBC,	LDY,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0xBD,	LDA,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0xBE,	LDX,		IndexedAbsoluteY,	2, 4, 1 )
	ILLEGAL( 0xBF,	LAX,		IndexedAbsoluteY,	2, 4, 1 )
	OP_ADDR( 0xC0,	CPY,		Immediate,			1, 2, 0 )
	OP_ADDR( 0xC1,	CMP,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0xC2,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0xC3,	DCP,		IndexedIndirect,	1, 8, 0 )
	OP_ADDR( 0xC4,	CPY,		Zero,				1, 3, 0 )
	OP_ADDR( 0xC5,	CMP,		Zero,				1, 3, 0 )
	OP_ADDR( 0xC6,	DEC,		Zero,				1, 5, 0 )
	ILLEGAL( 0xC7,	DCP,		Zero,				1, 5, 0 )
	OP_ADDR( 0xC8,	INY,		None,				0, 2, 0 )
	OP_ADDR( 0xC9,	CMP,		Immediate,			1, 2, 0 )
	OP_ADDR( 0xCA,	DEX,		None,				0, 2, 0 )
	OP_ADDR( 0xCB,	Illegal,	None,				0, 0, 0 )
	OP_ADDR( 0xCC,	CPY,		Absolute,			2, 4, 0 )
	OP_ADDR( 0xCD,	CMP,		Absolute,			2, 4, 0 )
	OP_ADDR( 0xCE,	DEC,		Absolute,			2, 6, 0 )
	ILLEGAL( 0xCF,	DCP,		Absolute,			2, 6, 0 )
	OP_JUMP( 0xD0,	BNE,		Branch,				1, 2, 1 )
	OP_ADDR( 0xD1,	CMP,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0xD2,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0xD3,	DCP,		IndirectIndexed,	1, 8, 0 )
	ILLEGAL( 0xD4,	SKB,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0xD5,	CMP,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0xD6,	DEC,		IndexedZeroX,		1, 6, 0 )
	ILLEGAL( 0xD7,	DCP,		IndexedZeroX,		1, 6, 0 )
	OP_ADDR( 0xD8,	CLD,		None,				0, 2, 0 )
	OP_ADDR( 0xD9,	CMP,		IndexedAbsoluteY,	2, 4, 1 )
	ILLEGAL( 0xDA,	NOP,		None,				0, 2, 0 )
	ILLEGAL( 0xDB,	DCP,		IndexedAbsoluteY,	2, 7, 0 )
	ILLEGAL( 0xDC,	SKW,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0xDD,	CMP,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0xDE,	DEC,		IndexedAbsoluteX,	2, 7, 1 )
	ILLEGAL( 0xDF,	DCP,		IndexedAbsoluteX,	2, 7, 0 )
	OP_ADDR( 0xE0,	CPX,		Immediate,			1, 2, 0 )
	OP_ADDR( 0xE1,	SBC,		IndexedIndirect,	1, 6, 0 )
	OP_ADDR( 0xE2,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0xE3,	ISB,		IndexedIndirect,	1, 8, 0 )
	OP_ADDR( 0xE4,	CPX,		Zero,				1, 3, 0 )
	OP_ADDR( 0xE5,	SBC,		Zero,				1, 3, 0 )
	OP_ADDR( 0xE6,	INC,		Zero,				1, 5, 0 )
	ILLEGAL( 0xE7,	ISB,		Zero,				1, 5, 0 )
	OP_ADDR( 0xE8,	INX,		None,				0, 2, 0 )
	OP_ADDR( 0xE9,	SBC,		Immediate,			1, 2, 0 )
	OP_ADDR( 0xEA,	NOP,		None,				0, 2, 0 )
	ILLEGAL( 0xEB,	SBC,		Immediate,			1, 2, 0 )
	OP_ADDR( 0xEC,	CPX,		Absolute,			2, 4, 0 )
	OP_ADDR( 0xED,	SBC,		Absolute,			2, 4, 0 )
	OP_ADDR( 0xEE,	INC,		Absolute,			2, 6, 0 )
	ILLEGAL( 0xEF,	ISB,		Absolute,			2, 6, 0 )
	OP_JUMP( 0xF0,	BEQ,		Branch,				1, 2, 1 )
	OP_ADDR( 0xF1,	SBC,		IndirectIndexed,	1, 5, 1 )
	OP_ADDR( 0xF2,	Illegal,	None,				0, 0, 0 )
	ILLEGAL( 0xF3,	ISB,		IndirectIndexed,	1, 8, 0 )
	ILLEGAL( 0xF4,	SKB,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0xF5,	SBC,		IndexedZeroX,		1, 4, 0 )
	OP_ADDR( 0xF6,	INC,		IndexedZeroX,		1, 6, 0 )
	ILLEGAL( 0xF7,	ISB,		IndexedZeroX,		1, 6, 0 )
	OP_ADDR( 0xF8,	SED,		None,				0, 2, 0 )
	OP_ADDR( 0xF9,	SBC,		IndexedAbsoluteY,	2, 4, 1 )
	ILLEGAL( 0xFA,	NOP,		None,				0, 2, 0 )
	ILLEGAL( 0xFB,	ISB,		IndexedAbsoluteY,	2, 7, 0 )
	ILLEGAL( 0xFC,	SKW,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0xFD,	SBC,		IndexedAbsoluteX,	2, 4, 1 )
	OP_ADDR( 0xFE,	INC,		IndexedAbsoluteX,	2, 7, 1 )
	ILLEGAL( 0xFF,	ISB,		IndexedAbsoluteX,	2, 7, 0 )
}