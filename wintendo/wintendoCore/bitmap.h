/*
* MIT License
*
* Copyright( c ) 2009-2021 Thomas Griebel
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this softwareand associated documentation files( the "Software" ), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright noticeand this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

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
	uint8_t vec[4];
	uint32_t rawABGR; // ABGR format
	RGBA rgba;

	inline uint32_t AsHexColor()
	{
		Pixel abgr;
		abgr.rgba = { rgba.alpha, rgba.red, rgba.green, rgba.blue };
		return abgr.rawABGR;
	}
};


enum BitmapFormat : uint32_t
{
	BITMAP_ABGR = 0,
	BITMAP_ARGB = 1,
	BITMAP_BGRA = 2,
	BITMAP_RGBA = 3,
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