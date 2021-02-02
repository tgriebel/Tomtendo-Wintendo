#pragma once
#include <stdio.h>
#include <string>
#include <cstdint>

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
	
	Serializer( const uint8_t sizeInBytes )
	{
		bytes = new uint8_t[ sizeInBytes ];
		byteCount = sizeInBytes;
		index = 0;
	}

	~Serializer()
	{
		delete[] bytes;
		byteCount = 0;
		index = 0;
	}

	Serializer() = delete;
	Serializer( const Serializer& ) = delete;
	Serializer operator=( const Serializer& ) = delete;

	uint8_t*	GetPtr();
	uint32_t	CurrentSize() const;
	uint32_t	BufferSize() const;
	bool		CanStore( const uint32_t sizeInBytes ) const;
	bool		Next8b( uint8_t& b8, serializeMode_t mode );
	bool		Next16b( uint16_t& b16, serializeMode_t mode );
	bool		Next32b( uint32_t& b32, serializeMode_t mode );
	bool		Next64b( uint64_t& b64, serializeMode_t mode );
	bool		NextArray( uint8_t* b8, uint32_t sizeInBytes, serializeMode_t mode );

private:
	uint8_t*	bytes;
	uint32_t	byteCount;
	uint32_t	index;
};