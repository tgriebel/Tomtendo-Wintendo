#include "serializer.h"
#include "assert.h"

uint8_t* Serializer::GetPtr()
{
	return bytes;
}


void Serializer::SetPosition( const uint32_t index )
{
	this->index = 0;
	dbgText.str( "" );
}


void Serializer::Clear()
{
	memset( bytes, 0, CurrentSize() );
	SetPosition( 0 );
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


void Serializer::SetMode( serializeMode_t serializeMode )
{
	mode = serializeMode;
}


serializeMode_t Serializer::GetMode() const
{
	return mode;
}


uint32_t Serializer::NewLabel( const char name[ serializerHeader_t::MaxNameLength ] )
{
	const uint32_t sectionIx = header.sectionCount;

	serializerHeader_t::section_t& section = header.sections[ sectionIx ];
	section.offset = index;
	strcpy_s< serializerHeader_t::MaxNameLength >( section.name, name );
	++header.sectionCount;

	return sectionIx;
}


void Serializer::EndLabel( const char name[ serializerHeader_t::MaxNameLength ] )
{
	serializerHeader_t::section_t* section;
	if( FindLabel( name, &section ) )
	{
		section->size = ( index - section->offset );
	}
}


bool Serializer::FindLabel( const char name[ serializerHeader_t::MaxNameLength ], serializerHeader_t::section_t** outSection )
{
	for( uint32_t i = 0; i < header.sectionCount; ++i )
	{
		serializerHeader_t::section_t& section = header.sections[ i ];
		if( _strnicmp( name, section.name, serializerHeader_t::MaxNameLength ) == 0 )
		{
			*outSection = &section;
			return true;
		}
	}

	*outSection = nullptr;
	return false;
}


bool Serializer::NextBool( bool& v)
{
	return Next8b( *reinterpret_cast<uint8_t*>( &v ) );
}

bool Serializer::NextChar( int8_t& v )
{
	return Next8b( *reinterpret_cast<uint8_t*>( &v ) );
}

bool Serializer::NextUchar( uint8_t& v )
{
	return Next8b( v );
}

bool Serializer::NextShort( int16_t& v )
{
	return Next16b( *reinterpret_cast<uint16_t*>( &v ) );
}

bool Serializer::NextUshort( uint16_t& v )
{
	return Next16b( v );
}

bool Serializer::NextInt( int32_t& v )
{
	return Next32b( *reinterpret_cast<uint32_t*>( &v ) );
}

bool Serializer::NextUint( uint32_t& v )
{
	return Next32b( v );
}

bool Serializer::NextLong( int64_t& v )
{
	return Next64b( *reinterpret_cast<uint64_t*>( &v ) );
}

bool Serializer::NextUlong( uint64_t& v )
{
	return Next64b( v );
}

bool Serializer::NextFloat( float& v )
{
	return Next32b( *reinterpret_cast<uint32_t*>( &v ) );
}

bool Serializer::NextDouble( double& v )
{
	return Next64b( *reinterpret_cast<uint64_t*>( &v ) );
}


bool Serializer::Next8b( uint8_t& b8 )
{
	const uint32_t size = sizeof( b8 );
	if ( !CanStore( size ) ) {
		assert( 0 ); // TODO: remove
		return false;
	}

	if ( mode == serializeMode_t::LOAD ) {
		b8 = bytes[ index ];
	}
	else {
		bytes[ index ] = b8;
	}
	index += size;

#if DBG_SERIALIZER
	dbgText << (int)b8 << "\n";
#endif

	return true;
}


bool Serializer::Next16b( uint16_t& b16 )
{
	const uint32_t size = sizeof( b16 );
	if ( !CanStore( size ) ) {
		assert( 0 ); // TODO: remove
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

#if DBG_SERIALIZER
	dbgText << b16 << "\n";
#endif

	return true;
}


bool Serializer::Next32b( uint32_t& b32 )
{
	const uint32_t size = sizeof( b32 );
	if ( !CanStore( size ) ) {
		assert( 0 ); // TODO: remove
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

#if DBG_SERIALIZER
	dbgText << b32 << "\n";
#endif

	return true;
}


bool Serializer::Next64b( uint64_t& b64 )
{
	const uint32_t size = sizeof( b64 );
	if ( !CanStore( size ) ) {
		assert( 0 ); // TODO: remove
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

#if DBG_SERIALIZER
	dbgText << b64 << "\n";
#endif

	return true;
}


bool Serializer::NextArray( uint8_t* b8, uint32_t sizeInBytes )
{
	if ( !CanStore( sizeInBytes ) ) {
		assert( 0 ); // TODO: remove
		return false;
	}

	if ( mode == serializeMode_t::LOAD ) {
	//	memcpy( b8, bytes + index, sizeInBytes );
		for ( uint32_t i = 0; i < sizeInBytes; ++i )
		{
			b8[ i ] = bytes[ index ];			
			dbgText << b8[ i ] << " ";
			++index;
		}
	} else {
	//	memcpy( bytes + index, b8, sizeInBytes );
		for( uint32_t i = 0; i < sizeInBytes; ++i )
		{
			bytes[ index ] = b8[ i ];
			dbgText << bytes[ index ] << " ";
			++index;
		}
	}
	//index += sizeInBytes;

	return true;
}