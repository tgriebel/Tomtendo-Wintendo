#include "stdafx.h"
#include"bitmap.h"


Bitmap::Bitmap( const string& filename )
{
	Load( filename );
}


Bitmap::~Bitmap()
{
	delete[] mapdata;
}


Bitmap::Bitmap( const Bitmap& bitmap )
{
	bh	= bitmap.bh;
	bhi	= bitmap.bhi;
	bmg	= bitmap.bmg;

	pixelsNum = bitmap.pixelsNum;

	mapdata = new RGBA[pixelsNum];

	for ( uint32_t i = 0; i < pixelsNum; ++i )
	{
		mapdata[i] = bitmap.mapdata[i];
	}
}


Bitmap::Bitmap( const uint32_t width, const uint32_t height, const uint32_t color )
{
	pixelsNum = ( width * height );

	mapdata = new RGBA[pixelsNum];

	bmg.magic_num[0] = 'B';
	bmg.magic_num[1] = 'M';

	bh.size			= ( pixelsNum * 4 );
	bh.reserve1		= 0;
	bh.reserve2		= 0;
	bh.offset		= 54;

	bhi.hSize		= 40;
	bhi.width		= width;
	bhi.height		= height;
	bhi.cPlanes		= 1;
	bhi.bpPixels	= 32;
	bhi.compression	= 0;
	bhi.imageSize	= ( bhi.bpPixels * pixelsNum );
	bhi.hRes		= 0;
	bhi.vRes		= 0;
	bhi.colors		= 0;
	bhi.iColors		= 0;

	for ( size_t i = 0; i < pixelsNum; ++i )
	{
		mapdata[i].alpha	= static_cast<uint8_t>( color & 0x000000FF );
		mapdata[i].blue		= static_cast<uint8_t>( ( color & 0x0000FF00 ) >> 8 );
		mapdata[i].green	= static_cast<uint8_t>( ( color & 0x00FF0000 ) >> 16 );
		mapdata[i].red		= static_cast<uint8_t>( ( color & 0xFF000000 ) >> 24 );
	}
}


Bitmap& Bitmap::operator=( const Bitmap& bitmap )
{
	return *this;
}


template<int BYTES>
void Bitmap::WriteInt( uint32 value )
{
	uint8_t bin[BYTES];

	const int cbytes = 8;
	uint32 off = 0;
	
	uint32	temp = value;

	for( uint32 i = 0; i < BYTES; ++i )
	{
		temp >>= ( i * cbytes );
		bin[i] = static_cast< uint8_t >( temp );	
	}

	outstream.write( reinterpret_cast<char*>( bin ), BYTES );
}


template<int BYTES>
uint32 Bitmap::ReadInt()
{
	uint8_t bin[BYTES];

	instream.read( reinterpret_cast<char*>( bin ), BYTES );

	const int cbyte = 8;
	uint32 numeral = 0;

	for( uint32 i = 1; i <= BYTES; ++i )
	{
		uint32 temp = static_cast<uint32>( bin[ BYTES - i ] );
		temp &= 0x000000FF;
		temp <<=  ( BYTES - i ) * cbyte;
		numeral |= temp;
	}

	return numeral;
}


void Bitmap::Load( const string& filename )
{
	instream.open( filename.c_str(), ios::in | ios::binary );
	
	instream.read( reinterpret_cast<char*>( bmg.magic_num ), 2 );

	bh.size			= ReadInt<4>();
	bh.reserve1		= ReadInt<2>();
	bh.reserve2		= ReadInt<2>();
	bh.offset		= ReadInt<4>();

	bhi.hSize		= ReadInt<4>();
	bhi.width		= ReadInt<4>();
	bhi.height		= ReadInt<4>();
	bhi.cPlanes		= ReadInt<2>();
	bhi.bpPixels	= ReadInt<2>();
	bhi.compression	= ReadInt<4>();
	bhi.imageSize	= ReadInt<4>();
	bhi.hRes		= ReadInt<4>();
	bhi.vRes		= ReadInt<4>();
	bhi.colors		= ReadInt<4>();
	bhi.iColors		= ReadInt<4>();

	uint32 padding = 0;

	if( ( bhi.width % 4 ) != 0 )
	{
		padding = 4 - ( bhi.width % 4 );
	}

	instream.seekg( bh.offset, ios_base::beg );

	uint32 bytes = bhi.imageSize;

	mapdata = new RGBA[bytes];

	uint32 pixelBytes = ( bhi.bpPixels / 8 );
	uint32 lineBytes = ( pixelBytes * bhi.width );
	
	for( uint32 row = 0; row < bhi.height; ++row )
	{
		uint8_t* buffer = new uint8_t[lineBytes];

		instream.read( reinterpret_cast<char*>( buffer ), lineBytes );

		for(uint32 pixNum = 0; pixNum < ( bhi.width* pixelBytes ); pixNum += pixelBytes )
		{
			RGBA pixel;
			//parse raw data width/24bits
			pixel.blue		= buffer[pixNum];
			pixel.green		= buffer[pixNum + 1];
			pixel.red		= buffer[pixNum + 2];
			pixel.alpha		= (uint8_t)0xFF;
			
			mapdata[ row * bhi.width + ( pixNum / pixelBytes ) ] = pixel;
		}

		instream.seekg(padding+instream.tellg(), ios_base::beg);

		delete[] buffer;
		buffer = nullptr;
	}

	instream.close();
}


void Bitmap::Write( const string& filename )
{
	outstream.open( filename.c_str(), ios::out | ios::binary | ios::trunc );

	outstream.write( reinterpret_cast<char*>( bmg.magic_num ), 2 );

	WriteInt<4>( bh.size );
	WriteInt<2>( bh.reserve1 );
	WriteInt<2>( bh.reserve2 );
	WriteInt<4>( bh.offset );

	WriteInt<4>( bhi.hSize );
	WriteInt<4>( bhi.width );
	WriteInt<4>( bhi.height );
	WriteInt<2>( bhi.cPlanes);
	WriteInt<2>( bhi.bpPixels);
	WriteInt<4>( bhi.compression );
	WriteInt<4>( bhi.imageSize );
	WriteInt<4>( bhi.hRes );
	WriteInt<4>( bhi.vRes );
	WriteInt<4>( bhi.colors );
	WriteInt<4>( bhi.iColors );

	outstream.seekp( bh.offset, ios_base::beg );

	int padding = 0;

	if ( ( bhi.width % 4 ) != 0 )
	{
		padding = 4 - ( bhi.width % 4 );
	}

	uint8_t pad[4] = { '\0','\0', '\0','\0' };

	uint32 pixelBytes = ( bhi.bpPixels / 8 );
	uint32 lineBytes = ( pixelBytes * bhi.width ) + 1; // bytes per line, w/o padding

	uint8_t* buffer = new uint8_t[lineBytes];

	for( uint32 i = 0; i < bhi.height; ++i )
	{
		for( uint32 pix_num = 0, buffer_i = 0; 
			pix_num < bhi.width; buffer_i += pixelBytes, pix_num++ )
		{
			buffer[buffer_i]		=  mapdata[ pix_num + ( i * bhi.width ) ].red;
			buffer[buffer_i + 1]	=  mapdata[ pix_num + ( i * bhi.width ) ].green;
			buffer[buffer_i + 2]	=  mapdata[ pix_num + ( i * bhi.width ) ].blue;
			buffer[buffer_i + 3]	= 0xFF;
		}

		outstream.write( reinterpret_cast<char*>( buffer ), lineBytes - 1 );
		outstream.write( reinterpret_cast<char*>( pad ), padding);
	}

	delete[] buffer;
	buffer = NULL;

	outstream.close();
}


uint32 Bitmap::GetSize()
{
	return bhi.imageSize;
}


uint32 Bitmap::GetWidth()
{
	return bhi.width;
}


uint32 Bitmap::GetHeight()
{
	return bhi.height;
}


void Bitmap::DeleteRow( const uint32 row )
{
	int col = ( bhi.width * row );

	for ( uint32 i = 0; i < bhi.width; ++i )
	{
		mapdata[i + col].red	= 0;
		mapdata[i + col].green	= 0;
		mapdata[i + col].blue	= 0;
	}
}


void Bitmap::Invert()
{
	for ( uint32 i = 0; i < bhi.imageSize; ++i )
	{
		mapdata[i].red		= ~mapdata[i].red;
		mapdata[i].green	= ~mapdata[i].green;
		mapdata[i].blue		= ~mapdata[i].blue;
	}
}


void Bitmap::ClearImage()
{
	for ( uint32 i = 0; i < bhi.imageSize; ++i )
	{
		mapdata[i].red = 0;
		mapdata[i].green = 0;
		mapdata[i].blue = 0;
	}
}


void Bitmap::GetBuffer( uint32_t buffer[] )
{
	for( uint32_t i = 0; i < pixelsNum; ++i )
	{
		Pixel pixel;
		CopyToPixel( mapdata[i], pixel, BITMAP_BGRA );

		buffer[i] = pixel.rawABGR;
	}
}


bool Bitmap::SetPixel( const uint32 x, const uint32 y, const uint32 color )
{
	if( ( ( x >= bhi.height ) && ( x < 0 ) ) || ( ( y >= bhi.width ) && ( y < 0 ) ) )
	{
		return false;
	}
	else
	{
		mapdata[ y * bhi.width + x ].alpha	= color & 0x000000FF;
		mapdata[ y * bhi.width + x ].blue	= ( color & 0x0000FF00 ) >> 8;
		mapdata[ y * bhi.width + x ].green	= ( color & 0x00FF0000 ) >> 16;
		mapdata[ y * bhi.width + x ].red	= ( color & 0xFF000000 ) >> 24;

		return true;
	}
}


uint32 Bitmap::GetPixel( const uint32 x, const uint32 y )
{	
	if( ( x > bhi.height && x < 0 ) || ( y > bhi.width && y < 0 ) )
	{
		return 0;
	}
	else
	{
		uint32 mask			= 0x000000FF;
		uint32 colorValue	= 0x00000000;

		uint32 blue  = static_cast<uint32>( mapdata[ y * bhi.width + x ].blue & mask );
		uint32 green = static_cast<uint32>( mapdata[ y * bhi.width + x ].green & mask );
		uint32 red   = static_cast<uint32>( mapdata[ y * bhi.width + x ].red & mask );
		uint32 alpha = static_cast<uint32>( mapdata[ y * bhi.width + x ].alpha & mask );

		colorValue |= ( red <<= 24 ) | ( green <<= 16 ) | ( blue <<= 8 ) | ( alpha );

		return colorValue;
	}
}


void Bitmap::CopyToPixel( const RGBA& rgba, Pixel& pixel, BitmapFormat format )
{
	switch( format )
	{
		case BITMAP_ABGR:
		{
			pixel.vec[0] = rgba.alpha;
			pixel.vec[1] = rgba.blue;
			pixel.vec[2] = rgba.green;
			pixel.vec[3] = rgba.red;
		}
		break;

		case BITMAP_ARGB:
		{
			pixel.vec[0] = rgba.alpha;
			pixel.vec[1] = rgba.red;
			pixel.vec[2] = rgba.green;
			pixel.vec[3] = rgba.blue;
		}
		break;

		case BITMAP_BGRA:
		{
			pixel.vec[0] = rgba.blue;
			pixel.vec[1] = rgba.green;
			pixel.vec[2] = rgba.red;
			pixel.vec[3] = rgba.alpha;
		}
		break;

		default:
		case BITMAP_RGBA:
		{
			pixel.vec[0] = rgba.red;
			pixel.vec[1] = rgba.green;
			pixel.vec[2] = rgba.blue;
			pixel.vec[3] = rgba.alpha;
		}
		break;
	}
}