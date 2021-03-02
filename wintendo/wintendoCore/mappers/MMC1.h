#pragma once

#include "common.h"
#include "NesSystem.h"

class MMC1 : public wtMapper
{
private:
	union ctrlReg_t
	{
		struct semantic_t
		{
			uint8_t	mirror : 2;
			uint8_t	prgMode : 2;
			uint8_t	chrMode : 1;
			uint8_t	unused : 3;
		} sem;

		uint8_t byte;
	};

	static const uint8_t ClearBit = 0x80;
	static const uint8_t CtrlRegDefault = 0x0C;

	uint8_t			prgRamBank[ KB( 8 ) ];

	ctrlReg_t		ctrlReg;
	uint8_t			chrBank0Reg;
	uint8_t			chrBank1Reg;
	uint8_t			prgBankReg;

	uint8_t			bank0;
	uint8_t			bank1;
	uint8_t			chrBank0;
	uint8_t			chrBank1;

	uint8_t			chrRam[ PPU::PatternTableMemorySize ];

	wtShiftReg<5>	shiftRegister;

	uint8_t GetMirrorMode()
	{
		const uint8_t mirrorBits = ctrlReg.sem.mirror;

		if ( mirrorBits == 2 )
		{
			return MIRROR_MODE_VERTICAL;
		}
		else if ( mirrorBits == 3 )
		{
			return MIRROR_MODE_HORIZONTAL;
		}
		else
		{
			assert( 0 ); // Unimplemented
			return MIRROR_MODE_HORIZONTAL;
		}
	}

	uint8_t MapMemory( const uint16_t address, const uint8_t regValue )
	{
		uint16_t mode = ( address >> 13 ) & 3; // Only bits 13-14 decoded

		const bool hasChrRom = system->cart->h.chrRomBanks > 0;
		const uint32_t chrRomStart = system->cart->h.prgRomBanks * KB( 16 );
		uint16_t chrRomBankSize = KB( 8 );

		if ( mode == 0 ) // Control
		{
			ctrlReg.byte = regValue;
			system->mirrorMode = GetMirrorMode();
		}
		else if ( mode == 1 ) // CHR bank 0
		{
			if ( ctrlReg.sem.chrMode )
			{
				chrRomBankSize = KB( 4 );
				chrBank0Reg = regValue;
				chrBank0 = chrBank0Reg;
			}
			else
			{
				chrRomBankSize = KB( 8 );
				chrBank0Reg = regValue & 0x1E;
				chrBank0 = chrBank0Reg;
				chrBank1 = chrBank0Reg + 1;
			}
		}
		else if ( mode == 2 ) // CHR bank 1
		{
			if ( ctrlReg.sem.chrMode )
			{
				chrBank1Reg = regValue;
				chrBank1 = chrBank1Reg;
			}
		}
		else if ( mode == 3 ) // PRG bank
		{
			assert( system->cart->h.prgRomBanks > 0 );

			if ( ctrlReg.sem.prgMode >= 2 )  // 16 KB mode
			{
				const uint8_t prgSrcBank = regValue & 0x0F;
				if ( ctrlReg.sem.prgMode == 3 ) {
					bank0 = prgSrcBank;
					bank1 = ( system->cart->h.prgRomBanks ) - 1;
				}
				else if ( ctrlReg.sem.prgMode == 2 ) {
					bank0 = 0;
					bank1 = prgSrcBank;
				}
				else {
					assert( 0 );
				}
			}
			else // 32 KB mode
			{
				const uint8_t prgSrcBank = regValue & 0x0E;
				bank0 = prgSrcBank;
				bank1 = prgSrcBank + 1;
			}
		}

		return 0;
	}

public:

	MMC1( const uint32_t _mapperId ) :
		chrBank0Reg( 0 ),
		chrBank1Reg( 0 ),
		prgBankReg( 0 ),
		bank0( 0 ),
		bank1( 0 )
	{
		ctrlReg.byte = CtrlRegDefault;
		mapperId = _mapperId;
		shiftRegister.Clear();
		memset( prgRamBank, 0, KB( 8 ) );
	}

	uint8_t OnLoadCpu() override
	{
		bank0 = 0;
		bank1 = ( system->cart->h.prgRomBanks - 1 );
		chrBank0 = 0;
		chrBank1 = 1;
		return 0;
	}

	uint8_t OnLoadPpu() override
	{
		memset( chrRam, 0, sizeof( PPU::PatternTableMemorySize ) );
		return 0;
	}

	uint8_t	ReadRom( const uint16_t addr ) override
	{
		if ( InRange( addr, wtSystem::Bank0, wtSystem::Bank0End ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank0 );
			return system->cart->GetPrgRomBank( bank0 )[ bankAddr ];
		}
		else if ( InRange( addr, wtSystem::Bank1, wtSystem::Bank1End ) )
		{
			const uint16_t bankAddr = ( addr - wtSystem::Bank1 );
			return system->cart->GetPrgRomBank( bank1 )[ bankAddr ];
		}
		else if ( InRange( addr, wtSystem::SramBase, wtSystem::SramEnd ) )
		{
			const uint16_t sramAddr = ( addr - wtSystem::SramBase );
			return prgRamBank[ sramAddr ];
		}

		assert( 0 );
		return 0;
	}

	uint8_t	ReadChrRom( const uint16_t addr ) override
	{
		if ( InRange( addr, 0x0000, 0x1FFF ) && system->cart->HasChrRam() ) {
			return chrRam[ addr ];
		}
		else if ( InRange( addr, 0x0000, 0x0FFF ) ) {
			return system->cart->GetChrRomBank( chrBank0, KB( 4 ) )[ addr ];
		}
		else if ( InRange( addr, 0x1000, 0x1FFF ) ) {
			return system->cart->GetChrRomBank( chrBank1, KB( 4 ) )[ addr - 0x1000 ];
		}
		assert( 0 );
		return 0;
	}

	uint8_t	WriteChrRam( const uint16_t addr, const uint8_t value ) override
	{
		if ( InRange( addr, 0x0000, 0x1FFF ) && system->cart->HasChrRam() ) {
			chrRam[ addr ] = value;
			return 1;
		}
		return 0;
	}

	bool InWriteWindow( const uint16_t addr, const uint16_t offset ) override
	{
		const uint16_t address = ( addr + offset );
		return ( system->cart->GetMapperId() == mapperId ) && InRange( address, wtSystem::ExpansionRomBase, wtSystem::Bank1End );
	}

	uint8_t Write( const uint16_t address, const uint8_t value ) override
	{
		//assert( address >= wtSystem::SramBase );
		if ( InRange( address, wtSystem::SramBase, wtSystem::SramEnd ) ) {
			const uint16_t sramAddr = ( address - wtSystem::SramBase );
			prgRamBank[ sramAddr ] = value;
			return 0;
		}

		if ( ( value & 0x80 ) > 0 )
		{
			shiftRegister.Clear();
			ctrlReg.byte |= 0x0C;
			return 0;
		}

		shiftRegister.Shift( value & 1 );

		if ( shiftRegister.IsFull() )
		{
			const uint8_t regValue = shiftRegister.GetValue();
			shiftRegister.Clear();
			MapMemory( address, regValue );
		}

		return 0;
	}

	void Serialize( Serializer& serializer, const serializeMode_t mode ) override
	{
		serializer.Next8b( ctrlReg.byte, mode );
		serializer.Next8b( chrBank0Reg, mode );
		serializer.Next8b( chrBank1Reg, mode );
		serializer.Next8b( bank0, mode );
		serializer.Next8b( bank1, mode );
		serializer.Next8b( chrBank0, mode );
		serializer.Next8b( chrBank1, mode );

		if ( mode == serializeMode_t::STORE ) {
			uint8_t shift = shiftRegister.GetValue();
			serializer.Next8b( shift, mode );
		}
		else if ( mode == serializeMode_t::LOAD ) {
			uint8_t shift;
			serializer.Next8b( shift, mode );
			shiftRegister.Set( shift );
		}

		serializer.NextArray( reinterpret_cast<uint8_t*>( &prgRamBank[ 0 ] ), KB( 8 ), mode );
		serializer.NextArray( reinterpret_cast<uint8_t*>( &chrRam[ 0 ] ), PPU::PatternTableMemorySize, mode );
	}
};