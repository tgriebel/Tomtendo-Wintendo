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

inline constexpr uint8_t operator "" _b( uint64_t arg ) noexcept
{
	return static_cast< uint8_t >( arg & 0xFF );
}

const uint64_t MasterClockHz		= 21477272;
const uint64_t CpuClockDivide		= 12;
const uint64_t PpuClockDivide		= 4;
const uint64_t PpuCyclesPerScanline	= 341;
const uint64_t FPS					= 60;

using masterCycles_t = std::chrono::duration< uint64_t, std::ratio<1, MasterClockHz> >;
using ppuCycle_t = std::chrono::duration< uint64_t, std::ratio<PpuClockDivide, MasterClockHz> >;
using cpuCycle_t = std::chrono::duration< uint64_t, std::ratio<CpuClockDivide, MasterClockHz> >;
using scanCycle_t = std::chrono::duration< uint64_t, std::ratio<PpuCyclesPerScanline * PpuClockDivide, MasterClockHz> >; // TODO: Verify
using frameRate_t = std::chrono::duration< double, std::ratio<1, FPS> >;

struct iNesHeader
{
	uint8_t type[3];
	uint8_t magic;
	uint8_t prgRomBanks;
	uint8_t chrRomBanks;
	struct ControlsBits0
	{
		uint8_t mirror : 1;
		uint8_t usesBattery : 1;
		uint8_t usesTrainer : 1;
		uint8_t fourScreenMirror : 1;
		uint8_t mapperNumberLower : 4;
	} controlBits0;
	struct ControlsBits1
	{
		uint8_t reserved0 : 4;
		uint8_t mappedNumberUpper : 4;
	} controlBits1;
	uint8_t reserved[8];
};


struct NesCart
{
	iNesHeader	header;
	uint8_t		rom[524288];
	size_t		size;
};


struct WtPoint
{
	uint32_t x;
	uint32_t y;
};


struct WtRect
{
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
};


static void LoadNesFile( const std::string& fileName, NesCart& outCart )
{
	std::ifstream nesFile;
	nesFile.open( fileName, std::ios::binary );

	assert( nesFile.good() );

	nesFile.seekg( 0, std::ios::end );
	size_t len = static_cast<size_t>( nesFile.tellg() );

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


inline uint16_t Combine( const uint8_t lsb, const uint8_t msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}
#endif // __COMMONTYPES__