#include "stdafx.h"
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include "common.h"
#include <memory>
#include "NesSystem.h"
#include "mos6502.h"
#include "input.h"
#include "nesMapper.h"

using namespace std;

ButtonFlags keyBuffer[2] = { ButtonFlags::BUTTON_NONE, ButtonFlags::BUTTON_NONE };
wtPoint mousePoint;

typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_t;
timePoint_t previousTime;
bool lockFps = true;
frameRate_t frame( 1 );


static void LoadNesFile( const std::wstring& fileName, wtCart& outCart )
{
	std::ifstream nesFile;
	nesFile.open( fileName, std::ios::binary );

	assert( nesFile.good() );

	nesFile.seekg( 0, std::ios::end );
	size_t len = static_cast<size_t>( nesFile.tellg() );

	nesFile.seekg( 0, std::ios::beg );
	nesFile.read( reinterpret_cast<char*>( &outCart ), len );
	nesFile.close();

	assert( outCart.header.type[0] == 'N' );
	assert( outCart.header.type[1] == 'E' );
	assert( outCart.header.type[2] == 'S' );
	assert( outCart.header.magic == 0x1A );

	outCart.size = len - sizeof( outCart.header ); // TODO: trainer needs to be checked
}


int wtSystem::InitSystem( const wstring& filePath )
{
	LoadNesFile( filePath, cart );

	LoadProgram( cart );
	fileName = filePath;

	return 0;
}


void wtSystem::ShutdownSystem()
{
#if DEBUG_ADDR == 1
	cpu.logFile.close();
#endif // #if DEBUG_ADDR == 1
}


void wtSystem::LoadProgram( wtCart& loadCart, const uint32_t resetVectorManual )
{
	memset( memory, 0, VirtualMemorySize );

	loadCart.mapper = AssignMapper( loadCart.GetMapperId() );
	loadCart.mapper->system = this;
	loadCart.mapper->OnLoadCpu();
	loadCart.mapper->OnLoadPpu();

	if ( resetVectorManual == 0x10000 )
	{
		cpu.resetVector = ( memory[ResetVectorAddr + 1] << 8 ) | memory[ResetVectorAddr];
	}
	else
	{
		cpu.resetVector = static_cast< uint16_t >( resetVectorManual & 0xFFFF );
	}

	cpu.nmiVector = Combine( memory[NmiVectorAddr], memory[NmiVectorAddr + 1] );
	cpu.irqVector = Combine( memory[IrqVectorAddr], memory[IrqVectorAddr + 1] );

	cpu.interruptRequestNMI = false;
	cpu.system = this;
	cpu.Reset();

	if ( loadCart.header.controlBits0.fourScreenMirror )
	{
		mirrorMode = MIRROR_MODE_FOURSCREEN;
	}
	else if ( loadCart.header.controlBits0.mirror )
	{
		mirrorMode = MIRROR_MODE_VERTICAL;
	}
	else
	{
		mirrorMode = MIRROR_MODE_HORIZONTAL;
	}
}


bool wtSystem::IsInputRegister( const uint16_t address )
{
	return ( ( address == InputRegister0 ) || ( address == InputRegister1 ) );
}


bool wtSystem::IsPpuRegister( const uint16_t address )
{
	return ( address >= PpuRegisterBase ) && ( address < PpuRegisterEnd );
}


bool wtSystem::IsApuRegister( const uint16_t address )
{
	return ( address >= ApuRegisterBase ) && ( address < ApuRegisterEnd );
}


bool wtSystem::IsDMA( const uint16_t address )
{
	// TODO: port technically on CPU
	return ( address == PpuOamDma );
}


uint16_t wtSystem::MirrorAddress( const uint16_t address )
{
	if ( IsPpuRegister( address ) )
	{
		return ( address % PPU::RegisterCount ) + PpuRegisterBase;
	}
	else if ( IsDMA( address ) )
	{
		return address;
	}
	else if ( ( address >= PhysicalMemorySize ) && ( address < PpuRegisterBase ) )
	{
		return ( address % PhysicalMemorySize );
	}
	else
	{
		return ( address % MemoryWrap );
	}
}


uint8_t& wtSystem::GetStack()
{
	return memory[StackBase + cpu.SP];
}


uint8_t& wtSystem::GetMemoryRef( const uint16_t address )
{
	if ( IsPpuRegister( address ) )
	{
		return ppu.Reg( address );
	}
	else if ( IsInputRegister( address ) )
	{
		assert( InputRegister0 <= address );
		const uint32_t controllerIndex = ( address - InputRegister0 );
		const ControllerId controllerId = static_cast<ControllerId>( controllerIndex );

		controllerBuffer[controllerIndex] = 0; // FIXME: this register is a hack to get around bad GetMemory func design

		if ( strobeOn )
		{
			controllerBuffer[controllerIndex] = static_cast<uint8_t>( GetKeyBuffer( controllerId ) & static_cast<ButtonFlags>( 0X80 ) );
			btnShift[controllerIndex] = 0;

			return controllerBuffer[controllerIndex];
		}

		controllerBuffer[controllerIndex] = static_cast<uint8_t>( GetKeyBuffer( controllerId ) >> static_cast<ButtonFlags>( 7 - btnShift[controllerIndex] ) ) & 0x01;
		++btnShift[controllerIndex];
		btnShift[controllerIndex] %= 8;

		return controllerBuffer[controllerIndex];
	}
	else if( IsApuRegister( address ) )
	{
		return apuDummyRegister;
	}
	else
	{
		return memory[MirrorAddress( address )];
	}
}


uint8_t wtSystem::GetMapperId()
{
	return ( cart.header.controlBits1.mappedNumberUpper << 4 ) | cart.header.controlBits0.mapperNumberLower;
}


uint8_t wtSystem::GetMirrorMode()
{
	return mirrorMode;
}


bool wtSystem::MouseInRegion( const wtRect& region ) // TODO: draft code, kill later
{
	return ( ( mousePoint.x >= region.x ) && ( mousePoint.x < region.width ) && ( mousePoint.y >= region.y ) && ( mousePoint.y < region.height ) );
}


uint8_t wtSystem::GetMemory( const uint16_t address )
{
	return GetMemoryRef( address );
}


void wtSystem::WritePhysicalMemory( const uint16_t address, const uint8_t value )
{
	memory[MirrorAddress(address)] = value;
}


void wtSystem::WriteInput( const uint8_t value )
{

}


void wtSystem::CaptureInput( const Controller keys )
{
	controller.exchange( keys );
}


void wtSystem::GetFrameResult( wtFrameResult& outFrameResult )
{
	static_assert( sizeof( wtFrameResult ) == 1491256, "Update wtSystem::GetFrameResult()" );

	const uint32_t lastFrameNumber = finishedFrame.first;

	outFrameResult.frameBuffer		= frameBuffer[lastFrameNumber];
	outFrameResult.nameTableSheet	= nameTableSheet;
	outFrameResult.paletteDebug		= paletteDebug;
	outFrameResult.patternTable0	= patternTable0;
	outFrameResult.patternTable1	= patternTable1;

	finishedFrame.second = false; // TODO: make atomic
	frameBuffer[lastFrameNumber].locked = false;

	GetState( outFrameResult.state );
	outFrameResult.dbgMetrics = cpu.dbgMetrics[0];
	outFrameResult.dbgInfo = dbgInfo;
	outFrameResult.romHeader = cart.header;
	outFrameResult.mirrorMode = static_cast<wtMirrorMode>( GetMirrorMode() );
	outFrameResult.mapperId = GetMapperId();
}


void wtSystem::GetState( wtState& state )
{
	static_assert( sizeof( wtState ) == 131080, "Update wtSystem::GetState()" );
	static_assert( wtState::CpuMemorySize >= VirtualMemorySize, "wtSystem::GetState(): Buffer Overflow." );
	static_assert( wtState::PpuMemorySize >= PPU::VirtualMemorySize, "wtSystem::GetState(): Buffer Overflow." );

	state.A = cpu.A;
	state.X = cpu.X;
	state.Y = cpu.Y;
	state.P = cpu.P;
	state.PC = cpu.PC;
	state.SP = cpu.SP;
	memcpy( state.cpuMemory, memory, VirtualMemorySize );
	memcpy( state.ppuMemory, ppu.vram, PPU::VirtualMemorySize );
}


void wtSystem::SyncState( wtState& state )
{
	// TODO: Handle safe timing
	static_assert( sizeof( wtState ) == 131080, "Update wtSystem::SyncState()" );
	static_assert( VirtualMemorySize >= wtState::CpuMemorySize, "wtSystem::SyncState(): Buffer Overflow." );
	static_assert( PPU::VirtualMemorySize >= wtState::PpuMemorySize, "wtSystem::SyncState(): Buffer Overflow." );

	cpu.A = state.A;
	cpu.X = state.X;
	cpu.Y = state.Y;
	cpu.P = state.P;
	cpu.PC = state.PC;
	cpu.SP = state.SP;
	memcpy( memory, state.cpuMemory, VirtualMemorySize );
	memcpy( ppu.vram, state.ppuMemory, PPU::VirtualMemorySize );
}


bool wtSystem::Run( const masterCycles_t& nextCycle )
{
	bool isRunning = true;

	static const masterCycles_t ticks( CpuClockDivide );

#if DEBUG_MODE == 1
	if( ( cpu.logFrameCount > 0 ) && !cpu.logFile.is_open() )
	{
		cpu.logFile.open( "tomTendo.log" );
	}
	auto start = chrono::steady_clock::now();
#endif // #if DEBUG_MODE == 1

	// TODO: CHECK WRAP AROUND LOGIC
	while ( ( sysCycles < nextCycle ) && isRunning )
	{
		sysCycles += ticks;

		isRunning = cpu.Step( chrono::duration_cast<cpuCycle_t>( sysCycles ) );
		ppu.Step( chrono::duration_cast<ppuCycle_t>( cpu.cycle ) );
	}

#if DEBUG_MODE == 1
	auto end = chrono::steady_clock::now();

	auto elapsed = end - start;
	auto dur = chrono::duration <double, milli>( elapsed ).count();

	//	cout << "Elapsed:" << dur << ": Cycles: " << cpu.cycle.count() << endl;
#endif // #if DEBUG_MODE == 1

#if DEBUG_ADDR == 1
	if ( cpu.logFrameCount <= 0 )
	{
		cpu.logFile.close();
	}
	cpu.logFrameCount--;
#endif // #if DEBUG_ADDR == 1

	return isRunning;
}


int wtSystem::RunFrame()
{
	chrono::milliseconds frameTime = chrono::duration_cast<chrono::milliseconds>( frameRate_t( 1 ) );

	auto targetTime = previousTime + frameTime;
	auto currentTime = chrono::steady_clock::now();

	auto elapsed = targetTime - currentTime;
	auto dur = chrono::duration <double, nano>( elapsed ).count();

	if ( lockFps && ( elapsed.count() > 0 ) )
	{
		std::this_thread::sleep_for( std::chrono::nanoseconds( elapsed ) );
	}

	previousTime = chrono::steady_clock::now();

	masterCycles_t cyclesPerFrame = chrono::duration_cast<masterCycles_t>( frameTime );
	masterCycles_t nextCycle = chrono::duration_cast<masterCycles_t>( ++frame );

	auto startTime = chrono::steady_clock::now();

	bool isRunning = isRunning = Run( nextCycle );

	auto endTime = chrono::steady_clock::now();
	auto frameTimeUs = endTime - startTime;
	dbgInfo.frameTimeUs = static_cast<uint32_t>( chrono::duration_cast<chrono::microseconds>( frameTimeUs ).count() );

	if ( headless )
	{
		return isRunning;
	}

	if ( !isRunning )
	{
		return false;
	}

	wtRect imageRect = { 0, 0, wtSystem::ScreenWidth, wtSystem::ScreenHeight };

	bool debugNT = debugNTEnable && ( ( (int)frame.count() % 60 ) == 0 );

	if ( debugNT )
	{
		wtRect ntRects[4] = { { 0,						0,							2 * wtSystem::ScreenWidth,	2 * wtSystem::ScreenHeight },
								{ wtSystem::ScreenWidth,	0,							2 * wtSystem::ScreenWidth,	2 * wtSystem::ScreenHeight },
								{ 0,						wtSystem::ScreenHeight,	2 * wtSystem::ScreenWidth, 2 * wtSystem::ScreenHeight },
								{ wtSystem::ScreenWidth,	wtSystem::ScreenHeight,	2 * wtSystem::ScreenWidth, 2 * wtSystem::ScreenHeight }, };

		wtPoint ntCorners[4] = { {0,							0 },
									{ wtSystem::ScreenWidth,	0 },
									{ 0,						wtSystem::ScreenHeight },
									{ wtSystem::ScreenWidth,	wtSystem::ScreenHeight }, };

		for ( uint32_t ntId = 0; ntId < 4; ++ntId )
		{
			for ( int32_t tileY = 0; tileY < (int)PPU::NameTableHeightTiles; ++tileY )
			{
				for ( int32_t tileX = 0; tileX < (int)PPU::NameTableWidthTiles; ++tileX )
				{
					ppu.DrawTile( nameTableSheet, ntRects[ntId], wtPoint{ tileX, tileY }, ntId, ppu.GetBgPatternTableId() );

					ntRects[ntId].x += static_cast<uint32_t>( PPU::TilePixels );
				}

				ntRects[ntId].x = ntCorners[ntId].x;
				ntRects[ntId].y += static_cast<uint32_t>( PPU::TilePixels );
			}
		}
	}

	for ( int32_t tileY = 0; tileY < 16; ++tileY )
	{
		for ( int32_t tileX = 0; tileX < 16; ++tileX )
		{
			ppu.DrawTile( &patternTable0, wtRect{ (int32_t)PPU::TilePixels * tileX, (int32_t)PPU::TilePixels * tileY, PPU::PatternTableWidth, PPU::PatternTableHeight }, (int32_t)( tileX + 16 * tileY ), 0 );
			ppu.DrawTile( &patternTable1, wtRect{ (int32_t)PPU::TilePixels * tileX, (int32_t)PPU::TilePixels * tileY, PPU::PatternTableWidth, PPU::PatternTableHeight }, (int32_t)( tileX + 16 * tileY ), 1 );
		}
	}

	ppu.DrawDebugPalette( paletteDebug );

	static bool debugAttribs = false;

	if ( debugAttribs )
	{
		ofstream attribFile;

		stringstream attribName;
		attribName << "Palettes/" << "attrib_" << frame.count() << ".txt";

		attribFile.open( attribName.str() );

		attribFile << "Frame: " << frame.count() << endl;

		const uint16_t attribAddr = 0x23C0;

		for ( int attribY = 0; attribY < 8; ++attribY )
		{
			for ( int attribX = 0; attribX < 8; ++attribX )
			{
				const uint16_t attribTable = ppu.vram[attribAddr + attribY * 8 + attribX];
				const uint16_t tL = ( attribTable >> 6 ) & 0x03;
				const uint16_t tR = ( attribTable >> 4 ) & 0x03;
				const uint16_t bL = ( attribTable >> 2 ) & 0x03;
				const uint16_t bR = ( attribTable >> 0 ) & 0x03;

				attribFile << "[" << setfill( '0' ) << hex << setw( 2 ) << tL << " " << setw( 2 ) << tR << " " << setw( 2 ) << bL << " " << setw( 2 ) << bR << "]";
			}

			attribFile << endl;
		}

		attribFile << endl;

		attribFile.close();
	}

	/*
	Pixel pixel;
	pixel.rgba.red = 0xFF;
	pixel.rgba.alpha = 0xFF;
	frameBuffer[currentFrame].SetPixel( mousePoint.x, mousePoint.y, pixel );
	*/

	return isRunning;
}