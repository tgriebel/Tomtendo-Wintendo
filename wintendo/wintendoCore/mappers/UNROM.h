/*
* MIT License
*
* Copyright( c ) 2017-2021 Thomas Griebel
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

#include "common.h"
#include "NesSystem.h"

class UNROM : public wtMapper
{
private:
	uint8_t		bank;
	uint8_t		chrRam[ PPU::PatternTableMemorySize ];
	uint8_t*	prgBanks[ 2 ];
	uint8_t*	chrBank;
public:
	UNROM( const uint32_t _mapperId )
	{
		mapperId = _mapperId;
		bank = 0;
		prgBanks[ 0 ] = nullptr;
		prgBanks[ 1 ] = nullptr;
		chrBank = nullptr;
	}

	uint8_t OnLoadCpu() override
	{
		bank = 0;
		const uint8_t lastBank = ( system->cart->h.prgRomBanks - 1 );
		prgBanks[ 0 ] = system->cart->GetPrgRomBank( bank );
		prgBanks[ 1 ] = system->cart->GetPrgRomBank( lastBank );
		return 0;
	};

	uint8_t OnLoadPpu() override
	{
		if( system->cart->HasChrRam() ) {
			chrBank = chrRam;
		} else {
			chrBank = system->cart->GetChrRomBank( 0 );
		}
		memset( chrRam, 0, sizeof( PPU::PatternTableMemorySize ) );
		return 0;
	};

	uint8_t	ReadRom( const uint16_t addr ) const override
	{
		const uint8_t bank = ( addr >> 14 ) & 0x01;
		const uint16_t offset = ( addr & ( wtSystem::BankSize - 1 ) );

		return prgBanks[ bank ][ offset ];
	}

	uint8_t	ReadChrRom( const uint16_t addr ) const override
	{
		return chrBank[ addr ];
	}

	uint8_t	 WriteChrRam( const uint16_t addr, const uint8_t value ) override
	{
		if ( InRange( addr, 0x0000, 0x1FFF ) && system->cart->HasChrRam() ) {
			chrRam[ addr ] = value;
			return 1;
		}
		return 0;
	}

	uint8_t Write( const uint16_t addr, const uint8_t value ) override
	{
		bank = ( value & 0x07 );
		prgBanks[ 0 ] = system->cart->GetPrgRomBank( bank );
		return 0;
	};

	bool InWriteWindow( const uint16_t addr, const uint16_t offset ) const override
	{
		const uint16_t address = ( addr + offset );
		return InRange( address, wtSystem::ExpansionRomBase, wtSystem::Bank1End );
	}

	void Serialize( Serializer& serializer ) override
	{
		serializer.Next8b( bank );

		if( system->cart->HasChrRam() ) {
			serializer.NextArray( chrRam, PPU::PatternTableMemorySize );
		}

		if( serializer.GetMode() == serializeMode_t::LOAD )
		{
			prgBanks[ 0 ] = system->cart->GetPrgRomBank( bank );
			if ( !system->cart->HasChrRam() ) {
				chrBank = system->cart->GetChrRomBank( 0 );
			}
		}
	}
};