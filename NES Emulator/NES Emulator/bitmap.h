//////////////////////////////////////////////////////////
//
//
//Last Modified:January 30 2012
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

namespace libBitmap
{
	typedef char BYTE;
};

typedef unsigned int uint32;
typedef unsigned short uint16;

class Bitmap  {
	public:
		Bitmap(){}
		Bitmap(const string& filename){
			load(filename);
			//Bitmap_Loader loader = new Bitmap_Loader();
			//mapdata = loader.load();
			//delete loader;

		}
		Bitmap(Bitmap& bitmap){
		
			bh	= bitmap.bh;
			bhi = bitmap.bhi;
			bmg	= bitmap.bmg;
			pixel_n = bitmap.pixel_n;

			mapdata = new RGBA[pixel_n];
			for(size_t i(0); i < pixel_n; ++i){

				mapdata[i] = bitmap.mapdata[i];
			}
		
		}
		Bitmap(size_t width, size_t height, unsigned int color = 0xFFFFFFFF){
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
			
			for(size_t i(0); i < pixel_n; ++i){
				
				mapdata[i].alpha	= (libBitmap::BYTE)(color & 0x000000FF);
				mapdata[i].blue		= (libBitmap::BYTE)((color & 0x0000FF00) >> 8);
				mapdata[i].green	= (libBitmap::BYTE)((color & 0x00FF0000) >> 16);
				mapdata[i].red		= (libBitmap::BYTE)((color & 0xFF000000) >> 24);
			}
		}
		~Bitmap(){
			delete[] mapdata;
		}
		Bitmap& operator=(const Bitmap& bitmap){
		
			return *this;
		
		}
	
		void load(const string& filename);
		void write(const string& filename);
		void printDataFile();
		inline uint32 getSize(){
			return bhi.imageSize;
		}
		inline uint32 getWidth(){
			return bhi.width;
		}
		inline uint32 getHeight(){
			return bhi.height;
		}
		void deleteRow(uint32 row){
			int col(bhi.width*row);
			for(uint32 i(0); i<bhi.width; ++i){
				mapdata[i+col].red = '\0';
				mapdata[i+col].green = '\0';
				mapdata[i+col].blue = '\0';
			} 
		}
		void invert(){
			for(uint32 i(0); i< bhi.imageSize; ++i){
				mapdata[i].red = ~mapdata[i].red;
				mapdata[i].green = ~mapdata[i].green;
				mapdata[i].blue = ~mapdata[i].blue;
			}
		}
		void clearImage(){
			for(register uint32 i(0); i< bhi.imageSize; ++i){
				mapdata[i].red = '\0';
				mapdata[i].green = '\0';
				mapdata[i].blue = '\0';
			}
		
		}
		inline void deleteColume(){
		}
		uint32 getPixel(uint32 x, uint32 y);
		bool setPixel(uint32 x, uint32 y, uint32 color);

	private:

		struct BITMAP_MagicNum{
			libBitmap::BYTE magic_num[2];//2
		}bmg;
		struct BITMAP_Header{
			uint32 size;//4 bytes in file
			uint16 reserve1;//2
			uint16 reserve2;//2
			uint32 offset;//4

		}bh;
		struct BITMAP_Header_Info{
			uint32 hSize;//header size, 4
			uint32 width;//image width, 4
			uint32 height;//image height, 4
			uint16 cPlanes; //color planes, 2
			uint16 bpPixels;//bits per pixel, 2
			uint32 compression;//4
			uint32 imageSize;//4
			uint32 hRes;//4
			uint32 vRes;//4
			uint32 colors;//4
			uint32 iColors;//4
		}bhi;
		struct RGBA{
			libBitmap::BYTE blue;
			libBitmap::BYTE green;
			libBitmap::BYTE red;
			libBitmap::BYTE alpha;

		}*mapdata;//actual bitmap data
		//
		ifstream instream;
		ofstream outstream;
		size_t pixel_n;
		uint32 readInt(uint32 bytes);
		void writeInt(uint32, int length);
		
};

#endif