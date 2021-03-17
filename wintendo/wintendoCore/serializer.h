#pragma once
#include <stdio.h>
#include <string>
#include <sstream>

#define DBG_SERIALIZER 0

enum class serializeMode_t
{
	STORE,
	LOAD,
};

union serializerTuple_t
{
	struct b8_t
	{
		uint8_t		e[8];
	} b8;

	struct b16_t
	{
		uint16_t	e[4];
	} b16;

	struct b32_t
	{
		uint32_t	e[2];
	} b32;

	uint64_t b64;
};

class Serializer
{
public:
	
	Serializer( const uint32_t _sizeInBytes, serializeMode_t _mode )
	{
		bytes = new uint8_t[ _sizeInBytes ];
		byteCount = _sizeInBytes;
		mode = _mode;
		Clear();
	}

	~Serializer()
	{
		if( bytes != nullptr ) {
			delete[] bytes;
		}
		byteCount = 0;
		mode = serializeMode_t::LOAD;
		SetPosition( 0 );
	}

	Serializer() = delete;
	Serializer( const Serializer& ) = delete;
	Serializer operator=( const Serializer& ) = delete;

	uint8_t*			GetPtr();
	void				SetPosition( const uint32_t index );
	void				Clear();
	uint32_t			CurrentSize() const;
	uint32_t			BufferSize() const;
	bool				CanStore( const uint32_t sizeInBytes ) const;
	void				SetMode( serializeMode_t mode );
	serializeMode_t		GetMode() const;

	bool				NextBool( bool& v );
	bool				NextChar( int8_t& v );
	bool				NextUchar( uint8_t& v );
	bool				NextShort( int16_t& v );
	bool				NextUshort( uint16_t& v );
	bool				NextInt( int32_t& v );
	bool				NextUint( uint32_t& v );
	bool				NextLong( int64_t& v );
	bool				NextUlong( uint64_t& v );
	bool				NextFloat( float& v );
	bool				NextDouble( double& v );

	bool				Next8b( uint8_t& b8 );
	bool				Next16b( uint16_t& b16 );
	bool				Next32b( uint32_t& b32 );
	bool				Next64b( uint64_t& b64 );
	bool				NextArray( uint8_t* b8, uint32_t sizeInBytes );

private:
	uint8_t*		bytes;
	uint32_t		byteCount;
	uint32_t		index;
	serializeMode_t mode;
public: // FIXME: temp
	std::stringstream dbgText;
};