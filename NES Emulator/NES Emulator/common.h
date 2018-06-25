#ifndef __COMMONTYPES__
#define __COMMONTYPES__

#include <string>
#include <chrono>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <iomanip>

#define NES_MODE 1
#define DEBUG_MODE 1
#define DEBUG_ADDR 1
#define DEBUG_MEM 0

typedef unsigned short	ushort;
typedef unsigned int	uint;

typedef unsigned char	byte;
typedef signed char		byte_signed;
typedef ushort			half;
typedef uint			word;

const uint64_t MasterClockHz	= 21477272;
const uint64_t CpuClockDivide	= 12;
const uint64_t PpuClockDivide	= 4;

using masterCycles_t = std::chrono::duration< uint64_t, std::ratio<1, MasterClockHz> >;
using ppuCycle_t = std::chrono::duration< uint64_t, std::ratio<4, MasterClockHz> >;
using cpuCycle_t = std::chrono::duration< uint64_t, std::ratio<12, MasterClockHz> >;
using frameRate_t = std::chrono::duration< uint64_t, std::ratio<1, 60> >;

struct iNesHeader
{
	byte type[3];
	byte magic;
	byte prgRomBanks;
	byte chrRomBanks;
	struct
	{
		byte mirror : 1;
		byte usesBattery : 1;
		byte usesTrainer : 1;
		byte fourScreenMirror : 1;
		byte mapperNumberLower : 4;
	} controlBits0;
	struct
	{
		byte reserved0 : 4;
		byte mappedNumberUpper : 4;
	} controlBits1;
	byte reserved[8];
};


struct NesCart
{
	iNesHeader	header;
	byte		rom[524288];
	size_t		size;
};


static void LoadNesFile( const std::string& fileName, NesCart& outCart )
{
	std::ifstream nesFile;
	nesFile.open( fileName, std::ios::binary );
	nesFile.seekg( 0, std::ios::end );
	size_t len = nesFile.tellg();

	nesFile.seekg( 0, std::ios::beg );
	nesFile.read( reinterpret_cast<char*>( &outCart ), len );
	nesFile.close();

	assert( outCart.header.type[0] == 'N' );
	assert( outCart.header.type[1] == 'E' );
	assert( outCart.header.type[2] == 'S' );
	assert( outCart.header.magic == 0x1A );

	std::ofstream checkFile;
	checkFile.open( "checkFile.nes", std::ios::binary );
	checkFile.write( reinterpret_cast<char*>( &outCart ), len );
	checkFile.close();
	

	outCart.size = len - sizeof( outCart.header ); // TODO: trainer needs to be checked
}


inline half Combine( const byte lsb, const byte msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}
#endif // __COMMONTYPES__