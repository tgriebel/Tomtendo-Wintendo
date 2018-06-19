#ifndef __COMMONTYPES__
#define __COMMONTYPES__

#include <fstream>
#include <string>

#define NES_MODE 1
#define DEBUG_MODE 1
#define DEBUG_ADDR 1
#define DEBUG_MEM 1

typedef unsigned short	ushort;
typedef unsigned int	uint;

typedef unsigned char	byte;
typedef signed char		byte_signed;
typedef ushort			half;
typedef uint			word;


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