#pragma once

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
	byte numBanks;
	byte reserved[7];
};


struct NesCart
{
	iNesHeader	header;
	byte		rom[65536];
	size_t		size;
};