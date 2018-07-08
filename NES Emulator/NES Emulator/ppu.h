#pragma once

struct PPU;

typedef byte&( PPU::* PpuRegWriteFunc )( const byte value );
typedef byte&( PPU::* PpuRegReadFunc )();


// TODO: Enums overkill?
enum PatternTable : byte
{
	PATTERN_TABLE_0 = 0X00,
	PATTERN_TABLE_1 = 0X01,
};


enum SpriteMode : byte
{
	SPRITE_MODE_8x8 = 0X00,
	SPRITE_MODE_8x16 = 0X01,
};


enum Nametable : byte
{
	NAMETABLE_0 = 0X00,
	NAMETABLE_1 = 0X01,
	NAMETABLE_2 = 0X02,
	NAMETABLE_3 = 0X03,
};


enum VramInc : byte
{
	VRAM_INC_0 = 0x00,
	VRAM_INC_1 = 0x01,
};


enum MasterSlaveMode : byte
{
	MASTER_SLAVE_READ_EXT	= 0x00,
	MASTER_SLAVE_WRITE_EXT	= 0x01,
};

enum NmiVblank : byte
{
	NMI_VBLANK_OFF	= 0X00,
	NMI_VBLANK_ON	= 0X01,
};


enum PpuReg : byte
{
	PPUREG_CTRL,
	PPUREG_MASK,
	PPUREG_STATUS,
	PPUREG_OAMADDR,
	PPUREG_OAMDATA,
	PPUREG_SCROLL,
	PPUREG_ADDR,
	PPUREG_DATA,
	PPUREG_OAMDMA,
};


union PpuCtrl
{
	struct PpuCtrlSemantic
	{
		Nametable		nameTableAddr	: 2;
		VramInc			vramInc			: 1;
		PatternTable	spriteTableAddr	: 1;
	
		PatternTable	bgTableAddr		: 1;
		SpriteMode		spriteSize		: 1;
		MasterSlaveMode	masterSlaveMode	: 1;
		NmiVblank		nmiVblank		: 1;
	} sem;

	byte raw;
};



union PpuMask
{
	struct PpuMaskSemantic
	{
		byte	greyscale	: 1;
		byte	bgLeft		: 1;
		byte	sprtLeft	: 1;
		byte	showBg		: 1;

		byte	showSprt	: 1;
		byte	emphazieR	: 1;
		byte	emphazieG	: 1;
		byte	emphazieB	: 1;
	} sem;

	byte raw;
};


union PpuStatusLatch
{
	struct PpuStatusSemantic
	{
		byte	lastReadLsb : 5;
		byte	o : 1;
		byte	s : 1;
		byte	v : 1;

	} sem;

	byte raw;
};


struct PpuStatus
{
	bool hasLatch;
	PpuStatusLatch current;
	PpuStatusLatch latched;
};



struct PPU
{
	static const half VirtualMemorySize = 0xFFFF;
	static const half PhysicalMemorySize = 0x3FFF;
	static const byte RegisterCount = 0x08;
	//static const ppuCycle_t VBlankCycles = ppuCycle_t( 20 * 341 * 5 );

	PatternTable bgPatternTbl;
	PatternTable sprPatternTbl;

	PpuCtrl regCtrl;
	PpuMask regMask;
	PpuStatus regStatus;

	bool masterSlave;
	bool genNMI;

	int currentPixel; // pixel on scanline
	int currentScanline;
	ppuCycle_t scanelineCycle;

	bool nmiOccurred;

	ppuCycle_t cycle;

	NesSystem* system;

	half vramAddr;

	byte vram[VirtualMemorySize];
	byte primaryOAM[256];
	byte secondaryOAM[32];

	byte registers[8]; // no need?

	inline byte& PPUCTRL( const byte value )
	{
		regCtrl.raw = value;
		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return regCtrl.raw;
	}


	inline byte& PPUCTRL()
	{
		return regCtrl.raw;
	}


	inline byte& PPUMASK( const byte value )
	{
		regMask.raw = value;
		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return regMask.raw;
	}


	inline byte& PPUMASK()
	{
		return regMask.raw;
	}


	inline byte& PPUSTATUS( const byte value )
	{
	// TODO: need to redesign to not return reference
		assert( value == 0 );

		regStatus.latched = regStatus.current;
		regStatus.latched.sem.v = 0;
		regStatus.latched.sem.lastReadLsb = 0;
		regStatus.hasLatch = true;
		registers[PPUREG_ADDR] = 0;
		registers[PPUREG_DATA] = 0;
		vramAddr = 0;

		if ( regStatus.current.raw > 0x10 )
		{
			std::cout << "Register is: " << std::hex << (int)regStatus.current.raw << std::endl;
		//	EnablePrinting();
		}

		return regStatus.current.raw;
	}


	inline byte& PPUSTATUS()
	{
		regStatus.latched = regStatus.current;
		regStatus.latched.sem.v = 0;
		regStatus.latched.sem.lastReadLsb = 0;
		regStatus.hasLatch = true;
		registers[PPUREG_ADDR] = 0;
		registers[PPUREG_DATA] = 0;
		vramAddr = 0;

		if ( regStatus.current.raw > 0x10 )
		{
			std::cout << "Register is: " << std::hex << (int)regStatus.current.raw << std::endl;
		//	EnablePrinting();
		}


		return regStatus.current.raw;
	}


	inline byte& OAMADDR( const byte value )
	{
		registers[PPUREG_OAMADDR] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return registers[PPUREG_OAMADDR];
	}


	inline byte& OAMADDR()
	{
		return registers[PPUREG_OAMADDR];
	}


	inline byte& OAMDATA( const byte value )
	{
		registers[PPUREG_OAMDATA] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return registers[PPUREG_OAMDATA];
	}


	inline byte& OAMDATA()
	{
		return registers[PPUREG_OAMDATA];
	}


	inline byte& PPUSCROLL( const byte value )
	{
		registers[PPUREG_SCROLL] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return registers[PPUREG_SCROLL];
	}


	inline byte& PPUSCROLL()
	{
		return registers[PPUREG_SCROLL];
	}


	inline byte& PPUADDR( const byte value )
	{
		registers[PPUREG_ADDR] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		vramAddr = ( vramAddr << 8 ) | ( value );

		return registers[PPUREG_ADDR];
	}


	inline byte& PPUADDR()
	{
		return registers[PPUREG_ADDR];
	}


	inline byte& PPUDATA( const byte value )
	{
		if( value == 0x62 )
		{
		//	EnablePrinting();
		}
		registers[PPUREG_DATA] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		vramWritePending = true;

		return registers[PPUREG_DATA];
	}


	inline byte& PPUDATA()
	{
		return registers[PPUREG_DATA];
	}

	byte DoDMA( const half address );
	void EnablePrinting();

	inline byte& OAMDMA( const byte value )
	{
		GenerateDMA();
		DoDMA( value );

		return registers[PPUREG_OAMDMA];
	}


	inline byte& OAMDMA()
	{
		GenerateDMA();
		return registers[PPUREG_OAMDMA];
	}


	inline void RenderScanline()
	{
		/*
		Memory fetch phase 1 thru 128
		---------------------------- -
		1. Name table byte
		2. Attribute table byte
		3. Pattern table bitmap #0
		4. Pattern table bitmap #1
		*/
	}

	void GenerateNMI();
	void GenerateDMA();

	bool vramWritePending;

	void WriteVram()
	{
		if( vramWritePending )
		{
			vram[vramAddr] = registers[PPUREG_DATA];
			vramAddr += regCtrl.sem.vramInc ? 32 : 1;

			vramWritePending = false;
		}
	}

	inline const ppuCycle_t Exec()
	{
		ppuCycle_t cycles = ppuCycle_t(0);

		// Cycle timing
		const uint64_t cycleCount = scanelineCycle.count();

		if ( regStatus.hasLatch )
		{
			regStatus.current = regStatus.latched;
			regStatus.latched.raw = 0;
			regStatus.hasLatch = false;
		}

		WriteVram();

		// Scanline Work
		// Scanlines take multiple cycles
		if( cycleCount == 0 )
		{
			if ( currentScanline < 0 )
			{
				regStatus.current.sem.v = 0;
				regStatus.latched.raw = 0;
				regStatus.hasLatch = false;
			}
			else if ( currentScanline < 240 )
			{		
			}
			else if ( currentScanline == 240 )
			{
			}
			else if ( currentScanline == 241 )
			{
			// must only exec once!
				// set vblank flag at second tick, cycle=1
				regStatus.current.sem.v = 1;
				// Gen NMI
				if( regCtrl.sem.nmiVblank )
					GenerateNMI();
			}
			else if ( currentScanline < 260 )
			{
			}
			else
			{
			}
		}

		if( cycleCount == 0 )
		{
			// Idle cycle
			cycles++;
			scanelineCycle++;
		}
		else if ( cycleCount <= 256 )
		{
			// Scanline render
			cycles += ppuCycle_t(8);
			scanelineCycle += ppuCycle_t( 8 );
		}
		else if ( cycleCount <= 320 )
		{
			// Prefetch the 8 sprites on next scanline
			cycles += ppuCycle_t( 8 );
			scanelineCycle += ppuCycle_t( 8 );
		}
		else if ( cycleCount <= 336 )
		{
			// Prefetch first two tiles on next scanline
			cycles++;
			scanelineCycle++;
		}
		else if ( cycleCount <= 340 )
		{
			// Unused fetch
			cycles += ppuCycle_t( 8 );
			scanelineCycle += ppuCycle_t( 8 );
		}
		else
		{
			if ( currentScanline >= 260 )
			{
				currentScanline = -1;
			}
			else
			{
				++currentScanline;
			}

			scanelineCycle = scanelineCycle.zero();
		}

		return cycles;
	}


	inline bool Step( const ppuCycle_t nextCycle )
	{
		/*
		Start of vertical blanking : Set NMI_occurred in PPU to true.
		End of vertical blanking, sometime in pre - render scanline : Set NMI_occurred to false.
		Read PPUSTATUS : Return old status of NMI_occurred in bit 7, then set NMI_occurred to false.
		Write to PPUCTRL : Set NMI_output to bit 7.
		The PPU pulls / NMI low if and only if both NMI_occurred and NMI_output are true.By toggling NMI_output( PPUCTRL.7 ) during vertical blank without reading PPUSTATUS, a program can cause / NMI to be pulled low multiple times, causing multiple NMIs to be generated.
		*/

		while ( cycle < nextCycle )
		{
			cycle += Exec();
		}

		return true;
	}
/*
	PPU()
	{
		cycle = ppuCycle_t( 0 );
	}*/


	byte& Reg( half address, byte value );
	byte& Reg( half address );
};

static const PpuRegWriteFunc PpuRegWriteMap[PPU::RegisterCount] =
{
	&PPU::PPUCTRL,
	&PPU::PPUMASK,
	&PPU::PPUSTATUS,
	&PPU::OAMADDR,
	&PPU::OAMDATA,
	&PPU::PPUSCROLL,
	&PPU::PPUADDR,
	&PPU::PPUDATA,
};



static const PpuRegReadFunc PpuRegReadMap[PPU::RegisterCount] =
{
	&PPU::PPUCTRL,
	&PPU::PPUMASK,
	&PPU::PPUSTATUS,
	&PPU::OAMADDR,
	&PPU::OAMDATA,
	&PPU::PPUSCROLL,
	&PPU::PPUADDR,
	&PPU::PPUDATA,
};

