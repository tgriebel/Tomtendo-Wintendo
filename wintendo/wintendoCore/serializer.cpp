#include "serializer.h"

uint8_t* Serializer::GetPtr()
{
	return bytes;
}


void Serializer::Reset()
{
	index = 0;
}


uint32_t Serializer::CurrentSize() const
{
	return index;
}


uint32_t Serializer::BufferSize() const
{
	return byteCount;
}


bool Serializer::CanStore( const uint32_t sizeInBytes ) const
{
	return ( CurrentSize() + sizeInBytes <= BufferSize() );
}


bool Serializer::Next8b( uint8_t& b8, serializeMode_t mode )
{
	const uint32_t size = sizeof( b8 );
	if ( !CanStore( size ) ) {
		return false;
	}

	if ( mode == serializeMode_t::LOAD ) {
		b8 = bytes[ index ];
	}
	else {
		bytes[ index ] = b8;
	}
	index += size;

	return true;
}


bool Serializer::Next16b( uint16_t& b16, serializeMode_t mode )
{
	const uint32_t size = sizeof( b16 );
	if ( !CanStore( size ) ) {
		return false;
	}

	serializerTuple_t t = {};
	if ( mode == serializeMode_t::LOAD )
	{
		t.b8.e[ 0 ] = bytes[ index + 0 ];
		t.b8.e[ 1 ] = bytes[ index + 1 ];
		b16 = t.b16.e[ 0 ];
	}
	else
	{
		t.b16.e[ 0 ] = b16;
		bytes[ index + 0 ] = t.b8.e[ 0 ];
		bytes[ index + 1 ] = t.b8.e[ 1 ];
	}
	index += size;

	return true;
}


bool Serializer::Next32b( uint32_t& b32, serializeMode_t mode )
{
	const uint32_t size = sizeof( b32 );
	if ( !CanStore( size ) ) {
		return false;
	}

	serializerTuple_t t = {};
	if ( mode == serializeMode_t::LOAD )
	{
		t.b8.e[ 0 ] = bytes[ index + 0 ];
		t.b8.e[ 1 ] = bytes[ index + 1 ];
		t.b8.e[ 2 ] = bytes[ index + 2 ];
		t.b8.e[ 3 ] = bytes[ index + 3 ];
		b32 = t.b32.e[ 0 ];
	}
	else
	{
		t.b32.e[ 0 ] = b32;
		bytes[ index + 0 ] = t.b8.e[ 0 ];
		bytes[ index + 1 ] = t.b8.e[ 1 ];
		bytes[ index + 2 ] = t.b8.e[ 2 ];
		bytes[ index + 3 ] = t.b8.e[ 3 ];
	}
	index += size;

	return true;
}


bool Serializer::Next64b( uint64_t& b64, serializeMode_t mode )
{
	const uint32_t size = sizeof( b64 );
	if ( !CanStore( size ) ) {
		return false;
	}

	serializerTuple_t t = {};
	if ( mode == serializeMode_t::LOAD )
	{
		t.b8.e[ 0 ] = bytes[ index + 0 ];
		t.b8.e[ 1 ] = bytes[ index + 1 ];
		t.b8.e[ 2 ] = bytes[ index + 2 ];
		t.b8.e[ 3 ] = bytes[ index + 3 ];
		t.b8.e[ 4 ] = bytes[ index + 4 ];
		t.b8.e[ 5 ] = bytes[ index + 5 ];
		t.b8.e[ 6 ] = bytes[ index + 6 ];
		t.b8.e[ 7 ] = bytes[ index + 7 ];
		b64 = t.b64;
	}
	else
	{
		t.b64 = b64;
		bytes[ index + 0 ] = t.b8.e[ 0 ];
		bytes[ index + 1 ] = t.b8.e[ 1 ];
		bytes[ index + 2 ] = t.b8.e[ 2 ];
		bytes[ index + 3 ] = t.b8.e[ 3 ];
		bytes[ index + 4 ] = t.b8.e[ 4 ];
		bytes[ index + 5 ] = t.b8.e[ 5 ];
		bytes[ index + 6 ] = t.b8.e[ 6 ];
		bytes[ index + 7 ] = t.b8.e[ 7 ];
	}
	index += size;

	return true;
}


bool Serializer::NextArray( uint8_t* b8, uint32_t sizeInBytes, serializeMode_t mode )
{
	if ( !CanStore( sizeInBytes ) ) {
		return false;
	}

	if ( mode == serializeMode_t::LOAD ) {
		memcpy( b8, bytes + index, sizeInBytes );
	}
	else {
		memcpy( bytes + index, b8, sizeInBytes );
	}
	index += sizeInBytes;

	return true;
}