#pragma once

struct PPU;

typedef uint8_t&( PPU::* PpuRegWriteFunc )( const uint8_t value );
typedef uint8_t&( PPU::* PpuRegReadFunc )();


// TODO: Enums overkill?
enum PatternTable : uint8_t
{
	PATTERN_TABLE_0 = 0X00,
	PATTERN_TABLE_1 = 0X01,
};


enum SpriteMode : uint8_t
{
	SPRITE_MODE_8x8 = 0X00,
	SPRITE_MODE_8x16 = 0X01,
};


enum Nametable : uint8_t
{
	NAMETABLE_0 = 0X00,
	NAMETABLE_1 = 0X01,
	NAMETABLE_2 = 0X02,
	NAMETABLE_3 = 0X03,
};


enum VramInc : uint8_t
{
	VRAM_INC_0 = 0x00,
	VRAM_INC_1 = 0x01,
};


enum MasterSlaveMode : uint8_t
{
	MASTER_SLAVE_READ_EXT	= 0x00,
	MASTER_SLAVE_WRITE_EXT	= 0x01,
};

enum NmiVblank : uint8_t
{
	NMI_VBLANK_OFF	= 0X00,
	NMI_VBLANK_ON	= 0X01,
};


enum PpuReg : uint8_t
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

	uint8_t raw;
};



union PpuMask
{
	struct PpuMaskSemantic
	{
		uint8_t	greyscale	: 1;
		uint8_t	bgLeft		: 1;
		uint8_t	sprtLeft	: 1;
		uint8_t	showBg		: 1;

		uint8_t	showSprt	: 1;
		uint8_t	emphazieR	: 1;
		uint8_t	emphazieG	: 1;
		uint8_t	emphazieB	: 1;
	} sem;

	uint8_t raw;
};


union PpuStatusLatch
{
	struct PpuStatusSemantic
	{
		uint8_t	lastReadLsb : 5;
		uint8_t	o : 1;
		uint8_t	s : 1;
		uint8_t	v : 1;

	} sem;

	uint8_t raw;
};


struct PpuStatus
{
	bool hasLatch;
	PpuStatusLatch current;
	PpuStatusLatch latched;
};



struct PPU
{
	static const uint16_t VirtualMemorySize = 0xFFFF;
	static const uint16_t PhysicalMemorySize = 0x3FFF;
	static const uint8_t RegisterCount = 0x08;
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

	uint16_t vramAddr;

	uint8_t vram[VirtualMemorySize];
	uint8_t primaryOAM[256];
	uint8_t secondaryOAM[32];

	uint8_t registers[8]; // no need?

	inline uint8_t& PPUCTRL( const uint8_t value )
	{
		regCtrl.raw = value;
		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return regCtrl.raw;
	}


	inline uint8_t& PPUCTRL()
	{
		return regCtrl.raw;
	}


	inline uint8_t& PPUMASK( const uint8_t value )
	{
		regMask.raw = value;
		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return regMask.raw;
	}


	inline uint8_t& PPUMASK()
	{
		return regMask.raw;
	}


	inline uint8_t& PPUSTATUS( const uint8_t value )
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

		return regStatus.current.raw;
	}


	inline uint8_t& PPUSTATUS()
	{
		regStatus.latched = regStatus.current;
		regStatus.latched.sem.v = 0;
		regStatus.latched.sem.lastReadLsb = 0;
		regStatus.hasLatch = true;
		registers[PPUREG_ADDR] = 0;
		registers[PPUREG_DATA] = 0;
		vramAddr = 0;

		return regStatus.current.raw;
	}


	inline uint8_t& OAMADDR( const uint8_t value )
	{
		assert( value == 0x00 ); // TODO: implement other modes

		registers[PPUREG_OAMADDR] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return registers[PPUREG_OAMADDR];
	}


	inline uint8_t& OAMADDR()
	{
		return registers[PPUREG_OAMADDR];
	}


	inline uint8_t& OAMDATA( const uint8_t value )
	{
		assert(0);

		registers[PPUREG_OAMDATA] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return registers[PPUREG_OAMDATA];
	}


	inline uint8_t& OAMDATA()
	{
		return registers[PPUREG_OAMDATA];
	}


	inline uint8_t& PPUSCROLL( const uint8_t value )
	{
		assert( value == 0 );
		registers[PPUREG_SCROLL] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		return registers[PPUREG_SCROLL];
	}


	inline uint8_t& PPUSCROLL()
	{
		return registers[PPUREG_SCROLL];
	}


	inline uint8_t& PPUADDR( const uint8_t value )
	{
		registers[PPUREG_ADDR] = value;

		regStatus.current.sem.lastReadLsb = ( value & 0x1F );

		vramAddr = ( vramAddr << 8 ) | ( value );

		return registers[PPUREG_ADDR];
	}


	inline uint8_t& PPUADDR()
	{
		return registers[PPUREG_ADDR];
	}


	inline uint8_t& PPUDATA( const uint8_t value )
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


	inline uint8_t& PPUDATA()
	{
		return registers[PPUREG_DATA];
	}

	uint8_t DoDMA( const uint16_t address );
	void EnablePrinting();

	inline uint8_t& OAMDMA( const uint8_t value )
	{
		GenerateDMA();
		DoDMA( value );

		return registers[PPUREG_OAMDMA];
	}


	inline uint8_t& OAMDMA()
	{
		GenerateDMA();
		return registers[PPUREG_OAMDMA];
	}


	inline void RenderScanline()
	{
		/*
		Memory fetch phase 1 thru 128
		---------------------------- -
		1. Name table uint8_t
		2. Attribute table uint8_t
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


	uint8_t& Reg( uint16_t address, uint8_t value );
	uint8_t& Reg( uint16_t address );
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

