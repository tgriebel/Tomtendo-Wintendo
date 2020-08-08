#ifndef __COMMONTYPES__
#define __COMMONTYPES__

#include <string>
#include <chrono>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <iomanip>
#include "bitmap.h"

#include <thread> // TODO: remove for testing
#include <chrono>

#define NES_MODE (1)
#define DEBUG_MODE (1)
#define DEBUG_ADDR (0)
#define MIRROR_OPTIMIZATION (1)

inline constexpr uint8_t operator "" _b( uint64_t arg ) noexcept
{
	return static_cast< uint8_t >( arg & 0xFF );
}

const uint64_t MasterClockHz		= 21477272;
const uint64_t CpuClockDivide		= 12;
const uint64_t PpuClockDivide		= 4;
const uint64_t PpuCyclesPerScanline	= 341;
const uint64_t FPS					= 60;

using masterCycles_t = std::chrono::duration< uint64_t, std::ratio<1, MasterClockHz> >;
using ppuCycle_t	= std::chrono::duration< uint64_t, std::ratio<PpuClockDivide, MasterClockHz> >;
using cpuCycle_t	= std::chrono::duration< uint64_t, std::ratio<CpuClockDivide, MasterClockHz> >;
using scanCycle_t	= std::chrono::duration< uint64_t, std::ratio<PpuCyclesPerScanline * PpuClockDivide, MasterClockHz> >; // TODO: Verify
using frameRate_t	= std::chrono::duration< double, std::ratio<1, FPS> >;

const uint32_t KB_1		= 1024;
const uint32_t KB_2		= ( 2 * KB_1 );
const uint32_t KB_4		= ( 2 * KB_2 );
const uint32_t KB_8		= ( 2 * KB_4 );
const uint32_t KB_16	= ( 2 * KB_8 );
const uint32_t KB_32	= ( 2 * KB_16 );

const uint16_t BIT_0	= 0;
const uint16_t BIT_1	= 1;
const uint16_t BIT_2	= 2;
const uint16_t BIT_3	= 3;
const uint16_t BIT_4	= 4;
const uint16_t BIT_5	= 5;
const uint16_t BIT_6	= 6;
const uint16_t BIT_7	= 7;
const uint16_t BIT_8	= 8;
const uint16_t BIT_9	= 9;
const uint16_t BIT_10	= 10;
const uint16_t BIT_11	= 11;
const uint16_t BIT_12	= 12;
const uint16_t BIT_13	= 13;
const uint16_t BIT_14	= 14;
const uint16_t BIT_15	= 15;

const uint16_t BIT_MASK_0	= ( 1 << 0 );
const uint16_t BIT_MASK_1	= ( 1 << 1 );
const uint16_t BIT_MASK_2	= ( 1 << 2 );
const uint16_t BIT_MASK_3	= ( 1 << 3 );
const uint16_t BIT_MASK_4	= ( 1 << 4 );
const uint16_t BIT_MASK_5	= ( 1 << 5 );
const uint16_t BIT_MASK_6	= ( 1 << 6 );
const uint16_t BIT_MASK_7	= ( 1 << 7 );
const uint16_t BIT_MASK_8	= ( 1 << 8 );
const uint16_t BIT_MASK_9	= ( 1 << 9 );
const uint16_t BIT_MASK_10	= ( 1 << 10 );
const uint16_t BIT_MASK_11	= ( 1 << 11 );
const uint16_t BIT_MASK_12	= ( 1 << 12 );
const uint16_t BIT_MASK_13	= ( 1 << 13 );
const uint16_t BIT_MASK_14	= ( 1 << 14 );
const uint16_t BIT_MASK_15	= ( 1 << 15 );

#define SELECT_BIT( x, y ) ( (x) & (BIT_MASK_##y) ) >> (BIT_##y)

class wtSystem;

struct Controller
{
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

	virtual uint8_t OnLoadCpu() = 0;
	virtual uint8_t OnLoadPpu() = 0;
	virtual uint8_t Write( const uint16_t addr, const uint16_t offset, const uint8_t value ) = 0;
	virtual bool InWriteWindow( const uint16_t addr, const uint16_t offset ) = 0;
};


struct wtCart
{
	// WARNING: This data is directly copied into right
	wtRomHeader				header;
	uint8_t					rom[524288];
	// Fine to add data after here
	size_t					size;
	unique_ptr<wtMapper>	mapper;

	uint8_t* GetPrgRomBank( const uint8_t bankNum )
	{
		return &rom[bankNum * KB_16];
	}

	uint8_t* GetChrRomBank( const uint8_t bankNum, const bool sizeIs8KB )
	{
		const uint32_t chrRomStart = header.prgRomBanks * KB_16;
		return &rom[chrRomStart + bankNum * ( sizeIs8KB ? KB_8 : KB_4 )];
	}

	uint32_t GetMapperId() const
	{
		return ( header.controlBits1.mappedNumberUpper << 4 ) | header.controlBits0.mapperNumberLower;
	}
};


struct wtDebugInfo
{
	uint32_t frameTimeUs;
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
	virtual void SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel ) = 0;
	virtual void Set( const uint32_t index, const Pixel value ) = 0;
	virtual void Clear() = 0;

	virtual const uint32_t *const GetRawBuffer() const = 0;
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual uint32_t GetBufferLength() const = 0;
	virtual const char* GetDebugName() const = 0;
	virtual void SetDebugName( const char* debugName ) = 0;
};


template< uint32_t N, uint32 M >
class wtRawImage : public wtRawImageInterface
{
public:

	wtRawImage()
	{
		Clear();
		locked = true;
		name = "";
	}

	wtRawImage( const char* name_ )
	{
		Clear();
		locked = true;
		name = name_;
	}

	//wtRawImage( const wtRawImage& _image ) = delete;

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

		locked = false;
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

	bool locked;

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

enum class wtImageTag
{
	FRAME_BUFFER,
	NAMETABLE,
	PALETTE,
	PATTERN_TABLE_0,
	PATTERN_TABLE_1
};


template< uint8_t B >
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

	bool GetBitValue( const uint8_t bit ) const
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
	static const uint8_t Bits = B;

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


inline uint16_t Combine( const uint8_t lsb, const uint8_t msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}

inline bool InRange( const uint16_t addr, const uint16_t loAddr, const uint16_t hiAddr )
{
	return ( addr >= loAddr ) && ( addr <= hiAddr );
}
#endif // __COMMONTYPES__