//////////////////////////////////////////////////////////
//
//
// Creation: October 2009
// Modified: July 2018
//
//
//////////////////////////////////////////////////////////
#pragma once

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


enum BitmapFormat : uint32_t
{
	BITMAP_ABGR = 0,
	BITMAP_BGRA = 1,
	BITMAP_RGBA = 2,
};


class Bitmap
{
	public:
		Bitmap() = delete;

		Bitmap( const string& filename );
		Bitmap( const Bitmap& bitmap );
		Bitmap( const uint32_t width, const uint32_t height, const uint32_t color = ~0x00 );
		~Bitmap();

		Bitmap& operator=( const Bitmap& bitmap );
		static void CopyToPixel( const RGBA& rgba, Pixel& pixel, BitmapFormat format );
	
		void Load( const string& filename );
		void Write( const string& filename );

		uint32 GetSize();
		uint32 GetWidth();
		uint32 GetHeight();

		void DeleteRow( const uint32 row );
		void Invert();
		void ClearImage();

		void GetBuffer( uint32_t buffer[] );

		uint32 GetPixel( const uint32 x, const uint32 y );
		bool SetPixel( const uint32 x, const uint32 y, const uint32 color );

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
		} bh;

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
		} bhi;

		RGBA* mapdata;

		ifstream instream;
		ofstream outstream;
		uint32 pixelsNum;

		template<int BYTES>
		uint32 ReadInt();

		template<int BYTES>
		void WriteInt( const uint32 value );
};