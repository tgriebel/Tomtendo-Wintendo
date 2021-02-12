#include "stdafx.h"
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include <wchar.h>
#include <memory>
#include "common.h"
#include "NesSystem.h"
#include "mos6502.h"
#include "input.h"
#include "mapper.h"
#include "timer.h"

using namespace std;

// TODO: remove globals
ButtonFlags keyBuffer[2] = { ButtonFlags::BUTTON_NONE, ButtonFlags::BUTTON_NONE };
wtPoint mousePoint;
bool lockFps = true;

static void LoadNesFile( const std::wstring& fileName, unique_ptr<wtCart>& outCart )
{
	// TODO: use serializer here
	std::ifstream nesFile;
	nesFile.open( fileName, std::ios::binary );

	assert( nesFile.good() );

	nesFile.seekg( 0, std::ios::end );
	uint32_t len = static_cast<uint32_t>( nesFile.tellg() );

	wtRomHeader header;
	uint32_t size = len - static_cast<uint32_t>( sizeof( header ) ); // TODO: trainer needs to be checked
	uint8_t* romData = new uint8_t[ size ];

	nesFile.seekg( 0, std::ios::beg );
	nesFile.read( reinterpret_cast<char*>( &header ), sizeof( header ) );
	nesFile.read( reinterpret_cast<char*>( romData ), size );
	nesFile.close();

	outCart = make_unique<wtCart>( header, romData, size );

	delete[] romData;
}


void wtSystem::DebugPrintFlushLog()
{
#if DEBUG_ADDR == 1
	if ( cpu.logFrameCount == 0 && cpu.logToFile )
	{
		for( OpDebugInfo& dbgInfo : cpu.dbgMetrics )
		{
			string dbgString;
			dbgInfo.ToString( dbgString );
			cpu.logFile << dbgString << endl;
		}
	}
#endif
}


int wtSystem::Init( const wstring& filePath )
{
	SaveSRam();

	LoadNesFile( filePath, cart );

	ppu.Reset();
	ppu.RegisterSystem( this );

	apu.Reset();
	apu.RegisterSystem( this );

	LoadProgram();
	fileName = filePath;

	const size_t offset = fileName.find( L".nes", 0 );
	baseFileName = fileName.substr( 0, offset );

	LoadSRam();

	return 0;
}


void wtSystem::Shutdown()
{
#if DEBUG_ADDR == 1
//	cpu.logFile.close();
#endif // #if DEBUG_ADDR == 1
}


void wtSystem::LoadProgram( const uint32_t resetVectorManual )
{
	memset( memory, 0, PhysicalMemorySize );

	cart->mapper = AssignMapper( cart->GetMapperId() );
	cart->mapper->system = this;
	cart->mapper->OnLoadCpu();
	cart->mapper->OnLoadPpu();

	if ( resetVectorManual == 0x10000 )	{	
		cpu.resetVector = Combine( ReadMemory( ResetVectorAddr ), ReadMemory( ResetVectorAddr + 1 ) );
	} else {
		cpu.resetVector = static_cast< uint16_t >( resetVectorManual & 0xFFFF );
	}

	cpu.nmiVector = Combine( ReadMemory( NmiVectorAddr ), ReadMemory( NmiVectorAddr + 1 ) );
	cpu.irqVector = Combine( ReadMemory( IrqVectorAddr ), ReadMemory( IrqVectorAddr + 1 ) );

	cpu.interruptRequestNMI = false;
	cpu.Reset(); // TODO: move this
	cpu.system = this;

	if ( cart->h.controlBits0.fourScreenMirror ) {
		mirrorMode = MIRROR_MODE_FOURSCREEN;
	} else if ( cart->h.controlBits0.mirror ) {
		mirrorMode = MIRROR_MODE_VERTICAL;
	} else {
		mirrorMode = MIRROR_MODE_HORIZONTAL;
	}
}


bool wtSystem::IsInputRegister( const uint16_t address )
{
	return ( ( address == InputRegister0 ) || ( address == InputRegister1 ) );
}


bool wtSystem::IsPpuRegister( const uint16_t address )
{
	return ( ( address >= PpuRegisterBase ) && ( address <= PpuRegisterEnd ) );
}


bool wtSystem::IsApuRegister( const uint16_t address )
{
	return ( ( address >= ApuRegisterBase ) && ( address <= ApuRegisterEnd ) || ( address == ApuRegisterCounter ) );
}


bool wtSystem::IsCartMemory( const uint16_t address )
{
	return InRange( address, ExpansionRomBase, 0xFFFF );
}


bool wtSystem::IsPhysicalMemory( const uint16_t address )
{
	return InRange( address, 0x0000, 0x1FFF );
}


bool wtSystem::IsDMA( const uint16_t address )
{
	// TODO: port technically on CPU
	return ( address == PpuOamDma );
}


uint16_t wtSystem::MirrorAddress( const uint16_t address ) const
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
	assert( ( StackBase + cpu.SP ) < PhysicalMemorySize );
	return memory[StackBase + cpu.SP];
}


uint8_t wtSystem::GetMapperId() const
{
	return ( cart->h.controlBits1.mappedNumberUpper << 4 ) | cart->h.controlBits0.mapperNumberLower;
}


uint8_t wtSystem::GetMirrorMode() const
{
	return mirrorMode;
}


bool wtSystem::MouseInRegion( const wtRect& region ) // TODO: draft code, kill later
{
	return ( ( mousePoint.x >= region.x ) && ( mousePoint.x < region.width ) && ( mousePoint.y >= region.y ) && ( mousePoint.y < region.height ) );
}


uint8_t wtSystem::ReadMemory( const uint16_t address )
{
	if( IsCartMemory( address ) )
	{
		return cart->mapper->ReadRom( address );
	}
	else if ( IsPpuRegister( address ) )
	{
		return ppu.ReadReg( address );
	}
	else if ( IsApuRegister( address ) )
	{
		return apu.ReadReg( address );
	}
	else if ( IsInputRegister( address ) ) // FIXME: APU will eat 0x4017
	{
		assert( InputRegister0 <= address );
		const uint32_t controllerIndex = ( address - InputRegister0 );
		const ControllerId controllerId = static_cast<ControllerId>( controllerIndex );

		uint8_t keyBuffer = 0;

		if ( strobeOn )
		{
			keyBuffer = static_cast<uint8_t>( GetKeyBuffer( controllerId ) & static_cast<ButtonFlags>( 0X80 ) );
			btnShift[controllerIndex] = 0;

			return keyBuffer;
		}

		keyBuffer = static_cast<uint8_t>( GetKeyBuffer( controllerId ) >> static_cast<ButtonFlags>( 7 - btnShift[controllerIndex] ) ) & 0x01;
		++btnShift[controllerIndex];
		btnShift[controllerIndex] %= 8;

		return keyBuffer;
	}
	else
	{
		assert( MirrorAddress( address ) < PhysicalMemorySize );
		return memory[MirrorAddress( address )];
	}
}


void wtSystem::WriteMemory( const uint16_t address, const uint16_t offset, const uint8_t value )
{
	if ( IsPpuRegister( address ) )
	{
		ppu.WriteReg( address, value );
	}
	else if ( wtSystem::IsDMA( address ) )
	{
		ppu.IssueDMA( value );
		apu.WriteReg( address, value );
	}
	else if ( IsInputRegister( address ) )
	{
		const bool prevStrobeOn = strobeOn;
		strobeOn = ( value & 0x01 );

		if ( prevStrobeOn && !strobeOn )
		{
			btnShift[0] = 0;
			btnShift[1] = 0;
		}
	}
	else if ( wtSystem::IsApuRegister( address ) ) // FIXME: the input will always eat 4017
	{
		apu.WriteReg( address, value );
	}
	else if ( cart->mapper->InWriteWindow( address, offset ) )
	{
		cart->mapper->Write( address, offset, value );
	}
	else
	{
		const uint32_t fullAddr = address + offset;
		WritePhysicalMemory( fullAddr, value );
	}
}


void wtSystem::WritePhysicalMemory( const uint16_t address, const uint8_t value )
{
	assert( MirrorAddress( address ) < PhysicalMemorySize );
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
	outFrameResult.ppuDebug			= ppu.dbgInfo;

	finishedFrame.second = false;
	frameBuffer[lastFrameNumber].locked = false;

	GetState( outFrameResult.state );
#if DEBUG_ADDR
	outFrameResult.dbgMetrics = &cpu.dbgMetrics;
#endif
	outFrameResult.dbgInfo = dbgInfo;
	outFrameResult.romHeader = cart->h;
	outFrameResult.mirrorMode = static_cast<wtMirrorMode>( GetMirrorMode() );
	outFrameResult.mapperId = GetMapperId();

	if( apu.frameOutput != nullptr )
	{
		outFrameResult.soundOutput = *apu.frameOutput;
		apu.GetDebugInfo( outFrameResult.apuDebug );
		apu.frameOutput = nullptr;
	}

	outFrameResult.currentFrame = frameNumber;
	outFrameResult.savedState = savedState;
	outFrameResult.loadedState = loadedState;
}


void wtSystem::GetState( wtState& state )
{
	static_assert( sizeof( wtState ) == 4104, "Update wtSystem::GetState()" );

	state.A = cpu.A;
	state.X = cpu.X;
	state.Y = cpu.Y;
	state.P = cpu.P;
	state.PC = cpu.PC;
	state.SP = cpu.SP;
	memcpy( state.cpuMemory, memory, wtState::CpuMemorySize );
	memcpy( state.ppuMemory, ppu.nt, KB_2 );
}


void wtSystem::InitConfig()
{
	// CPU
	config.cpu.traceFrameCount	= 0;
	config.cpu.requestLoadState = false;
	config.cpu.requestSaveState = false;

	// PPU
	config.ppu.chrPalette		= 0;
	config.ppu.showBG			= true;
	config.ppu.showSprite		= true;
	config.ppu.spriteLimit		= PPU::SecondarySprites;

	// APU
	config.apu.frequencyScale	= 1.0f;
	config.apu.volume			= 1.0f;
	config.apu.waveShift		= 0;
	config.apu.disableSweep		= false;
	config.apu.disableEnvelope	= false;
	config.apu.mutePulse1		= false;
	config.apu.mutePulse2		= false;
	config.apu.muteTri			= false;
	config.apu.muteNoise		= false;
	config.apu.muteDMC			= false;
}


void wtSystem::GetConfig( wtConfig& systemConfig )
{
	systemConfig = config;
}


void wtSystem::SyncConfig( wtConfig& systemConfig )
{
	config = systemConfig;
}


void wtSystem::RequestNMI() const
{
	cpu.interruptRequestNMI = true;
}


void wtSystem::RequestIRQ() const
{
	cpu.interruptRequest = true;
}


void wtSystem::RequestDMA() const
{
	cpu.oamInProcess = true;
}


void wtSystem::SaveSRam() const
{
	if( ( cart.get() != nullptr ) && cart->HasSave() )
	{
		uint8_t saveBuffer[ KB_2 ];
		for( int32_t i = 0; i < KB_2; ++i ) {
			saveBuffer[ i ] = cart->mapper->ReadRom( 0x6000 + i );
		}

		std::ofstream saveFile;
		saveFile.open( baseFileName + L".sav", ios::binary );
		saveFile.write( reinterpret_cast<char*>( saveBuffer ), KB_2 );
		saveFile.close();
	}
}


void wtSystem::LoadSRam()
{
	if ( ( cart.get() != nullptr ) && cart->HasSave() )
	{
		uint8_t saveBuffer[ KB_2 ];

		std::ifstream saveFile;
		saveFile.open( baseFileName + L".sav", ios::binary );

		if( !saveFile.good() ) {
			return;
		}

		saveFile.read( reinterpret_cast<char*>( saveBuffer ), KB_2 );
		saveFile.close();

		for ( int32_t i = 0; i < KB_2; ++i ) {
			cart->mapper->Write( 0x6000, i, saveBuffer[ i ] );
		}
	}
}


bool wtSystem::Run( const masterCycles_t& nextCycle )
{
	bool isRunning = true;

	static constexpr masterCycles_t ticks( CpuClockDivide );

#if DEBUG_ADDR == 1
	if( config.cpu.traceFrameCount ) {
		cpu.resetLog = true;
		cpu.logFrameCount = config.cpu.traceFrameCount;
	}

	if( cpu.resetLog ) {
		cpu.dbgMetrics.resize( 0 );
		cpu.resetLog = false;
	}

	if( ( cpu.logFrameCount > 0 ) && !cpu.logFile.is_open() && cpu.logToFile )
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
	if ( cpu.logFrameCount <= 0 ) {
		if( cpu.logToFile ) {
			cpu.logFile.close();
		}
	} else {
		cpu.logFrameCount--;
	}
#endif // #if DEBUG_ADDR == 1

	return isRunning;
}


string wtSystem::GetPrgBankDissambly( const uint8_t bankNum )
{
	std::stringstream debugStream;
	uint8_t* bankMem = cart->GetPrgRomBank( bankNum );
	uint16_t curByte = 0;

	while( curByte < KB_16 )
	{
		stringstream hexString;
		const uint32_t instrAddr = curByte;
		const uint32_t opCode = bankMem[curByte];

		opInfo_t instrInfo = cpu.opLUT[opCode];
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


void wtSystem::GenerateRomDissambly( string prgRomAsm[128] )
{
	assert( cart->h.prgRomBanks <= 128 );
	for( uint32_t bankNum = 0; bankNum < cart->h.prgRomBanks; ++bankNum )
	{
		prgRomAsm[bankNum] = GetPrgBankDissambly( bankNum );
	}
}


void wtSystem::GenerateChrRomTables( wtPatternTableImage chrRom[32] )
{
	assert( cart->GetChrBankCount() <= 32 );
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

	assert( cart->h.chrRomBanks <= 16 );
	for ( uint32_t bankNum = 0; bankNum < cart->h.chrRomBanks; ++bankNum )
	{
		ppu.DrawDebugPatternTables( chrRom[bankNum], palette, bankNum );
	}
}


int wtSystem::RunFrame()
{
	const timePoint_t currentTime = chrono::steady_clock::now();
	const std::chrono::nanoseconds elapsed = ( currentTime - previousTime );
	previousTime = std::chrono::steady_clock::now();

	const masterCycles_t cyclesPerFrame = std::chrono::duration_cast<masterCycles_t>( elapsed );
	const masterCycles_t nextCycle = sysCycles + cyclesPerFrame;

	Timer emuTime;
	emuTime.Start();
	bool isRunning = Run( nextCycle );
	emuTime.Stop();

	const double frameTimeUs = emuTime.GetElapsedUs();
	dbgInfo.frameTimeUs = static_cast<uint32_t>( frameTimeUs );

	if ( headless )	{
		return isRunning;
	}

	if ( !isRunning )
	{
		SaveSRam();
		return false;
	}

	// TEMP TEST CODE
	loadedState = false;
	savedState = false;
	if ( config.cpu.requestSaveState )
	{
		Serializer serializer( MB_1 );
		serializer.Clear();
		Serialize( serializer, serializeMode_t::STORE );
		savedState = true;

		std::ofstream saveFile;
		saveFile.open( baseFileName + L".st", ios::binary );
		saveFile.write( reinterpret_cast<char*>( serializer.GetPtr() ), serializer.CurrentSize() );
		saveFile.close();

		std::ofstream txt;
		txt.open( "saveState.txt", ios::trunc );
		txt.write( serializer.dbgText.str().c_str(), serializer.dbgText.str().size() );
		txt.close();
		
	}

	if ( config.cpu.requestLoadState )
	{
		std::ifstream loadFile;
		loadFile.open( baseFileName + L".st", ios::binary| ios::in );
		
		assert( loadFile.good() );

		loadFile.seekg( 0, std::ios::end );
		const uint32_t len = static_cast<uint32_t>( loadFile.tellg() );
			
		Serializer serializer( len );
		serializer.Clear();

		loadFile.seekg( 0, std::ios::beg );
		loadFile.read( reinterpret_cast<char*>( serializer.GetPtr() ), len );

		loadFile.close();				
		loadedState = true;

		Serialize( serializer, serializeMode_t::LOAD );
		
		std::ofstream txt;
		txt.open( "loadState.txt", ios::trunc );
		txt.write( serializer.dbgText.str().c_str(), serializer.dbgText.str().size() );
		txt.close();
		
	}
	// END

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