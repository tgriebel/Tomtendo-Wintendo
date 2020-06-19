#pragma once

#include "common.h"
#include "NesSystem.h"

class MMC1 : public wtMapper
{
private:
	
	uint8_t prgRamBank[KB_8];
	uint8_t prgRomBank0[KB_16];
	uint8_t prgRomBank1[KB_16];
	uint8_t chrRomBank0[KB_4];
	uint8_t chrRomBank1[KB_4];

	uint8_t ctrlReg;
	uint8_t chrBank0Reg;
	uint8_t chrBank1Reg;
	uint8_t prgBankReg;

	wtShiftReg<5> shiftRegister;

	static const uint8_t ClearBit		= 0x80;
	static const uint8_t CtrlRegDefault = 0x0C;

	uint8_t GetMirrorMode()
	{
		const uint8_t mirrorBits = ctrlReg & 0x03;

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
		}
	}

	uint8_t MapMemory( const uint16_t address, const uint8_t regValue )
	{
		const uint16_t maskedAddr = address;//( addr & 0x6000 ); // Only bits 13-14 decoded

		const bool hasChrRom = system->cart.header.chrRomBanks > 0;
		const uint32_t chrRomStart = system->cart.header.prgRomBanks * KB_16;
		uint16_t prgBankSize = KB_16;
		uint16_t chrRomBankSize = KB_8;
		uint16_t prgSrcBank = 0;
		uint16_t prgDestBank = 0;

		if ( InRange( address, 0x8000, 0x9FFF ) )
		{
			ctrlReg = regValue;		
			system->mirrorMode = GetMirrorMode();
			
		}
		else if ( hasChrRom && InRange( address, 0xA000, 0xBFFF ) )
		{			
			if ( ctrlReg & 0x10 )
			{
				chrRomBankSize = KB_4;
				chrBank0Reg = regValue;
			}
			else
			{
				chrRomBankSize = KB_8;
				chrBank0Reg = regValue & 0x0E;
			}

			memcpy( &system->ppu.vram[PPU::PatternTable0BaseAddr], &system->cart.rom[chrRomStart + chrBank0Reg * chrRomBankSize], chrRomBankSize );
		}
		else if ( hasChrRom && InRange( address, 0xC000, 0xDFFF ) )
		{
			if ( ( ctrlReg & 0x10 ) != 0 )
			{
				chrBank1Reg = regValue;
				memcpy( &system->ppu.vram[PPU::PatternTable1BaseAddr], &system->cart.rom[chrRomStart + chrBank1Reg * KB_4], KB_4 );
			}
		}
		else if ( InRange( address, 0xE000, 0xFFFF ) )
		{
			assert( system->cart.header.prgRomBanks > 0 );

			if ( ctrlReg & 0x08 )
			{
				prgBankSize	= KB_16;
				prgSrcBank	= regValue & 0x0F;
				prgDestBank	= ( ctrlReg & 0x04 ) ? wtSystem::Bank0 : wtSystem::Bank1;
			}
			else
			{
				prgBankSize	= KB_32;
				prgSrcBank	= regValue & 0x0E;
				prgDestBank	= wtSystem::Bank0;
			}

			memcpy( &system->memory[prgDestBank], &system->cart.rom[prgSrcBank * KB_16], prgBankSize );
		}
		
		return 0;
	}

public:

	MMC1():	ctrlReg( CtrlRegDefault ),
			chrBank0Reg(0),
			chrBank1Reg(0),
			prgBankReg(0)
	{
		shiftRegister.Clear();
	}

	uint8_t OnLoadCpu()
	{
		const size_t lastBank = ( system->cart.header.prgRomBanks - 1 );
		memcpy( &system->memory[wtSystem::Bank0], system->cart.rom, wtSystem::BankSize );
		memcpy( &system->memory[wtSystem::Bank1], &system->cart.rom[lastBank * wtSystem::BankSize], wtSystem::BankSize );

		return 0;
	}

	uint8_t OnLoadPpu()
	{
		return 0;
	}

	bool InWriteWindow( const uint16_t addr, const uint16_t offset )
	{
		return ( system->cart.GetMapperId() == 1 ) && ( addr >= 0x8000 ) && ( addr <= 0xFFFF );
	}


	uint8_t Write( const uint16_t addr, const uint16_t offset, const uint8_t value )
	{
		const uint16_t address = ( addr + offset );

		if ( !InRange( address, 0x8000, 0xFFFF ) )
			return 0;

		if ( ( value & 0x80 ) > 0 )
		{
			shiftRegister.Clear();
			ctrlReg |= 0x0C;

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
};