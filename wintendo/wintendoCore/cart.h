#pragma once
#include "common.h"

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
	uint8_t type[ 3 ];
	uint8_t magic;
	uint8_t prgRomBanks;
	uint8_t chrRomBanks;
	struct ControlsBits0
	{
		uint8_t mirror				: 1;
		uint8_t usesBattery			: 1;
		uint8_t usesTrainer			: 1;
		uint8_t fourScreenMirror	: 1;
		uint8_t mapperNumberLower	: 4;
	} controlBits0;
	struct ControlsBits1
	{
		uint8_t reserved0			: 4;
		uint8_t mappedNumberUpper	: 4;
	} controlBits1;
	uint8_t reserved[ 8 ];
};


class wtMapper
{
protected:
	uint32_t mapperId;

public:
	wtSystem* system;

	virtual uint8_t			OnLoadCpu() { return 0; };
	virtual uint8_t			OnLoadPpu() { return 0; };
	virtual uint8_t			ReadRom( const uint16_t addr ) const = 0;
	virtual uint8_t			ReadChrRom( const uint16_t addr ) const { return 0; };
	virtual uint8_t			WriteChrRam( const uint16_t addr, const uint8_t value ) { return 0; };
	virtual uint8_t			Write( const uint16_t addr, const uint8_t value ) { return 0; };
	virtual bool			InWriteWindow( const uint16_t addr, const uint16_t offset ) const { return false; };

	virtual void			Serialize( Serializer& serializer ) {};
	virtual void			Clock() {};
};


class wtCart
{
private:
	uint8_t* rom;
	size_t					size;
	size_t					prgSize;
	size_t					chrSize;

public:
	wtRomHeader				h;
	shared_ptr<wtMapper>	mapper;

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
		if ( rom != nullptr )
		{
			delete[] rom;
			rom = nullptr;
		}
		size = 0;
	}

	uint8_t* GetPrgRomBank( const uint32_t bankNum, const uint32_t bankSize = KB( 16 ) )
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

	uint8_t* GetChrRomBank( const uint32_t bankNum, const uint32_t bankSize = KB( 4 ) )
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