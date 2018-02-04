#pragma once

#include <fstream>
#include <string>
#include "types_common.h"
using namespace std;

inline half Combine( const byte lsb, const byte msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}


void LoadNesFile( const string& fileName, NesCart& outCart )
{
	ifstream nesFile;
	nesFile.open( fileName, ios::binary );
	std::string rawFile( ( istreambuf_iterator< char >( nesFile ) ), istreambuf_iterator< char >() );
	nesFile.close();

	memcpy( &outCart, rawFile.c_str(), rawFile.size() );

	outCart.size = rawFile.size();
	outCart.header.numBanks = max( 1, outCart.header.numBanks );
}