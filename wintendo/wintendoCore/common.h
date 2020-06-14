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

#define NES_MODE 1
#define DEBUG_MODE 0
#define DEBUG_ADDR 0

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


struct wtCart
{
	wtRomHeader	header;
	uint8_t		rom[524288];
	size_t		size;
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
	virtual uint32_t GetBufferSize() const = 0;
	virtual const char* GetName() const = 0;
	virtual void SetName( const char* debugName ) = 0;
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

	inline void SetName( const char* debugName )
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

	inline uint32_t GetBufferSize() const
	{
		return ( sizeof( buffer[0] ) * length );
	}

	inline const char* GetName() const
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


static void LoadNesFile( const std::wstring& fileName, wtCart& outCart )
{
	std::ifstream nesFile;
	nesFile.open( fileName, std::ios::binary );

	assert( nesFile.good() );

	nesFile.seekg( 0, std::ios::end );
	size_t len = static_cast<size_t>( nesFile.tellg() );

	nesFile.seekg( 0, std::ios::beg );
	nesFile.read( reinterpret_cast<char*>( &outCart ), len );
	nesFile.close();

	assert( outCart.header.type[0] == 'N' );
	assert( outCart.header.type[1] == 'E' );
	assert( outCart.header.type[2] == 'S' );
	assert( outCart.header.magic == 0x1A );

	outCart.size = len - sizeof( outCart.header ); // TODO: trainer needs to be checked
}


inline uint16_t Combine( const uint8_t lsb, const uint8_t msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}
#endif // __COMMONTYPES__