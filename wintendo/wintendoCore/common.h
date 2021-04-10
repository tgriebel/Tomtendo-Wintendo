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

using masterCycle_t		= std::chrono::duration< uint64_t, std::ratio<1, MasterClockHz> >;
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

static const std::chrono::nanoseconds FrameLatencyNs = std::chrono::duration_cast<chrono::nanoseconds>( frameRate_t( 1 ) );
static const std::chrono::nanoseconds MaxFrameLatencyNs = 4 * FrameLatencyNs;

const uint32_t KB_1		= 1024;
const uint32_t MB_1		= 1024 * KB_1;

#define KB(n) ( n * KB_1 )
#define BIT_MASK(n)	( 1 << n )
#define SELECT_BIT( word, bit ) ( (word) & (BIT_MASK_##bit) ) >> (BIT_##bit)

#define FORCE_INLINE __forceinline

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

	virtual uint8_t			OnLoadCpu() { return 0; };
	virtual uint8_t			OnLoadPpu() { return 0; };
	virtual uint8_t			ReadRom( const uint16_t addr ) = 0;
	virtual uint8_t			ReadChrRom( const uint16_t addr ) { return 0; };
	virtual uint8_t			WriteChrRam( const uint16_t addr, const uint8_t value ) { return 0; };
	virtual uint8_t			Write( const uint16_t addr, const uint8_t value ) { return 0; };
	virtual bool			InWriteWindow( const uint16_t addr, const uint16_t offset ) { return false; };
	
	virtual void			Serialize( Serializer& serializer ) {};
	virtual void			Clock() {};
};


class wtCart
{
private:
	uint8_t*				rom;
	size_t					size;
	size_t					prgSize;
	size_t					chrSize;

public:
	wtRomHeader				h;
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

		memcpy( &h, &header, sizeof( wtRomHeader ) );
		memcpy( rom, romData, romSize );
		size = romSize;
		prgSize = KB( 16 ) * (size_t)h.prgRomBanks;
		chrSize = KB( 8 ) * (size_t)h.chrRomBanks;

		assert( h.type[ 0 ] == 'N' );
		assert( h.type[ 1 ] == 'E' );
		assert( h.type[ 2 ] == 'S' );
		assert( h.magic == 0x1A );
	}

	~wtCart()
	{
		memset( &h, 0, sizeof( wtRomHeader ) );
		if( rom != nullptr )
		{
			delete[] rom ;
			rom = nullptr;
		}
		size = 0;
	}

	uint8_t* GetPrgRomBank( const uint32_t bankNum, const uint32_t bankSize = KB(16) )
	{
		const size_t addr = ( bankNum * (size_t)bankSize ) % prgSize;
		assert( addr < size );
		return &rom[ addr ];
	}

	uint8_t GetPrgRomBankAddr( const uint32_t address )
	{
		assert( address < size );
		return rom[ address ];
	}

	uint8_t* GetChrRomBank( const uint32_t bankNum, const uint32_t bankSize = KB(4) )
	{
		const size_t addr = prgSize + ( bankNum * (size_t)bankSize ) % chrSize;
		assert( addr < size );
		return &rom[ addr ];
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


struct debugTiming_t
{
	uint32_t		frameTimeUs;
	uint32_t		totalTimeUs;
	uint32_t		elapsedTimeUs;
	uint64_t		frameNumber;
	uint64_t		framePerRun;
	uint64_t		runInvocations;
	masterCycle_t	cycleBegin;
	masterCycle_t	cycleEnd;
};


struct config_t
{
	//struct System
	//{
	//} sys;

	//struct CPU
	//{
	//} cpu;

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

	void Set( Serializer& s )
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
	}

	void WriteTo( Serializer& s ) const
	{
		assert( s.BufferSize() >= byteCount );
		if( s.BufferSize() < byteCount ) {
			return;
		}
		s.SetMode( serializeMode_t::STORE );
		s.NextArray( bytes, byteCount );
		s.SetPosition( 0 );
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