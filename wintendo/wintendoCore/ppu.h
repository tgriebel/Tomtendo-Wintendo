#pragma once

struct PPU;

typedef uint8_t&( PPU::* PpuRegWriteFunc )( const uint8_t value );
typedef uint8_t&( PPU::* PpuRegReadFunc )();

union Pixel;
struct RGBA;

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
		Nametable		nameTableId		: 2;
		VramInc			vramInc			: 1;
		PatternTable	spriteTableId	: 1;
	
		PatternTable	bgTableId		: 1;
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
		uint8_t	lastReadLsb		: 5;
		uint8_t	spriteOverflow	: 1;
		uint8_t	spriteHit		: 1;
		uint8_t	vBlank			: 1;

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
	static const uint32_t VirtualMemorySize			= 0x10000;
	static const uint32_t PhysicalMemorySize		= 0x4000;
	static const uint32_t OamSize					= 0x0100;
	static const uint32_t OamSecondSize				= 0x0020;
	static const uint32_t NametableMemorySize		= 0x03C0;
	static const uint32_t AttributeTableMemorySize	= 0x0040;
	static const uint32_t NameTableAttribMemorySize	= NametableMemorySize + AttributeTableMemorySize;
	static const uint16_t NameTable0BaseAddr		= 0x2000;
	static const uint16_t PaletteBaseAddr			= 0x3F00;
	static const uint32_t NameTableWidthTiles		= 32;
	static const uint32_t NameTableHeightTiles		= 30;
	static const uint32_t AttribTableWidthTiles		= 8;
	static const uint32_t AttribTableWHeightTiles	= 8;
	static const uint32_t NtTilesPerAttribute		= 4;
	static const uint32_t NameTableTilePixels		= 8;
	static const uint32_t NameTableWidthPixels		= NameTableWidthTiles * NameTableTilePixels;
	static const uint32_t NameTableHeightPixels		= NameTableHeightTiles * NameTableTilePixels;
	static const uint32_t RegisterCount				= 8;
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
	const RGBA* palette;

	uint16_t vramAddr;

	uint8_t vram[VirtualMemorySize];
	uint8_t primaryOAM[OamSize];
	uint8_t secondaryOAM[OamSecondSize];

	uint8_t scrollPosition[2];
	uint8_t scrollReg = 0x00;

	uint8_t registers[9]; // no need?

	uint8_t& PPUCTRL( const uint8_t value );
	uint8_t& PPUCTRL();

	uint8_t& PPUMASK( const uint8_t value );
	uint8_t& PPUMASK();

	uint8_t& PPUSTATUS( const uint8_t value );
	uint8_t& PPUSTATUS();

	uint8_t& OAMADDR( const uint8_t value );
	uint8_t& OAMADDR();

	uint8_t& OAMDATA( const uint8_t value );
	uint8_t& OAMDATA();

	uint8_t& PPUSCROLL( const uint8_t value );
	uint8_t& PPUSCROLL();

	uint8_t& PPUADDR( const uint8_t value );
	uint8_t& PPUADDR();

	uint8_t& PPUDATA( const uint8_t value );
	uint8_t& PPUDATA();

	uint8_t& OAMDMA( const uint8_t value );
	uint8_t& OAMDMA();

	void RenderScanline();

	uint8_t GetBgPatternTableId();
	uint8_t GetSpritePatternTableId();
	uint8_t GetNameTableId();

	void FrameBufferWritePixel( const uint32_t x, const uint32_t y, const Pixel pixel );
	void DrawTile( uint32_t imageBuffer[], const WtRect& imageRect, const WtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId, bool scroll );
	void DrawSprite( const uint32_t tableId );

	void GenerateNMI();
	void GenerateDMA();
	uint8_t DMA( const uint16_t address );

	void EnablePrinting();

	bool vramWritePending;

	uint16_t MirrorVramAddr( uint16_t addr );
	uint8_t ReadMem( const uint16_t addr );
	uint8_t GetNtTileForPoint( const uint32_t ntId, WtPoint point );
	uint8_t GetNtTile( const uint32_t ntId, const WtPoint& tileCoord );
	uint8_t GetArribute( const uint32_t ntId, const WtPoint& tileCoord );
	uint8_t GetTilePaletteId( const uint32_t ntId, const WtPoint& tileCoord );

	void WriteVram();

	const ppuCycle_t Exec();
	bool Step( const ppuCycle_t& nextCycle );

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

