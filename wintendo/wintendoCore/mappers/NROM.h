#pragma once

#include "common.h"
#include "NesSystem.h"

class NROM : public wtMapper
{
public:
	NROM( const uint32_t _mapperId )
	{
		mapperId = _mapperId;
	}

	uint8_t	ReadRom( const uint16_t addr ) override
	{
		if( InRange( addr, wtSystem::Bank0, 0xFFFF ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank0 );
			return system->cart->rom[ bankAddr ];
		}
		assert( 0 );
		return 0;
	}

	uint8_t	ReadChrRom( const uint16_t addr ) override
	{
		if ( InRange( addr, 0x0000, 0x1FFF ) ) {
			return system->cart->GetChrRomBank( 0 )[ addr ];
		}
		assert( 0 );
		return 0;
	}
};