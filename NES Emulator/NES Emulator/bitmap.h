//////////////////////////////////////////////////////////
//
//
// Creation: October 2009
// Modified: July 2018
//
//
//
//
//////////////////////////////////////////////////////////
#ifndef _BITMAPCLASS_
#define _BITMAPCLASS_
#include<iostream>
#include<fstream>
using namespace std;


struct RGBA
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
};

typedef unsigned int uint32;
typedef unsigned short uint16;

union Pixel
{
	uint8_t vec[4]; // ABGR format
	uint32 raw;
};


class Bitmap
{
	public:
		Bitmap(){}

		Bitmap(const string& filename)
		{
			load(filename);
		}

		Bitmap(Bitmap& bitmap)
		{
			bh	= bitmap.bh;
			bhi = bitmap.bhi;
			bmg	= bitmap.bmg;
			pixel_n = bitmap.pixel_n;

			mapdata = new RGBA[pixel_n];

			for(size_t i = 0; i < pixel_n; ++i )
			{
				mapdata[i] = bitmap.mapdata[i];
			}
		}

		Bitmap( unsigned int width, unsigned int height, unsigned int color = 0xFFFFFFFF )
		{
			pixel_n = width * height;

			//finsih
			mapdata = new RGBA[pixel_n];

			bmg.magic_num[0] = 'B'; bmg.magic_num[1] = 'M';
			bh.size = pixel_n * 4;
			bh.reserve1 = 0;
			bh.reserve2 = 0;
			bh.offset = 54;
			//
			bhi.hSize = 40;
			bhi.width = width;
			bhi.height = height;
			bhi.cPlanes = 1;
			bhi.bpPixels = 32;
			bhi.compression = 0;
			bhi.imageSize = bhi.bpPixels * pixel_n;
			bhi.hRes = 0;
			bhi.vRes = 0;
			bhi.colors = 0;
			bhi.iColors = 0;
			
			for( size_t i(0); i < pixel_n; ++i )
			{
				mapdata[i].alpha	= (uint8_t)(color & 0x000000FF);
				mapdata[i].blue		= (uint8_t)((color & 0x0000FF00) >> 8);
				mapdata[i].green	= (uint8_t)((color & 0x00FF0000) >> 16);
				mapdata[i].red		= (uint8_t)((color & 0xFF000000) >> 24);
			}
		}

		~Bitmap()
		{
			delete[] mapdata;
		}

		Bitmap& operator=(const Bitmap& bitmap)
		{
			return *this;	
		}

		static inline void CopyToPixel( RGBA& rgba, Pixel& pixel )
		{
			pixel.vec[0] = rgba.alpha;
			pixel.vec[1] = rgba.blue;
			pixel.vec[2] = rgba.green;
			pixel.vec[3] = rgba.red;
		}
	
		void load(const string& filename);
		void write(const string& filename);
		void printDataFile();

		inline uint32 getSize()
		{
			return bhi.imageSize;
		}

		inline uint32 getWidth()
		{
			return bhi.width;
		}

		inline uint32 getHeight()
		{
			return bhi.height;
		}

		void deleteRow( uint32 row )
		{
			int col = bhi.width * row;

			for( uint32 i = 0; i<bhi.width; ++i )
			{
				mapdata[i+col].red = '\0';
				mapdata[i+col].green = '\0';
				mapdata[i+col].blue = '\0';
			} 
		}

		void invert()
		{
			for( uint32 i = 0; i< bhi.imageSize; ++i )
			{
				mapdata[i].red = ~mapdata[i].red;
				mapdata[i].green = ~mapdata[i].green;
				mapdata[i].blue = ~mapdata[i].blue;
			}
		}

		void clearImage()
		{
			for( uint32 i = 0; i < bhi.imageSize; ++i )
			{
				mapdata[i].red = '\0';
				mapdata[i].green = '\0';
				mapdata[i].blue = '\0';
			}
		}

		inline void deleteColumn()
		{
		}

		uint32 getPixel( uint32 x, uint32 y );
		bool setPixel( uint32 x, uint32 y, uint32 color );

	private:

		struct BITMAP_MagicNum
		{
			uint8_t magic_num[2];
		} bmg;

		struct BITMAP_Header
		{
			uint32 size;
			uint16 reserve1;
			uint16 reserve2;
			uint32 offset;
		}bh;

		struct BITMAP_Header_Info
		{
			uint32 hSize;
			uint32 width;
			uint32 height;
			uint16 cPlanes;
			uint16 bpPixels;
			uint32 compression;
			uint32 imageSize;
			uint32 hRes;
			uint32 vRes;
			uint32 colors;
			uint32 iColors;
		}bhi;

		RGBA* mapdata;
		//
		ifstream instream;
		ofstream outstream;
		uint32 pixel_n;
		uint32 readInt( uint32 bytes );
		void writeInt( uint32, int length );
};

#endif