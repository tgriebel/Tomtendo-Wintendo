#pragma once

#include <string>
#include <assert.h>
#include <map>
#include <queue>
#include <thread>
#include <chrono>
#include "bitmap.h"
#include "serializer.h"

#define NES_MODE			(1)
#define DEBUG_MODE			(0)
#define DEBUG_ADDR			(1)
#define MIRROR_OPTIMIZATION	(1)

const uint64_t MasterClockHz		= 21477272;
const uint64_t CpuClockDivide		= 12;
const uint64_t ApuClockDivide		= 24;
const uint64_t ApuSequenceDivide	= 89490;
const uint64_t PpuClockDivide		= 4;
const uint64_t PpuCyclesPerScanline	= 341;
const uint64_t FPS					= 60;
const uint64_t MinFPS				= 30;

using masterCycles_t	= std::chrono::duration< uint64_t, std::ratio<1, MasterClockHz> >;
using ppuCycle_t		= std::chrono::duration< uint64_t, std::ratio<PpuClockDivide, MasterClockHz> >;
using cpuCycle_t		= std::chrono::duration< uint64_t, std::ratio<CpuClockDivide, MasterClockHz> >;
using apuCycle_t		= std::chrono::duration< uint64_t, std::ratio<ApuClockDivide, MasterClockHz> >;
using apuSeqCycle_t		= std::chrono::duration< uint64_t, std::ratio<ApuSequenceDivide, MasterClockHz> >;
using scanCycle_t		= std::chrono::duration< uint64_t, std::ratio<PpuCyclesPerScanline * PpuClockDivide, MasterClockHz> >; // TODO: Verify
using frameRate_t		= std::chrono::duration< double, std::ratio<1, FPS> >;
using timePoint_t		= std::chrono::time_point< std::chrono::steady_clock >;

static constexpr uint64_t CPU_HZ = chrono::duration_cast<cpuCycle_t>( chrono::seconds( 1 ) ).count();
static constexpr uint64_t APU_HZ = chrono::duration_cast<apuCycle_t>( chrono::seconds( 1 ) ).count();
static constexpr uint64_t PPU_HZ = chrono::duration_cast<ppuCycle_t>( chrono::seconds( 1 ) ).count();

const uint32_t KB_1		= 1024;
const uint32_t MB_1		= 1024 * KB_1;

#define KB(n) ( n * KB_1 )
#define BIT_MASK(n)	( 1 << n )
#define SELECT_BIT( word, bit ) ( (word) & (BIT_MASK_##bit) ) >> (BIT_##bit)

class wtSystem;

struct Controller
{
};


enum analogMode_t
{
	NTSC,
	PAL,
	ANALOG_MODE_COUNT,
};


enum wtMirrorMode : uint8_t
{
	MIRROR_MODE_SINGLE,
	MIRROR_MODE_HORIZONTAL,
	MIRROR_MODE_VERTICAL,
	MIRROR_MODE_FOURSCREEN,
	MIRROR_MODE_SINGLE_LO,
	MIRROR_MODE_SINGLE_HI,
	MIRROR_MODE_COUNT
};

// TODO: bother with endianness?
struct wtRomHeader
{
	uint8_t type[3];
	uint8_t magic;
	uint8_t prgRomBanks;
	uint8_t chrRomBanks;
	struct ControlsBits0
	{
		uint8_t mirror : 1;
		uint8_t usesBattery : 1;
		uint8_t usesTrainer : 1;
		uint8_t fourScreenMirror : 1;
		uint8_t mapperNumberLower : 4;
	} controlBits0;
	struct ControlsBits1
	{
		uint8_t reserved0 : 4;
		uint8_t mappedNumberUpper : 4;
	} controlBits1;
	uint8_t reserved[8];
};


class wtMapper
{
protected:
	uint32_t mapperId;

public:
	wtSystem* system;

	virtual uint8_t	OnLoadCpu() { return 0; };
	virtual uint8_t	OnLoadPpu() { return 0; };
	virtual uint8_t	ReadRom( const uint16_t addr ) = 0;
	virtual uint8_t	ReadChrRom( const uint16_t addr ) { return 0; };
	virtual uint8_t	WriteChrRam( const uint16_t addr, const uint8_t value ) { return 0; };
	virtual uint8_t	Write( const uint16_t addr, const uint16_t offset, const uint8_t value ) { return 0; };
	virtual bool	InWriteWindow( const uint16_t addr, const uint16_t offset ) { return false; };
	
	virtual void	Serialize( Serializer& serializer, const serializeMode_t mode ) {};
	virtual void	Clock() {};
};


struct wtCart
{
public:
	// WARNING: This data is directly copied into right
	wtRomHeader				h;
	uint8_t*				rom;
	// Fine to add data after here
	size_t					size;
	unique_ptr<wtMapper>	mapper;

	wtCart()
	{
		memset( &h, 0, sizeof( wtRomHeader ) );
		rom = nullptr;
		size = 0;
	}

	wtCart( wtRomHeader& header, uint8_t* romData, uint32_t romSize )
	{
		rom = new uint8_t[ romSize ];

		assert( romSize <= 1 * MB_1 );
		memcpy( &h, &header, sizeof( wtRomHeader ) );
		memcpy( rom, romData, romSize );
		size = romSize;

		assert( h.type[ 0 ] == 'N' );
		assert( h.type[ 1 ] == 'E' );
		assert( h.type[ 2 ] == 'S' );
		assert( h.magic == 0x1A );
	}

	~wtCart()
	{
		memset( &h, 0, sizeof( wtRomHeader ) );
		if( rom == nullptr )
		{
			delete[] rom ;
			rom = nullptr;
		}
		size = 0;
	}

	uint8_t* GetPrgRomBank( const uint32_t bankNum, const uint32_t bankSize = KB(16) ) {
		//const uint32_t bankRatio = KB_16 / bankSize;
		//assert( bankNum <= bankRatio * GetPrgBankCount() );
		return &rom[ bankNum * bankSize ];
	}

	uint8_t* GetChrRomBank( const uint32_t bankNum, const uint32_t bankSize = KB(4) )
	{
		//const uint32_t bankRatio = KB_4 / bankSize;
		//assert( bankNum <= bankRatio * GetChrBankCount() );
		const uint32_t chrRomStart = h.prgRomBanks * KB(16);
		return &rom[ chrRomStart + bankNum * bankSize ];
	}

	uint8_t GetPrgBankCount() const
	{
		return h.prgRomBanks;
	}

	uint8_t GetChrBankCount() const
	{
		return h.chrRomBanks;
	}

	uint8_t HasChrRam() const
	{
		return ( h.chrRomBanks == 0 );
	}

	uint8_t HasSave() const
	{
		return h.controlBits0.usesBattery;
	}

	uint32_t GetMapperId() const {
		return ( h.controlBits1.mappedNumberUpper << 4 ) | h.controlBits0.mapperNumberLower;
	}
};


struct wtDebugInfo
{
	uint32_t		frameTimeUs;
	uint64_t		frameNumber;
	uint64_t		framePerRun;
	uint64_t		runInvocations;
	masterCycles_t	cycleBegin;
	masterCycles_t	cycleEnd;
};


struct wtConfig
{
	struct System
	{
		int32_t		restoreFrame;
		int32_t		nextScaline;
		bool		replay;
		bool		record;
		bool		requestSaveState;
		bool		requestLoadState;
	} sys;

	struct CPU
	{
		int32_t		traceFrameCount;
	} cpu;

	struct APU
	{
		float		volume;
		float		frequencyScale;
		int32_t		waveShift;
		bool		disableSweep;
		bool		disableEnvelope;
		bool		mutePulse1;
		bool		mutePulse2;
		bool		muteTri;
		bool		muteNoise;
		bool		muteDMC;
	} apu;

	struct PPU
	{
		int32_t		chrPalette;
		int32_t		spriteLimit;
		bool		showBG;
		bool		showSprite;
	} ppu;
};


struct wtPoint
{
	int32_t x;
	int32_t y;
};


struct wtRect
{
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};


class wtRawImageInterface
{
public:
	virtual void					SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel ) = 0;
	virtual void					Set( const uint32_t index, const Pixel value ) = 0;
	virtual void					Clear() = 0;

	virtual const uint32_t *const	GetRawBuffer() const = 0;
	virtual uint32_t				GetWidth() const = 0;
	virtual uint32_t				GetHeight() const = 0;
	virtual uint32_t				GetBufferLength() const = 0;
	virtual const char*				GetDebugName() const = 0;
	virtual void					SetDebugName( const char* debugName ) = 0;
};


template< uint32_t N, uint32 M >
class wtRawImage : public wtRawImageInterface
{
public:

	wtRawImage()
	{
		Clear();
		name = "";
	}

	wtRawImage( const char* name_ )
	{
		Clear();
		name = name_;
	}

	wtRawImage& operator=( const wtRawImage& _image )
	{
		for ( uint32_t i = 0; i < _image.length; ++i )
		{
			buffer[i].rawABGR = _image.buffer[i].rawABGR;
		}

		name = _image.name;

		return *this;
	}

	void SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel )
	{
		const uint32_t index = ( x + y * width );
		assert( index < length );

		if ( index < length )
		{
			buffer[index] = pixel;
		}
	}

	void Set( const uint32_t index, const Pixel value )
	{
		assert( index < length );

		if ( index < length )
		{
			buffer[index] = value;
		}
	}

	inline void SetDebugName( const char* debugName )
	{
		name = debugName;
	}

	void Clear()
	{
		for ( uint32_t i = 0; i < length; ++i )
		{
			buffer[i].rawABGR = 0;
		}
	}

	inline const uint32_t *const GetRawBuffer() const
	{
		return &buffer[0].rawABGR;
	}

	inline uint32_t GetWidth() const
	{
		return width;
	}

	inline uint32_t GetHeight() const
	{
		return height;
	}

	inline uint32_t GetBufferLength() const
	{
		return length;
	}

	inline const char* GetDebugName() const
	{
		return name;
	}

	inline wtRect GetRect()
	{
		return { 0, 0, width, height };
	}

private:
	static const uint32_t width = N;
	static const uint32_t height = M;
	static const uint32_t length = N * M;
	Pixel buffer[length];
	const char* name;
};

using wtDisplayImage = wtRawImage<256, 240>;
using wtNameTableImage = wtRawImage<2 * 256, 2 *240>;
using wtPaletteImage = wtRawImage<16, 2>;
using wtPatternTableImage = wtRawImage<128, 128>;
using wt16x8ChrImage = wtRawImage<8, 16>;
using wt8x8ChrImage = wtRawImage<8, 8>;

enum class wtImageTag
{
	FRAME_BUFFER,
	NAMETABLE,
	PALETTE,
	PATTERN_TABLE_0,
	PATTERN_TABLE_1
};


class wtStateBlob
{
public:
	wtStateBlob()
	{
		bytes = nullptr;
		byteCount = 0;
		cycle = masterCycles_t( 0 );
	}

	~wtStateBlob()
	{
		Reset();
	}

	bool IsValid() const {
		return ( byteCount > 0 );
	}

	uint32_t GetBufferSize() const {
		return byteCount;
	}

	uint8_t* GetPtr() {
		return bytes;
	}

	void Set( Serializer& s )
	{
		Reset();
		byteCount = s.CurrentSize();
		bytes = new uint8_t[ byteCount ];
		memcpy( bytes, s.GetPtr(), byteCount );
	}

	void WriteTo( Serializer& s ) const
	{
		assert( s.BufferSize() >= byteCount );
		if( s.BufferSize() < byteCount ) {
			return;
		}
		s.NextArray( bytes, byteCount, serializeMode_t::STORE );
		s.SetPosition( 0 );
	}

	void Reset()
	{
		if ( bytes == nullptr )
		{
			delete[] bytes;
			byteCount = 0;
		}
		cycle = masterCycles_t( 0 );
	}

private:
	uint8_t*		bytes;
	uint32_t		byteCount;
	masterCycles_t	cycle;
};


template< uint16_t B >
class wtShiftReg
{
public:

	wtShiftReg()
	{
		Clear();
	}

	void Shift( const bool bitValue )
	{
		reg >>= 1;
		reg &= LMask;
		reg |= ( static_cast<uint32_t>( bitValue ) << ( Bits - 1 ) ) & ~LMask;

		++shifts;
	}

	uint32_t GetShiftCnt() const
	{
		return shifts;
	}

	bool IsFull() const
	{
		return ( shifts >= Bits );
	}

	bool GetBitValue( const uint16_t bit ) const
	{
		return ( ( reg >> bit ) & 0x01 );
	}

	uint32_t GetValue() const
	{
		return reg & Mask;
	}

	void Set( const uint32_t value )
	{
		reg = value;
		shifts = 0;
	}

	void Clear()
	{
		reg = 0;
		shifts = 0;
	}

private:
	uint32_t reg;
	uint32_t shifts;
	static const uint16_t Bits = B;

	static constexpr uint32_t CalcMask()
	{
		uint32_t mask = 0x02;
		for ( uint32_t i = 1; i < B; ++i )
		{
			mask <<= 1;
		}
		return ( mask - 1 );
	}

	static const uint32_t Mask = CalcMask();
	static const uint32_t LMask = ( Mask >> 1 );
	static const uint32_t RMask = ~0x01ul;
};


template < uint16_t B >
class BitCounter
{
public:
	BitCounter() {
		Reload();
	}

	void Inc() {
		bits++;
	}

	void Dec() {
		bits--;
	}

	void Reload( uint16_t value = 0 )
	{
		bits = value;
		unused = 0;
	}

	uint16_t Value() {
		return bits;
	}

	bool IsZero() {
		return ( Value() == 0 );
	}
private:
	uint16_t bits	: B;
	uint16_t unused	: ( 16 - B );
};


inline uint16_t Combine( const uint8_t lsb, const uint8_t msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}

inline bool InRange( const uint16_t addr, const uint16_t loAddr, const uint16_t hiAddr )
{
	return ( addr >= loAddr ) && ( addr <= hiAddr );
}