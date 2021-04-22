#pragma once
#include "cart.h"

union Pixel;
struct RGBA;

static const RGBA DefaultPalette[64] =
{
	{ 0x75, 0x75, 0x75, 0xFF }, { 0x27, 0x1B, 0x8F, 0xFF }, { 0x00, 0x00, 0xAB, 0xFF }, { 0x47, 0x00, 0x9F, 0xFF },
	{ 0x8F, 0x00, 0x77, 0xFF }, { 0xAB, 0x00, 0x13, 0xFF }, { 0xA7, 0x00, 0x00, 0xFF }, { 0x7F, 0x0B, 0x00, 0xFF },
	{ 0x43, 0x2F, 0x00, 0xFF }, { 0x00, 0x47, 0x00, 0xFF }, { 0x00, 0x51, 0x00, 0xFF }, { 0x00, 0x3F, 0x17, 0xFF },
	{ 0x1B, 0x3F, 0x5F, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },
	{ 0xBC, 0xBC, 0xBC, 0xFF }, { 0x00, 0x73, 0xEF, 0xFF }, { 0x23, 0x3B, 0xEF, 0xFF }, { 0x83, 0x00, 0xF3, 0xFF },
	{ 0xBF, 0x00, 0xBF, 0xFF }, { 0xE7, 0x00, 0x5B, 0xFF }, { 0xDB, 0x2B, 0x00, 0xFF }, { 0xCB, 0x4F, 0x0F, 0xFF },
	{ 0x8B, 0x73, 0x00, 0xFF }, { 0x00, 0x97, 0x00, 0xFF }, { 0x00, 0xAB, 0x00, 0xFF }, { 0x00, 0x93, 0x3B, 0xFF },
	{ 0x00, 0x83, 0x8B, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },
	{ 0xFF, 0xFF, 0xFF, 0xFF }, { 0x3F, 0xBF, 0xFF, 0xFF }, { 0x5F, 0x97, 0xFF, 0xFF }, { 0xA7, 0x8B, 0xFD, 0xFF },
	{ 0xF7, 0x7B, 0xFF, 0xFF }, { 0xFF, 0x77, 0xB7, 0xFF }, { 0xFF, 0x77, 0x63, 0xFF }, { 0xFF, 0x9B, 0x3B, 0xFF },
	{ 0xF3, 0xBF, 0x3F, 0xFF }, { 0x83, 0xD3, 0x13, 0xFF }, { 0x4F, 0xDF, 0x4B, 0xFF }, { 0x58, 0xF8, 0x98, 0xFF },
	{ 0x00, 0xEB, 0xDB, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },
	{ 0xFF, 0xFF, 0xFF, 0xFF }, { 0xAB, 0xE7, 0xFF, 0xFF }, { 0xC7, 0xD7, 0xFF, 0xFF }, { 0xD7, 0xCB, 0xFF, 0xFF },
	{ 0xFF, 0xC7, 0xFF, 0xFF }, { 0xFF, 0xC7, 0xDB, 0xFF }, { 0xFF, 0xBF, 0xB3, 0xFF }, { 0xFF, 0xDB, 0xAB, 0xFF },
	{ 0xFF, 0xE7, 0xA3, 0xFF }, { 0xE3, 0xFF, 0xA3, 0xFF }, { 0xAB, 0xF3, 0xBF, 0xFF }, { 0xB3, 0xFF, 0xCF, 0xFF },
	{ 0x9F, 0xFF, 0xF3, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },
};

enum ppuReg_t : uint8_t
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


union ppuCtrl
{
	struct semantic
	{
		uint8_t	ntId			: 2;
		uint8_t	vramInc			: 1;
		uint8_t	spriteTableId	: 1;
	
		uint8_t	bgTableId		: 1;
		uint8_t	sprite8x16Mode	: 1;
		uint8_t	masterSlaveMode	: 1;
		uint8_t	nmiVblank		: 1;
	} sem;

	uint8_t byte;
};



union ppuMask_t
{
	struct semantic
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

	uint8_t byte;
};


union ppuScroll_t // Internal register T and V
{
	struct semantic
	{
		uint16_t coarseX	: 5;
		uint16_t coarseY	: 5;
		uint16_t ntId		: 2;
		uint16_t fineY		: 3;
		uint16_t unused		: 1;
	} sem;

	uint16_t byte2x;
};


union ppuStatusLatch_t
{
	struct semantic
	{
		uint8_t	lastReadLsb		: 5;
		uint8_t	spriteOverflow	: 1;
		uint8_t	spriteHit		: 1;
		uint8_t	vBlank			: 1;

	} sem;

	uint8_t byte;
};


struct ppuStatus_t
{
	bool				hasLatch;
	ppuStatusLatch_t	current;
	ppuStatusLatch_t	latched;
};


struct ppuAttribPaletteId_t
{
	struct semantic
	{;
		uint8_t topLeft		: 2;
		uint8_t topRight	: 2;
		uint8_t bottomLeft	: 2;
		uint8_t bottomRight	: 2;
	} sem;

	uint8_t byte;
};


struct spriteAttrib_t
{
	uint8_t	x;
	uint8_t	y;
	uint8_t	tileId;
	uint8_t	palette;
	uint8_t	oamIndex;
	uint8_t	secondaryOamIndex;	// debugging
	uint8_t	tableId;			// debugging
	uint8_t	flippedHorizontal	: 1;
	uint8_t	flippedVertical		: 1;
	uint8_t	priority			: 1;
	bool	sprite0				: 1;
	bool	is8x16				: 1;
};


struct pipelineData_t
{
	uint32_t flags;
	uint8_t tileId;
	uint8_t attribId;
	uint8_t chrRom0;
	uint8_t chrRom1;
};


enum ppuScanLine_t
{
	POSTRENDER_SCANLINE	= 240,
	PRERENDER_SCANLINE	= 261,
};


union ppuImageIx_t
{
	struct point_t
	{
		uint8_t x;
		uint8_t y;
	} point;
	uint16_t index;
};


struct ppuDebug_t
{
	spriteAttrib_t	spritePicked;
};


class PPU
{
public:
	static const uint32_t VirtualMemorySize			= 0x10000;
	static const uint32_t PhysicalMemorySize		= 0x4000;
	static const uint32_t OamSize					= 0x0100;
	static const uint32_t OamSecondSize				= 0x0020;
	static const uint32_t NametableMemorySize		= 0x03C0;
	static const uint32_t PatternTableMemorySize	= 0x2000;
	static const uint32_t AttributeTableMemorySize	= 0x0040;
	static const uint32_t NameTableAttribMemorySize	= 0x0400;
	static const uint32_t PatternTable0BaseAddr		= 0x0000;
	static const uint32_t PatternTable1BaseAddr		= 0x1000;
	static const uint16_t NameTable0BaseAddr		= 0x2000;
	static const uint16_t AttribTable0BaseAddr		= 0x23C0;
	static const uint16_t NameTable1BaseAddr		= 0x2400;
	static const uint16_t AttribTable1BaseAddr		= 0x27C0;
	static const uint16_t NameTable2BaseAddr		= 0x2800;
	static const uint16_t AttribTable2BaseAddr		= 0x2BC0;
	static const uint16_t NameTable3BaseAddr		= 0x2C00;
	static const uint16_t AttribTable3BaseAddr		= 0x2FC0;
	static const uint16_t PaletteBaseAddr			= 0x3F00;
	static const uint16_t SpritePaletteAddr			= 0x3F10;
	static const uint16_t TotalSprites				= 64;
	static const uint16_t SecondarySprites			= 8;
	static const uint32_t NameTableWidthTiles		= 32;
	static const uint32_t NameTableHeightTiles		= 30;
	static const uint32_t AttribTableWidthTiles		= 8;
	static const uint32_t AttribTableWHeightTiles	= 8;
	static const uint32_t NtTilesPerAttribute		= 4;
	static const uint32_t TilePixels				= 8;
	static const uint32_t NameTableWidthPixels		= NameTableWidthTiles * TilePixels;
	static const uint32_t NameTableHeightPixels		= NameTableHeightTiles * TilePixels;
	static const uint32_t PaletteColorNumber		= 16;
	static const uint32_t PaletteSetNumber			= 2;
	static const uint32_t PatternTableWidth			= 128;
	static const uint32_t PatternTableHeight		= 128;
	static const uint32_t RegisterCount				= 8;
	static const uint32_t ScanlineCycles			= 341;
	static const uint32_t ScreenWidth				= 256;
	static const uint32_t ScreenHeight				= 240;
	//static const ppuCycle_t VBlankCycles = ppuCycle_t( 20 * 341 * 5 );

	ppuDebug_t		dbgInfo;
	const RGBA*		palette;
	uint8_t			nt[ KB(2) ];
	uint8_t			imgPal[ PPU::PaletteColorNumber ];
	uint8_t			sprPal[ PPU::PaletteColorNumber ];

private:
	wtSystem*		system;
	ppuCycle_t		cycle;
	int32_t			currentScanline;

	ppuCtrl			regCtrl;
	ppuMask_t		regMask;
	ppuStatus_t		regStatus;

	ppuImageIx_t	beam;

	bool			debugPrefetchTiles = false;

	bool			genNMI;
	bool			nmiOccurred;
	bool			vramWritePending;
	bool			vramAccessed;

	// Internal registers
	ppuScroll_t		regV;
	ppuScroll_t		regT;
	uint16_t		regX;
	uint16_t		regW;

	uint32_t		debugVramWriteCounter[VirtualMemorySize];
	uint8_t			primaryOAM[OamSize];
	spriteAttrib_t	secondaryOAM[OamSize];
	uint8_t			secondaryOamSpriteCnt;

	uint8_t			ppuReadBuffer;

	bool			inVBlank; // This is for internal state tracking not for reporting to the CPU

	pipelineData_t	plLatches;
	pipelineData_t	plShifts[2];
	uint16_t		chrShifts[2];
	uint8_t			palShifts[2];
	uint8_t			palLatch[2];
	uint8_t			chrLatch[2];

	uint8_t			curShift;
	uint16_t		attrib;

	uint8_t			registers[9]; // no need?
	uint16_t		MirrorMap[MIRROR_MODE_COUNT][VirtualMemorySize];

public:
	void			IssueDMA( const uint8_t value );

	void			WriteReg( const uint16_t addr, const uint8_t value );
	uint8_t			ReadReg( uint16_t address );

	void			DrawDebugPatternTables( wtPatternTableImage& imageBuffer, const RGBA dbgPalette[4], const uint32_t tableID, const bool isCartbank );
	void			DrawDebugObject( wtRawImageInterface* imageBuffer, const RGBA dbgPalette[ 4 ], const spriteAttrib_t& attrib );
	void			DrawDebugNametable( wtNameTableImage& nameTableSheet );
	void			DrawDebugPalette( wtPaletteImage& imageBuffer );

	void			WriteVram();
	uint8_t			ReadVram( const uint16_t addr );
	bool			IsMemoryMapped( const uint16_t addr ) const;
	ppuCycle_t		GetCycle() const;
	uint32_t		GetScanline() const;

	ppuCycle_t		Exec();
	bool			Step( const ppuCycle_t& nextCycle );	

	PPU()
	{
		palette = &DefaultPalette[0];
		Reset();
		GenerateMirrorMap();
	}

	void Reset()
	{
		cycle					= ppuCycle_t( 21 );  // FIXME: +21 is a hack to match test log, +7 on CPU

		genNMI					= false;
		attrib					= 0;
		chrLatch[0]				= 0;
		chrLatch[1]				= 0;
		chrShifts[0]			= 0;
		chrShifts[1]			= 0;
		vramAccessed			= false;
		nmiOccurred				= false;
		palLatch[0]				= 0;
		palLatch[1]				= 0;
		palShifts[0]			= 0;
		palShifts[1]			= 0;
		ppuReadBuffer			= 0;

		system					= nullptr;

		currentScanline			= 0;

		vramWritePending		= false;

		beam.point.x	= 0;
		beam.point.y	= 0;

		regT.byte2x				= 0;
		regV.byte2x				= 0;
		regX					= 0;
		regW					= 0;

		curShift				= 0;

		inVBlank				= true;

		regCtrl.byte			= 0;
		regMask.byte			= 0;
		regStatus.current.byte	= 0;
		regStatus.latched.byte	= 0;
		regStatus.hasLatch		= false;

		memset( secondaryOAM, 0, sizeof( secondaryOAM ) );
		memset( nt, 0, KB(2) );
		memset( imgPal, 0, PPU::PaletteColorNumber );
		memset( sprPal, 0, PPU::PaletteColorNumber );
		memset( debugVramWriteCounter, 0, VirtualMemorySize );
	}

	void			Begin();
	void			End();
	void			RegisterSystem( wtSystem* sys );

	void			Serialize( Serializer& serializer );

private:
	static uint8_t	GetChrRomPalette( const uint8_t plane0, const uint8_t plane1, const uint8_t col );

	uint8_t			GetChrRom8x8( const uint32_t tileId, const uint8_t plane, const uint8_t ptrnTableId, const uint8_t row );
	uint8_t			GetChrRom8x16( const uint32_t tileId, const uint8_t plane, const uint8_t row, const uint8_t isUpper );
	uint8_t			GetChrRomBank8x8( const uint32_t tileId, const uint8_t plane, const uint8_t bankId, const uint8_t row );

	void			DrawBlankScanline( wtDisplayImage& imageBuffer, const wtRect& imageRect, const uint8_t scanY );
	void			DrawTile( wtNameTableImage& imageBuffer, const wtRect& imageRect, const wtPoint& nametableTile, const uint32_t ntId, const uint32_t ptrnTableId );
	void			DrawChrRomTile( wtRawImageInterface* imageBuffer, const wtRect& imageRect, const RGBA palette[4], const uint32_t tileId, const uint32_t tableId, const bool cartBank, const bool is8x16 = false, const bool isUpper = false );
	bool			DrawSpritePixel( wtDisplayImage& fb, const spriteAttrib_t attribs, const ppuImageIx_t& index, const uint8_t bgPixel );

	bool			BgDataFetchEnabled();
	void			BgPipelineShiftRegisters();
	void			BgPipelineDebugPrefetchFetchTiles();
	uint8_t			BgPipelineDecodePalette();
	void			BgPipelineFetch( const uint64_t cycle );
	void			AdvanceXScroll();
	void			AdvanceYScroll();
	void			LoadSecondaryOAM();
	void			DMA( const uint16_t address );
	void			Render();
	spriteAttrib_t	GetSpriteData( const uint8_t spriteId, const uint8_t oam[] );

	uint8_t			GetBgPatternTableId();
	uint8_t			GetSpritePatternTableId();
	uint8_t			GetNameTableId();
	uint8_t			GetNtTile( const uint32_t ntId, const wtPoint& tileCoord );
	uint8_t			GetArribute( const uint32_t ntId, const wtPoint& tileCoord );
	uint8_t			GetTilePaletteId( const uint32_t attribTable, const wtPoint& tileCoord );

	uint16_t		StaticMirrorVram( uint16_t addr, uint32_t mirrorMode );
	uint16_t		MirrorVram( uint16_t addr );
	void			GenerateMirrorMap();

	bool			RenderEnabled();
	bool			DataportEnabled();
	bool			InVBlank();
	void			IncRenderAddr();

	// Write Registers
	void			PPUCTRL( const uint8_t value );
	void			PPUMASK( const uint8_t value );
	void			OAMADDR( const uint8_t value );
	void			OAMDATA( const uint8_t value );
	void			PPUSCROLL( const uint8_t value );
	void			PPUADDR( const uint8_t value );
	void			PPUDATA( const uint8_t value );

	// Read Registers
	uint8_t			PPUSTATUS();
	uint8_t			PPUDATA();
};