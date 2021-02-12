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
	
	Serializer( const uint32_t sizeInBytes )
	{
		bytes = new uint8_t[ sizeInBytes ];
		byteCount = sizeInBytes;
		Clear();
	}

	~Serializer()
	{
		if( bytes != nullptr ) {
			delete[] bytes;
		}
		byteCount = 0;
		SetPosition( 0 );
	}

	Serializer() = delete;
	Serializer( const Serializer& ) = delete;
	Serializer operator=( const Serializer& ) = delete;

	uint8_t*	GetPtr();
	void		SetPosition( const uint32_t index );
	void		Clear();
	uint32_t	CurrentSize() const;
	uint32_t	BufferSize() const;
	bool		CanStore( const uint32_t sizeInBytes ) const;

	bool		NextBool( bool& v, serializeMode_t mode );
	bool		NextChar( int8_t& v, serializeMode_t mode );
	bool		NextUchar( uint8_t& v, serializeMode_t mode );
	bool		NextShort( int16_t& v, serializeMode_t mode );
	bool		NextUshort( uint16_t& v, serializeMode_t mode );
	bool		NextInt( int32_t& v, serializeMode_t mode );
	bool		NextUint( uint32_t& v, serializeMode_t mode );
	bool		NextLong( int64_t& v, serializeMode_t mode );
	bool		NextUlong( uint64_t& v, serializeMode_t mode );
	bool		NextFloat( float& v, serializeMode_t mode );
	bool		NextDouble( double& v, serializeMode_t mode );

	bool		Next8b( uint8_t& b8, serializeMode_t mode );
	bool		Next16b( uint16_t& b16, serializeMode_t mode );
	bool		Next32b( uint32_t& b32, serializeMode_t mode );
	bool		Next64b( uint64_t& b64, serializeMode_t mode );
	bool		NextArray( uint8_t* b8, uint32_t sizeInBytes, serializeMode_t mode );

private:
	uint8_t*	bytes;
	uint32_t	byteCount;
	uint32_t	index;
public: // FIXME: temp
	std::stringstream dbgText;
};