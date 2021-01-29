#pragma once
#pragma once

#include "common.h"
#include "NesSystem.h"

class MMC3 : public wtMapper
{
private:

	union BankSelect
	{
		struct BankSelectSemantic
		{
			uint8_t	bankReg			: 3;
			uint8_t	unused			: 3;
			uint8_t	prgRomBankMode	: 1;
			uint8_t	chrA12Inversion : 1;
		} sem;

		uint8_t byte;
	};

	BankSelect	bankSelect;
	uint16_t	R[8];
	uint8_t		irqLatch;
	uint8_t		irqCounter;
	bool		irqReload;
	bool		irqEnable;
	int8_t		oldPrgBankMode;
	int8_t		oldChrBankMode;
	bool		bankDataInit;

	uint8_t GetMirrorMode()
	{
		return 0;
	}

	uint8_t MapMemory( const uint16_t address, const uint8_t regValue )
	{
		return 0;
	}

public:

	MMC3( const uint32_t _mapperId ) :
		irqReload( false ),
		irqLatch( 0x00 ),
		irqCounter( 0x00 ),
		irqEnable( false ),
		oldPrgBankMode( -1 ),
		oldChrBankMode( -1 ),
		bankDataInit( false )
	{
		mapperId = _mapperId;
		bankSelect.byte = 0;
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
		return ( system->cart.GetMapperId() == mapperId ) && ( addr >= 0x8000 ) && ( addr <= 0xFFFF );
	}


	void Clock()
	{
		if( ( irqCounter == 0 ) || irqReload )
		{
			if( irqEnable ) {
			//	system->RequestIRQ();
			}
			
			irqCounter = irqLatch;
			irqReload = false;
		} else {
			--irqCounter;
		}
	}


	uint8_t Write( const uint16_t addr, const uint16_t offset, const uint8_t value )
	{
		const uint16_t address = ( addr + offset );
		bool swapPrgBanks = false;
		bool swapChrBanks = false;

		if ( InRange( address, 0x8000, 0x9FFF ) )
		{
			if( ( address % 2 ) == 0 )
			{
				bankSelect.byte = value;

				if( bankSelect.sem.prgRomBankMode != oldPrgBankMode )
				{
					swapPrgBanks = bankDataInit;
					oldPrgBankMode = bankSelect.sem.prgRomBankMode;
				}

				if ( bankSelect.sem.chrA12Inversion != oldChrBankMode )
				{
					swapChrBanks = bankDataInit;
					oldChrBankMode = bankSelect.sem.chrA12Inversion;
				}
			}
			else
			{
				switch( bankSelect.sem.bankReg )
				{
					default: case 2: case 3: case 4: case 5:
						R[bankSelect.sem.bankReg] = value;
					break;

					case 0: case 1:
						R[bankSelect.sem.bankReg] = value & 0xFE;
					break;

					case 6: case 7:
						R[bankSelect.sem.bankReg] = value & 0x3F;
					break;
				}

				bankDataInit = true;
				swapPrgBanks = true;
				swapChrBanks = true;
			}
		}
		else if ( InRange( address, 0xA000, 0xBFFF ) )
		{
			if ( ( address % 2 ) == 0 )
			{
				if( !system->cart.header.controlBits0.fourScreenMirror )
				{
					system->mirrorMode = ( value & 0x01 ) ? MIRROR_MODE_HORIZONTAL : MIRROR_MODE_VERTICAL;
				}
			}
			else
			{
				// PRG RAM Protect
			}
		}
		else if ( InRange( address, 0xC000, 0xDFFF ) )
		{
			if ( ( address % 2 ) == 0 )	{
				irqLatch = value;
			} else {
				irqReload = true;
			}
		}
		else if ( InRange( address, 0xE000, 0xFFFF ) )
		{
			if ( ( address % 2 ) == 0 )
			{
				if ( irqEnable ) {
				//	system->RequestIRQ();
				}
				irqEnable = false;
			}
			else
			{
				irqEnable = true;
			}
		}

		if( swapPrgBanks )
		{
			const size_t lastBank = 0x1F;
			const size_t secondLastBank = 0x1E;
			if ( bankSelect.sem.prgRomBankMode )
			{
				memcpy( &system->memory[0x8000], &system->cart.rom[secondLastBank * KB_8], KB_8 );
				memcpy( &system->memory[0xA000], &system->cart.rom[R[7] * KB_8], KB_8 );
				memcpy( &system->memory[0xC000], &system->cart.rom[R[6] * KB_8], KB_8 );
				memcpy( &system->memory[0xE000], &system->cart.rom[lastBank * KB_8], KB_8 );
			}
			else
			{
				memcpy( &system->memory[0x8000], &system->cart.rom[R[6] * KB_8], KB_8 );
				memcpy( &system->memory[0xA000], &system->cart.rom[R[7] * KB_8], KB_8 );
				memcpy( &system->memory[0xC000], &system->cart.rom[secondLastBank * KB_8], KB_8 );
				memcpy( &system->memory[0xE000], &system->cart.rom[lastBank * KB_8], KB_8 );
			}
		}

		if( swapChrBanks )
		{
			const uint32_t chrRomStart = system->cart.header.prgRomBanks * KB_16;
			if ( bankSelect.sem.chrA12Inversion )
			{
				memcpy( &system->ppu.vram[0x0000], &system->cart.rom[chrRomStart + R[2] * KB_1], KB_1 );
				memcpy( &system->ppu.vram[0x0400], &system->cart.rom[chrRomStart + R[3] * KB_1], KB_1 );
				memcpy( &system->ppu.vram[0x0800], &system->cart.rom[chrRomStart + R[4] * KB_1], KB_1 );
				memcpy( &system->ppu.vram[0x0C00], &system->cart.rom[chrRomStart + R[5] * KB_1], KB_1 );
				memcpy( &system->ppu.vram[0x1000], &system->cart.rom[chrRomStart + R[0] * KB_1], KB_2 );
				memcpy( &system->ppu.vram[0x1800], &system->cart.rom[chrRomStart + R[1] * KB_1], KB_2 );
			}
			else
			{
				memcpy( &system->ppu.vram[0x0000], &system->cart.rom[chrRomStart + R[0] * KB_1], KB_2 );
				memcpy( &system->ppu.vram[0x0800], &system->cart.rom[chrRomStart + R[1] * KB_1], KB_2 );
				memcpy( &system->ppu.vram[0x1000], &system->cart.rom[chrRomStart + R[2] * KB_1], KB_1 );
				memcpy( &system->ppu.vram[0x1400], &system->cart.rom[chrRomStart + R[3] * KB_1], KB_1 );
				memcpy( &system->ppu.vram[0x1800], &system->cart.rom[chrRomStart + R[4] * KB_1], KB_1 );
				memcpy( &system->ppu.vram[0x1C00], &system->cart.rom[chrRomStart + R[5] * KB_1], KB_1 );
			}
		}
		
		//memcpy( &system->ppu.vram[PPU::PatternTable1BaseAddr], &system->cart.rom[chrRomStart + chrBank1Reg * KB_4], KB_4 );
		//memcpy( &system->memory[prgDestBank], &system->cart.rom[prgSrcBank * KB_16], prgBankSize );

		return 0;
	}
};