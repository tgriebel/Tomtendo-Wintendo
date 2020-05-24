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
using ppuCycle_t = std::chrono::duration< uint64_t, std::ratio<PpuClockDivide, MasterClockHz> >;
using cpuCycle_t = std::chrono::duration< uint64_t, std::ratio<CpuClockDivide, MasterClockHz> >;
using scanCycle_t = std::chrono::duration< uint64_t, std::ratio<PpuCyclesPerScanline * PpuClockDivide, MasterClockHz> >; // TODO: Verify
using frameRate_t = std::chrono::duration< double, std::ratio<1, FPS> >;

struct iNesHeader
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


struct NesCart
{
	iNesHeader	header;
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


class wtRawImage
{
public:

	wtRawImage()
	{
		width = 0;
		height = 0;
		length = 0;
		buffer = nullptr;
	}

	wtRawImage( const uint32_t _width, const uint32_t _height )
	{
		width = _width;
		height = _height;
		length = ( width * height );
		buffer = new Pixel[length];
	}

	wtRawImage( const wtRawImage& _image ) = delete;

	wtRawImage& operator=( const wtRawImage& _image )
	{
		width = _image.width;
		height = _image.height;
		length = _image.length;
		buffer = new Pixel[length];

		assert( buffer != nullptr );

		for ( uint32_t i = 0; i < _image.length; ++i )
		{
			buffer[i].raw = _image.buffer[i].raw;
		}

		return *this;
	}

	~wtRawImage()
	{
		if ( buffer != nullptr )
		{
			delete[] buffer;
			buffer = nullptr;

			length = 0;
			width = 0;
			height = 0;
		}
	}

	void SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel )
	{
		const uint32_t index = ( x + y * width );
		assert( buffer != nullptr );
		assert( index < length );

		if ( ( buffer != nullptr ) && ( index < length ) )
		{
			buffer[index] = pixel;
		}
	}

	void Set( const uint32_t index, const Pixel value )
	{
		assert( buffer != nullptr );
		assert( index < length );

		if ( ( buffer != nullptr ) && ( index < length ) )
		{
			buffer[index] = value;
		}
	}

	void Clear()
	{
		if ( buffer == nullptr )
			return;

		for ( uint32_t i = 0; i < length; ++i )
		{
			buffer[i].raw = 0;
		}
	}

	inline const uint32_t* GetRawBuffer()
	{
		return &buffer[0].raw;
	}

	inline uint32_t GetWidth()
	{
		return width;
	}

	inline uint32_t GetHeight()
	{
		return height;
	}

	inline uint32_t GetBufferSize()
	{
		return ( sizeof( buffer[0] ) * length );
	}

private:
	uint32_t width;
	uint32_t height;
	uint32_t length;
	Pixel* buffer;
};


enum class wtImageTag
{
	FRAME_BUFFER,
	NAMETABLE,
	PALETTE,
	PATTERN_TABLE_0,
	PATTERN_TABLE_1
};


static void LoadNesFile( const std::string& fileName, NesCart& outCart )
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
	
	std::ofstream checkFile;
	checkFile.open( "checkFile.nes", std::ios::binary );
	checkFile.write( reinterpret_cast<char*>( &outCart ), len );
	checkFile.close();
	

	outCart.size = len - sizeof( outCart.header ); // TODO: trainer needs to be checked
}


inline uint16_t Combine( const uint8_t lsb, const uint8_t msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}
#endif // __COMMONTYPES__