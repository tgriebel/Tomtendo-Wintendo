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

#include "../common.h"
#include "../system/NesSystem.h"

class NROM : public wtMapper
{
private:
	uint8_t*	prgBanks[2];
	uint8_t*	chrBank;
public:
	NROM( const uint32_t _mapperId )
	{
		mapperId = _mapperId;
		prgBanks[0] = nullptr;
		prgBanks[1] = nullptr;
		chrBank = nullptr;
	}

	uint8_t OnLoadCpu() override
	{
		const uint8_t bank1 = ( system->cart->GetPrgBankCount() == 1 ) ? 0 : 1;
		prgBanks[ 0 ] = system->cart->GetPrgRomBank( 0 );
		prgBanks[ 1 ] = system->cart->GetPrgRomBank( bank1 );
		return 0;
	};

	uint8_t OnLoadPpu() override
	{
		chrBank = system->cart->GetChrRomBank( 0 );
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
};