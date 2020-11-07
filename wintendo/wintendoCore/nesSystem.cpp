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


void wtSystem::DebugPrintFlushLog()
{
#if DEBUG_ADDR == 1
	if ( cpu.logFrameCount == 0 )
	{
		for( InstrDebugInfo& dbgInfo : cpu.dbgMetrics )
		{
			string dbgString;
			dbgInfo.ToString( dbgString );
			cpu.logFile << dbgString << endl;
		}
	}
#endif
}


int wtSystem::InitSystem( const wstring& filePath )
{
	LoadNesFile( filePath, cart );

	ppu.Reset();
	ppu.system = this;

	apu.Reset();
	apu.system = this;

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
	cpu.Reset(); // TODO: move this
	cpu.system = this;

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
	if ( IsPpuRegister( address ) )
	{
		return ppu.ReadReg( address );
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
	else if ( IsApuRegister( address ) )
	{
		return apuDummyRegister;
	}
	else
	{
		return memory[MirrorAddress( address )];
	}
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
	const uint32_t lastFrameNumber = finishedFrame.first;

	outFrameResult.frameBuffer		= frameBuffer[lastFrameNumber];
	outFrameResult.nameTableSheet	= nameTableSheet;
	outFrameResult.paletteDebug		= paletteDebug;
	outFrameResult.patternTable0	= patternTable0;
	outFrameResult.patternTable1	= patternTable1;

	finishedFrame.second = false;
	frameBuffer[lastFrameNumber].locked = false;

	GetState( outFrameResult.state );
#if DEBUG_ADDR
	outFrameResult.dbgMetrics = cpu.dbgMetrics[0];
#endif
	outFrameResult.dbgInfo = dbgInfo;
	outFrameResult.romHeader = cart.header;
	outFrameResult.mirrorMode = static_cast<wtMirrorMode>( GetMirrorMode() );
	outFrameResult.mapperId = GetMapperId();

	outFrameResult.soundOutput.locked = false;
	if( apu.frameOutput != nullptr )
	{
		outFrameResult.soundOutput = *apu.frameOutput;
		apu.GetDebugInfo( outFrameResult.apuDebug );
		apu.frameOutput->locked = false;
		apu.frameOutput = nullptr;
	}

	outFrameResult.currentFrame = frameNumber;
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


void wtSystem::InitConfig()
{
	// PPU
	config.ppu.chrPalette		= 0;
	// APU
	config.apu.frequencyScale	= 1.0f;
	config.apu.volume			= 1.0f;
	config.apu.waveShift		= 0;
	config.apu.disableSweep		= false;
	config.apu.disableEnvelope	= false;
}


void wtSystem::GetConfig( wtConfig& systemConfig )
{
	// PPU
	systemConfig.ppu.chrPalette		= config.ppu.chrPalette;
	// APU
	systemConfig.apu.frequencyScale	= config.apu.frequencyScale;
	systemConfig.apu.volume			= config.apu.volume;
	systemConfig.apu.waveShift		= config.apu.waveShift;
	systemConfig.apu.disableSweep	= config.apu.disableSweep;
	systemConfig.apu.disableEnvelope = config.apu.disableEnvelope;
}


void wtSystem::SyncConfig( wtConfig& systemConfig )
{
	// PPU
	config.ppu.chrPalette		= systemConfig.ppu.chrPalette;
	// APU
	config.apu.frequencyScale	= systemConfig.apu.frequencyScale;
	config.apu.volume			= systemConfig.apu.volume;
	config.apu.waveShift		= systemConfig.apu.waveShift;
	config.apu.disableSweep		= systemConfig.apu.disableSweep;
	config.apu.disableEnvelope	= systemConfig.apu.disableEnvelope;
}


bool wtSystem::Run( const masterCycles_t& nextCycle )
{
	bool isRunning = true;

	static const masterCycles_t ticks( CpuClockDivide );

#if DEBUG_ADDR == 1
	if( ( cpu.logFrameCount > 0 ) && !cpu.logFile.is_open() )
	{
		cpu.logFile.open( "tomTendo.log" );
	}
#endif

	// cpu.Begin(); // TODO
	// ppu.Begin(); // TODO
	apu.Begin();

	// TODO: CHECK WRAP AROUND LOGIC
	while ( ( sysCycles < nextCycle ) && isRunning )
	{
		sysCycles += ticks;

		isRunning = cpu.Step( chrono::duration_cast<cpuCycle_t>( sysCycles ) );
		ppu.Step( chrono::duration_cast<ppuCycle_t>( sysCycles ) );
		apu.Step( cpu.cycle );
	}

	// cpu.End(); // TODO
	// ppu.End(); // TODO
	apu.End();

#if DEBUG_MODE == 1
	dbgInfo.masterCpu = chrono::duration_cast<masterCycles_t>( cpu.cycle );
	dbgInfo.masterPpu = chrono::duration_cast<masterCycles_t>( ppu.cycle );
	dbgInfo.masterApu = chrono::duration_cast<masterCycles_t>( apu.cpuCycle );
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


string wtSystem::GetPrgBankDissambly( const uint8_t bankNum )
{
	std::stringstream debugStream;
	uint8_t* bankMem = cart.GetPrgRomBank( bankNum );
	uint16_t curByte = 0;

	while( curByte < KB_16 )
	{
		stringstream hexString;
		const uint32_t instrAddr = curByte;
		const uint32_t opCode = bankMem[curByte];

		IntrInfo instrInfo = cpu.instLookup[opCode];
		const uint32_t operandCnt = instrInfo.operands;
		const char* mnemonic = instrInfo.mnemonic;

		if ( operandCnt == 1 )
		{
			const uint32_t op0 = bankMem[curByte + 1];
			hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << opCode << " " << setw( 2 ) << op0;
		}
		else if ( operandCnt == 2 )
		{
			const uint32_t op0 = bankMem[curByte + 1];
			const uint32_t op1 = bankMem[curByte + 2];
			hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << opCode << " " << setw( 2 ) << op0 << " " << setw( 2 ) << op1;
		}
		else
		{
			hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << opCode;
		}

		debugStream << "0x" << right << uppercase << setfill( '0' ) << setw( 4 ) << hex << instrAddr << setfill( ' ' ) << "  " << setw( 10 ) << left << hexString.str() << mnemonic << std::endl;

		curByte += 1 + operandCnt;
		assert( curByte <= ( KB_16 + 1 ) );
	}

	return debugStream.str();
}


void wtSystem::GenerateRomDissambly( string prgRomAsm[16] )
{
	assert( cart.header.prgRomBanks <= 16 );
	for( uint32_t bankNum = 0; bankNum < cart.header.prgRomBanks; ++bankNum )
	{
		prgRomAsm[bankNum] = GetPrgBankDissambly( bankNum );
	}
}


void wtSystem::GenerateChrRomTables( wtPatternTableImage chrRom[16] )
{
	const uint16_t baseAddr = ( config.ppu.chrPalette > 3 ) ? PPU::SpritePaletteAddr: PPU::PaletteBaseAddr;
	const uint16_t baseOffset = 4 * ( config.ppu.chrPalette % 4 );
	const uint8_t p0 = ppu.ReadVram( baseAddr + baseOffset + 0 );
	const uint8_t p1 = ppu.ReadVram( baseAddr + baseOffset + 1 );
	const uint8_t p2 = ppu.ReadVram( baseAddr + baseOffset + 2 );
	const uint8_t p3 = ppu.ReadVram( baseAddr + baseOffset + 3 );

	RGBA palette[4];
	palette[0] = ppu.palette[p0];
	palette[1] = ppu.palette[p1];
	palette[2] = ppu.palette[p2];
	palette[3] = ppu.palette[p3];

	assert( cart.header.chrRomBanks <= 16 );
	for ( uint32_t bankNum = 0; bankNum < cart.header.chrRomBanks; ++bankNum )
	{
		ppu.DrawDebugPatternTables( chrRom[bankNum], palette, bankNum );
	}
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

	++frameNumber;

	RGBA palette[4];
	for( uint32_t i = 0; i < 4; ++i )
	{
		palette[i] = ppu.palette[ppu.ReadVram( PPU::PaletteBaseAddr + i )];
	}

	ppu.DrawDebugPatternTables( patternTable0, palette, 0 );
	ppu.DrawDebugPatternTables( patternTable1, palette, 1 );

	bool debugNT = debugNTEnable && ( ( (int)frame.count() % 60 ) == 0 );
	if( debugNT )
		ppu.DrawDebugNametable( nameTableSheet );
	ppu.DrawDebugPalette( paletteDebug );

	DebugPrintFlushLog();

	return isRunning;
}