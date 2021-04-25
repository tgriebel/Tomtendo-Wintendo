#pragma once

#include <string>
#include <map>
#include <queue>
#include <thread>
#include <chrono>
#include "bitmap.h"
#include "util.h"
#include "serializer.h"
#include "assert.h"
#include "time.h"

#define NES_MODE			(1)
#define DEBUG_MODE			(0)
#define DEBUG_ADDR			(1)
#define MIRROR_OPTIMIZATION	(1)

const uint32_t KB_1		= 1024;
const uint32_t MB_1		= 1024 * KB_1;

#define KB(n) ( n * KB_1 )
#define BIT_MASK(n)	( 1 << n )
#define SELECT_BIT( word, bit ) ( (word) & (BIT_MASK_##bit) ) >> (BIT_##bit)

#ifdef _MSC_BUILD
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif

#define DEFINE_ENUM_OPERATORS( enumType, intType )														\
inline enumType operator|=( enumType lhs, enumType rhs )												\
{																										\
	return static_cast< enumType >( static_cast< intType >( lhs ) | static_cast< intType >( rhs ) );	\
}																										\
																										\
inline bool operator&( enumType lhs, enumType rhs )														\
{																										\
	return ( ( static_cast< intType >( lhs ) & static_cast< intType >( rhs ) ) != 0 );					\
}																										\
																										\
inline enumType operator>>( const enumType lhs, const enumType rhs )									\
{																										\
	return static_cast<enumType>( static_cast<intType>( lhs ) >> static_cast<intType>( rhs ) );			\
}																										\
																										\
inline enumType operator<<( const enumType lhs, const enumType rhs )									\
{																										\
	return static_cast<enumType>( static_cast<intType>( lhs ) << static_cast<intType>( rhs ) );			\
}

enum analogMode_t
{
	NTSC,
	PAL,
	ANALOG_MODE_COUNT,
};

struct debugTiming_t
{
	uint32_t		frameTimeUs;
	uint32_t		totalTimeUs;
	uint32_t		simulationTimeUs;
	uint32_t		realTimeUs;
	uint64_t		frameNumber;
	uint64_t		framePerRun;
	uint64_t		runInvocations;
	masterCycle_t	cycleBegin;
	masterCycle_t	cycleEnd;
	masterCycle_t	stateCycle;
};


enum class emulationFlags_t : uint32_t
{
	NONE		= 0,
	CLAMP_FPS	= BIT_MASK( 1 ),
	LIMIT_STALL = BIT_MASK( 2 ),
	HEADLESS	= BIT_MASK( 3 ),
	ALL			= 0xFFFFFFFF,
};
DEFINE_ENUM_OPERATORS( emulationFlags_t, uint32_t )


struct config_t
{
	struct System
	{
		emulationFlags_t	flags;
	} sys;

	//struct CPU
	//{
	//} cpu;

	struct APU
	{
		float				volume;
		float				frequencyScale;
		int32_t				waveShift;
		bool				disableSweep;
		bool				disableEnvelope;
		bool				mutePulse1;
		bool				mutePulse2;
		bool				muteTri;
		bool				muteNoise;
		bool				muteDMC;
		uint8_t				dbgChannelBits;
	} apu;

	struct PPU
	{
		int32_t				chrPalette;
		int32_t				spriteLimit;
		bool				showBG;
		bool				showSprite;
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
	virtual const Pixel&			Get( const uint32_t index ) = 0;
	virtual void					Clear( const uint32_t r8g8b8a8 = 0 ) = 0;

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

	FORCE_INLINE void Set( const uint32_t index, const Pixel value )
	{
		assert( index < length );

		//if ( index < length )
		{
			buffer[index] = value;
		}
	}

	const Pixel& Get( const uint32_t index )
	{
		assert( index < length );
		if ( index < length )
		{
			return buffer[ index ];
		}
		return buffer[ 0 ];
	}

	inline void SetDebugName( const char* debugName )
	{
		name = debugName;
	}

	void Clear( const uint32_t r8g8b8a8 = 0 )
	{
		for ( uint32_t i = 0; i < length; ++i )
		{
			buffer[i].rawABGR = r8g8b8a8;
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

#define STATE_MEMORY_LABEL	"Memory"
#define STATE_VRAM_LABEL	"VRAM"

struct stateHeader_t
{
	uint8_t* memory;
	uint8_t* vram;
	uint32_t memorySize;
	uint32_t vramSize;
};


class wtStateBlob
{
public:

	wtStateBlob()
	{
		bytes = nullptr;
		byteCount = 0;
		cycle = masterCycle_t( 0 );
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

	void Set( Serializer& s, const masterCycle_t sysCycle )
	{
		Reset();
		byteCount = s.CurrentSize();
		bytes = new uint8_t[ byteCount ];
		memcpy( bytes, s.GetPtr(), byteCount );
		
		serializerHeader_t::section_t* memSection;
		s.FindLabel( STATE_MEMORY_LABEL, &memSection );
		header.memory = bytes + memSection->offset;
		header.memorySize = memSection->size;

		serializerHeader_t::section_t* vramSection;
		s.FindLabel( STATE_VRAM_LABEL, &vramSection );
		header.vram = bytes + vramSection->offset;
		header.vramSize = vramSection->size;

		cycle = sysCycle;
	}

	void WriteTo( Serializer& s ) const
	{
		assert( s.BufferSize() >= byteCount );
		if( s.BufferSize() < byteCount ) {
			return;
		}
		const serializeMode_t mode = s.GetMode();
		s.SetMode( serializeMode_t::STORE );
		s.NextArray( bytes, byteCount );
		s.SetPosition( 0 );
		s.SetMode( mode );
	}

	void Reset()
	{
		if ( bytes == nullptr )
		{
			delete[] bytes;
			byteCount = 0;
		}
		cycle = masterCycle_t( 0 );
	}

	stateHeader_t	header;
private:
	uint8_t*		bytes;
	uint32_t		byteCount;
	masterCycle_t	cycle;
};

FORCE_INLINE uint16_t Combine( const uint8_t lsb, const uint8_t msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}

FORCE_INLINE bool InRange( const uint16_t addr, const uint16_t loAddr, const uint16_t hiAddr )
{
	return ( addr >= loAddr ) && ( addr <= hiAddr );
}