#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <assert.h>
#include <map>
#include <sstream>
#include "common.h"
#include "6502.h"
#include "debug.h"
#include "bitmap.h"


bool Load6502Program( Cpu6502& cpu, const string fileName, const half loadAddr, const half resetVector )
{
	ifstream programFile;
	programFile.open( fileName, ios::binary );
	std::string str( ( istreambuf_iterator< char >( programFile ) ), istreambuf_iterator< char >() );
	programFile.close();

	memset( cpu.memory, 0, Cpu6502::VirtualMemorySize );
	memcpy( cpu.memory + loadAddr, str.c_str(), str.size() );

	cpu.resetVector = resetVector;

	cpu.Reset();

	return true;
}


inline bool TestRegisters( const Cpu6502& cpu, const byte A, const byte X, const byte Y, const byte SP, const half PC, const byte P )
{
	return ( cpu.A == A ) && ( cpu.X == X ) && ( cpu.Y == Y ) && ( cpu.SP == SP ) && ( cpu.PC == PC ) && ( cpu.P == P );
}


bool Easy6502Tests( Cpu6502& cpu )
{
	Load6502Program( cpu, "Program1.bin", 0x0600, 0x0600 );
	cpu.Run();	
	const bool passed1 = TestRegisters( cpu, 0x08, 0x00, 0x00, 0xFF, 0x0610, 0x30 );

	Load6502Program( cpu, "Program2.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed2 = TestRegisters( cpu, 0x84, 0xC1, 0x00, 0xFF, 0x0607, 0xB1 );

	Load6502Program( cpu, "Program3.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed3 = TestRegisters( cpu, 0x00, 0x03, 0x00, 0xFF, 0x060E, 0x33 );

	Load6502Program( cpu, "Program4.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed4 = TestRegisters( cpu, 0x01, 0x00, 0x00, 0xFF, 0x0609, 0xB0 );

	Load6502Program( cpu, "Program5.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed5 = TestRegisters( cpu, 0xCC, 0x00, 0x00, 0xFF, 0xCC02, 0xB0 );

	Load6502Program( cpu, "Program6.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed6 = TestRegisters( cpu, 0x0A, 0x01, 0x0A, 0xFF, 0x0612, 0x30 );

	Load6502Program( cpu, "Program7.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed7 = TestRegisters( cpu, 0x0A, 0x0A, 0x01, 0xFF, 0x0612, 0x30 );

	Load6502Program( cpu, "Program8.bin", 0x0600, 0x0600 );
	cpu.Run();

	const byte memCheckValues[48] = {	0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
										0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
										0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

	bool memCheck = true;

	for ( half i = 0x00; i < 0x0F; ++i )
	{
		if ( cpu.memory[0x01f0 + i] != memCheckValues[i] )
		{
			memCheck = false;
			break;
		}
	}

	const bool passed8 = memCheck && TestRegisters( cpu, 0x00, 0x10, 0x20, 0xFF, 0x0619, 0x33 );

	Load6502Program( cpu, "Program9.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed9 = TestRegisters( cpu, 0x03, 0x00, 0x00, 0xFF, 0x060C, 0x30 );

	Load6502Program( cpu, "Program10.bin", 0x0600, 0x0600 );
	cpu.Run();
	const bool passed10 = TestRegisters( cpu, 0x00, 0x05, 0x00, 0xFD, 0x0613, 0x33 );

	return passed1 && passed2 && passed3 && passed4 && passed5 && passed6 && passed7 && passed8 && passed9 && passed10;
}


bool KlausTests( Cpu6502& cpu )
{
	Load6502Program( cpu, "6502_functional_test.bin", 0x0000, 0x0400 );

	cpu.Run();

	return ( cpu.A == 0x42 ) && ( cpu.X == 0x52 ) && ( cpu.Y == 0x4B ) && ( cpu.SP == 0xFF ) && ( cpu.PC == 0x09D0 ) && ( cpu.P == 0x30 );
}


float RunTests( Cpu6502& cpu )
{
	uint passedTests = Easy6502Tests( cpu ) ? 1 : 0;

	passedTests += KlausTests( cpu ) ? 1 : 0;

	return ( passedTests / 2.0f );
}


int main()
{
	Cpu6502 cpu;

	cpu.BuildInstructionMap();
	//cpu.PrintInstructionMap();
	cpu.Reset();

#if NES_MODE == 1
	NesCart cart;
	LoadNesFile( "Donkey Kong.nes", cart );

	LoadProgram( cart );

	//Run();
#else
	const float passedPercent = RunTests( cpu );

	cout << "Passed Tests %" << ( passedPercent * 100.0f ) << endl;
#endif

#if DEBUG_MEM
	/*
	uint colorIndex[] = { 0x000000FF, 0xFFFFFFFF,	0x880000FF, 0xAAFFEEFF,
		0xCC44CCFF, 0x00CC55FF, 0x0000AAFF, 0xEEEE77FF,
		0xDD8855FF, 0x664400FF, 0xFF7777FF, 0x333333FF,
		0x777777FF, 0xAAFF66FF, 0x0088FFFF, 0xBBBBBBFF };
	/*
	for ( std::map< half, byte>::iterator it = memoryDebug.begin(); it != memoryDebug.end(); ++it )
	{
		const uint x = ( it->first - 0x200 ) % 32;
		const uint y = ( it->first - 0x200 ) / 32;

		image.setPixel( x, 31 - y, colorIndex[it->second % 16] );
	}
	*/

#if NES_MODE == 1
	uint colorIndex[] = { 0x000000FF, 0xFF0000FF,	0x880000FF, 0xAAFFEEFF };

	Bitmap image( 128, 256, 0x00 );
	
	int tileIx = 0;

	const byte tilebytes = 16;

	for ( int tileIx = 0; tileIx < 512; ++tileIx )
	{
		const half baseAddr = cart.header.prgRomBanks * 0x4000;
		
		const half chrRomBase = baseAddr + tileIx * tilebytes;

		for ( int y = 0; y < 8; ++y )
		{
			for ( int x = 0; x < 8; ++x )
			{
				const byte chrRom0 = cart.rom[chrRomBase + y];
				const byte chrRom1 = cart.rom[chrRomBase + ( tilebytes / 2 ) + y];

				const half xBitMask = ( 0x80 >> x );

				const byte lowerBits = ( ( chrRom0 & xBitMask ) >> ( 7 - x ) ) | ( ( ( chrRom1 & xBitMask ) >> ( 7 - x ) ) << 1 );

				const uint imageX = x + ( tileIx % 16 ) * 8;
				const uint imageY = y + 8 * floor( tileIx / 16.0f );

				image.setPixel( imageX, 255 - imageY, colorIndex[lowerBits] );
			}
		}

		//stringstream bitmapNameStream;
		//bitmapNameStream << "output" << tileIx << ".bmp";
		//image.write( bitmapNameStream.str() );
	}
	

	image.write( "output.bmp" );
#endif // #if NES_MODE == 1
	
#endif // #if DEBUG_MEM

	char end;
	cin >> end;

	return 0;
}

