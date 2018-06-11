#include "stdafx.h"
#include <assert.h>
#include "common.h"
#include "6502.h"


class System6502
{
public:
	System6502(){};
protected:
	byte* memory;
	Cpu6502 cpu;
};


class TestSystem6503 : System6502
{
	static const half PhysicalMemorySize = 0x0800;
	static const half VirtualMemorySize = 0xFFFF;

public:
	TestSystem6503()
	{
		memory = &ram[0];
	}

private:
	byte ram[VirtualMemorySize];
};


class NesSystem : System6502
{
	static const half PhysicalMemorySize = 0x0800;
	static const half VirtualMemorySize = 0xFFFF;

	Cpu6502Config config;

	PPU ppu;
	Cpu6502 cpu;

public:
	NesSystem()
	{
		memory = &ram[0];
	}

	bool Run()
	{
		while ( true )
		{
			cpu.Step();
		}

		return true;
	}

	void LoadProgram( const NesCart& cart )
	{
		memset( cpu.memory, 0, VirtualMemorySize );

		memcpy( cpu.memory + config.Bank0, cart.rom, config.BankSize );

		assert( cart.header.prgRomBanks <= 2 );

		if ( cart.header.prgRomBanks == 1 )
		{
			memcpy( cpu.memory + config.Bank1, cart.rom, config.BankSize );
		}
		else
		{
			memcpy( cpu.memory + config.Bank1, cart.rom + config.Bank1, config.BankSize );
		}

		cpu.resetVector = ( memory[config.ResetVectorAddr + 1] << 8 ) | memory[config.ResetVectorAddr];

		cpu.Reset();
	}

private:
	byte ram[VirtualMemorySize];
};