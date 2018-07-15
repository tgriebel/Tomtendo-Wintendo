#include "stdafx.h"
#include"bitmap.h"

//REFACTOR: LOADER AND WRITER RESPONSIBLE FOR PRODUCING HEADER AND BITMAP FORMAT FROM RAW DATA
void Bitmap::writeInt(uint32 value, const int uint8_ts){

	uint8_t* bin = new uint8_t[uint8_ts];

	const int cuint8_t(8); //uint8_t constant
	uint32 off(0);
	
	uint32	temp = value;

	for(int i = 0; i<uint8_ts; i++){
		temp >>= i*cuint8_t;
		bin[i] = ( uint8_t)temp;
		
	}

	outstream.write( reinterpret_cast<char*>( bin ), uint8_ts);
	delete[] bin;
	bin = NULL;
	

}


uint32 Bitmap::readInt(const uint32 bytes )
{
	uint8_t* bin = new uint8_t[bytes];

	instream.read( reinterpret_cast<char*>( bin ), bytes );

	const int cbyte = 8;
	uint32 numeral = 0;

	for(uint32 i = 1; i <= bytes; ++i)
	{
		uint32 temp = (uint32)bin[ bytes - i ];
		temp &= 0x000000FF;
		temp <<=  ( bytes - i ) * cbyte;
		numeral |= temp;
	}

	delete[] bin;
	bin = NULL;
	return numeral;

}


void Bitmap::load( const string& filename )
{
	instream.open( filename.c_str(), ios::in | ios::binary );
	
	instream.read( reinterpret_cast<char*>( bmg.magic_num ), 2 );
	bh.size = readInt(4);
	bh.reserve1 = readInt(2);
	bh.reserve2 = readInt(2);
	bh.offset = readInt(4);
	//
	bhi.hSize = readInt(4);
	bhi.width = readInt(4);
	bhi.height = readInt (4);
	bhi.cPlanes = readInt(2);
	bhi.bpPixels = readInt(2);
	bhi.compression = readInt(4);
	bhi.imageSize = readInt(4);
	bhi.hRes = readInt(4);
	bhi.vRes = readInt(4);
	bhi.colors = readInt(4);
	bhi.iColors = readInt(4);
	//

	uint32 padding = 0;

	if( bhi.width % 4 != 0 )
	{
		padding = 4 - (bhi.width%4);
	}

	instream.seekg( bh.offset, ios_base::beg );

	uint32 bytes = bhi.imageSize;

	mapdata = new RGBA[bytes];
	uint32 pixelBytes = ( bhi.bpPixels / 8 );
	uint32 lineBytes = pixelBytes * bhi.width;
	
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
			
			mapdata[ row * bhi.width + ( pixNum/ pixelBytes ) ] = pixel;
		}

		instream.seekg(padding+instream.tellg(), ios_base::beg);

		delete[] buffer;
		buffer = NULL;
	}

	instream.close();
}


void Bitmap::write( const string& filename )
{
	outstream.open( filename.c_str(), ios::out | ios::binary );

	outstream.write( reinterpret_cast<char*>( bmg.magic_num ), 2 );

	writeInt( bh.size,			4 );
	writeInt( bh.reserve1,		2 );
	writeInt( bh.reserve2,		2 );
	writeInt( bh.offset,		4 );
	//
	writeInt( bhi.hSize,		4 );
	writeInt( bhi.width,		4 );
	writeInt( bhi.height,		4 );
	writeInt( bhi.cPlanes,		2 );
	writeInt( bhi.bpPixels,		2 );
	writeInt( bhi.compression,	4 );
	writeInt( bhi.imageSize,	4 );
	writeInt( bhi.hRes,			4 );
	writeInt( bhi.vRes,			4 );
	writeInt( bhi.colors,		4 );
	writeInt( bhi.iColors,		4 );

	outstream.seekp( bh.offset, ios_base::beg );

	int padding = 0;

	if ( ( bhi.width % 4 ) != 0 )
	{
		padding = 4 - ( bhi.width % 4 );
	}

	uint8_t pad[4] = { '\0','\0', '\0','\0' };

	uint32 pixelBytes = ( bhi.bpPixels / 8 );
	uint32 lineBytes = pixelBytes * bhi.width; // bytes per line, w/o padding

	uint8_t* buffer = new uint8_t[lineBytes];

	for( uint32 i = 0; i < bhi.height; ++i )
	{
		for( uint32 pix_num = 0, buffer_i = 0; 
			pix_num < bhi.width; buffer_i += pixelBytes, pix_num++ )
		{
			buffer[buffer_i]		=  mapdata[ pix_num + ( i * bhi.width ) ].blue;
			buffer[buffer_i + 1]	=  mapdata[ pix_num + ( i * bhi.width ) ].green;
			buffer[buffer_i + 2]	=  mapdata[ pix_num + ( i * bhi.width ) ].red;
		}

		outstream.write( reinterpret_cast<char*>( buffer ), lineBytes );
		outstream.write( reinterpret_cast<char*>( pad ), padding);
	}

	delete[] buffer;
	buffer = NULL;

	outstream.close();
}


void Bitmap::printDataFile(){

	ofstream infostream;
	infostream.open("Info.txt", ios::out);

	//infostream.write(bmg.magic_num, 2);
	infostream<<"Bitmap size: "<<bh.size<<endl;
	infostream<<"Reserve1: "<<bh.reserve1<<endl;
	infostream<<"Reserve2: "<<bh.reserve2<<endl;
	infostream<<"Offset: "<<bh.offset<<endl;
	//
	infostream<<"Header size: "<<bhi.hSize<<endl;
	infostream<<"Width: "<<bhi.width<<endl;
	infostream<<"Height: "<<bhi.height<<endl;
	infostream<<"Color planes: "<<bhi.cPlanes<<endl;
	infostream<<"Bits/Pixel: "<<bhi.bpPixels<<endl;
	infostream<<"Compression: "<<bhi.compression<<endl;
	infostream<<"ImageSize: "<<bhi.imageSize<<endl;
	infostream<<"Horizontal Resolution: "<<bhi.hRes<<endl;
	infostream<<"Vertical Resolution: "<<bhi.vRes<<endl;
	infostream<<"Color Palette (0 = default): "<<bhi.colors<<endl;
	infostream<<"Important Colors: "<<bhi.iColors<<endl;

	infostream.close();

}


bool Bitmap::setPixel(uint32 x, uint32 y, uint32 color)
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


uint32 Bitmap::getPixel(uint32 x, uint32 y){
		
	if( (x > bhi.height && x < 0) || (y > bhi.width && y < 0 ) )
	{
		return - 1;
	}
	else
	{
		uint32 mask = 0x000000FF;
		uint32 color_value = 0x00000000;
		uint32 blue  = (uint32)mapdata[ y * bhi.width + x ].blue & mask;
		uint32 green = (uint32)mapdata[ y * bhi.width + x ].green & mask;
		uint32 red   = (uint32)mapdata[ y * bhi.width + x ].red & mask;
		uint32 alpha = (uint32)mapdata[ y * bhi.width + x ].alpha & mask;

		color_value |= ( red <<= 24 ) | ( green <<= 16 ) | ( blue <<= 8 ) | ( alpha );
		return color_value;
	}
}