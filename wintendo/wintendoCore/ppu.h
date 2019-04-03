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
		Nametable		ntId			: 2;
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


union PpuScrollReg // Internal register T and V
{
	struct iRegScrollSemantic
	{
		uint16_t coarseX	: 5;
		uint16_t coarseY	: 5;
		uint16_t ntId		: 2;
		uint16_t fineY		: 3;
		uint16_t unused		: 1;
	} sem;

	uint16_t raw;
};


struct PpuScrollRegisters
{
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


struct PpuAttribPaletteId
{
	struct PpuAttribSemantic
	{;
		uint8_t topLeft		: 2;
		uint8_t topRight	: 2;
		uint8_t bottomLeft	: 2;
		uint8_t bottomRight	: 2;
	} sem;

	uint8_t raw;
};


struct PpuSpriteAttrib
{
	uint8_t x;
	uint8_t y;
	uint8_t tileId;

	uint8_t palette;
	uint8_t priority;
	uint8_t flippedHorizontal;
	uint8_t flippedVertical;
};


struct OamPipeLineData
{
	uint8_t primaryOAM[4];

};


struct TilePipeLineData
{
	uint32_t flags;
	uint8_t tileId;
	uint8_t attribId;
	uint8_t chrRom0;
	uint8_t chrRom1;
};


enum PpuScanLine
{
	POSTRENDER_SCANLINE = 240,
	PRERENDER_SCANLINE = 261,
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
	static const uint16_t AttribTable0BaseAddr		= NameTable0BaseAddr + NametableMemorySize;
	static const uint16_t PaletteBaseAddr			= 0x3F00;
	static const uint16_t SpritePaletteAddr			= 0x3F10;
	static const uint16_t TotalSprites				= 64;
	static const uint16_t SecondarySprites			= 8;
	static const uint32_t NameTableWidthTiles		= 32;
	static const uint32_t NameTableHeightTiles		= 30;
	static const uint32_t AttribTableWidthTiles		= 8;
	static const uint32_t AttribTableWHeightTiles	= 8;
	static const uint32_t NtTilesPerAttribute		= 4;
	static const uint32_t NameTableTilePixels		= 8;
	static const uint32_t NameTableWidthPixels		= NameTableWidthTiles * NameTableTilePixels;
	static const uint32_t NameTableHeightPixels		= NameTableHeightTiles * NameTableTilePixels;
	static const uint32_t RegisterCount				= 8;
	static const uint32_t ScanlineCycles			= 341;
	//static const ppuCycle_t VBlankCycles = ppuCycle_t( 20 * 341 * 5 );

	PpuCtrl regCtrl;
	PpuMask regMask;
	PpuStatus regStatus;

	bool genNMI;

	int currentScanline;
	ppuCycle_t scanelineCycle;

	WtPoint beamPosition;

	bool loadingSecondingOAM;
	OamPipeLineData primaryOamSpriteData;

	bool nmiOccurred;

	ppuCycle_t cycle;

	NesSystem* system;
	const RGBA* palette;

	bool debugPrefetchTiles = false;

	bool vramWritePending;
	bool vramAccessed = false;

	// Internal registers
	PpuScrollReg regV;
	PpuScrollReg regT;
	uint16_t regX = 0x0000;
	uint16_t regW = 0x0000;

	uint8_t vram[VirtualMemorySize];
	uint8_t primaryOAM[OamSize];
	uint8_t secondaryOAM[OamSize/*OamSecondSize*/];
	bool sprite0InList; // In secondary OAM

	uint8_t ppuReadBuffer[2];

	bool inVBlank; // This is for internal state tracking not for reporting to the CPU

	TilePipeLineData plLatches;
	TilePipeLineData plShifts[2];
	uint16_t chrShifts[2];
	uint8_t palShifts[2];
	uint8_t palLatch[2];
	uint8_t chrLatch[2];

	uint8_t curShift;
	uint16_t attrib;

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

	void BgPipelineShiftRegisters();
	void BgPipelineDebugPrefetchFetchTiles();
	uint8_t BgPipelineDecodePalette();
	void BgPipelineFetch( const uint64_t cycle );
	void AdvanceXScroll( const uint64_t cycleCount );
	void AdvanceYScroll( const uint64_t cycleCount );

	void IncRenderAddr();

	bool BgDataFetchEnabled();
	bool RenderEnabled();
	bool DataportEnabled();
	bool InVBlank();

	uint8_t GetBgPatternTableId();
	uint8_t GetSpritePatternTableId();
	uint8_t GetNameTableId();

	void FrameBufferWritePixel( const uint32_t x, const uint32_t y, const Pixel pixel );
	void DrawPixel( uint32_t imageBuffer[], const WtRect& imageRect );
	void DrawBlankScanline( uint32_t imageBuffer[], const WtRect& imageRect, const uint8_t scanY );
	void DrawTile( uint32_t imageBuffer[], const WtRect& imageRect, const WtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId );
	void DrawSpritePixel( uint32_t imageBuffer[], const WtRect& imageRect, const PpuSpriteAttrib attribs, const WtPoint& point, const uint8_t bgPixel, bool sprite0 );
	void DrawSprites( const uint32_t tableId );
	void DrawDebugPalette( uint32_t imageBuffer[] );

	void GenerateNMI();
	void GenerateDMA();
	uint8_t DMA( const uint16_t address );

	uint16_t MirrorVram( uint16_t addr );
	uint8_t ReadVram( const uint16_t addr );
	uint8_t GetNtTile( const uint32_t ntId, const WtPoint& tileCoord );
	uint8_t GetArribute( const uint32_t ntId, const WtPoint& tileCoord );
	uint8_t GetTilePaletteId( const uint32_t attribTable, const WtPoint& tileCoord );
	uint8_t GetChrRom( const uint32_t tileId, const uint8_t plane, const uint8_t ptrnTableId, const uint8_t row );

	void LoadSecondaryOAM();
	PpuSpriteAttrib GetSpriteData( const uint8_t spriteId, const uint8_t oam[] );

	static uint8_t GetChrRomPalette( const uint8_t plane0, const uint8_t plane1, const uint8_t col );

	void WriteVram();

	const ppuCycle_t Exec();
	bool Step( const ppuCycle_t& nextCycle );

	PPU()
	{
		cycle = ppuCycle_t( 0 );

		currentScanline = PRERENDER_SCANLINE;
		scanelineCycle = ppuCycle_t( 0 );

		regCtrl.raw = 0;
		regMask.raw = 0;
		regStatus.current.raw = 0;
		regStatus.latched.raw = 0;
		regStatus.hasLatch = false;

		vramWritePending = false;

		beamPosition.x = 0;
		beamPosition.y = 0;

		regT.raw = 0;
		regV.raw = 0;

		curShift = 0;

		inVBlank = true;
	}


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

