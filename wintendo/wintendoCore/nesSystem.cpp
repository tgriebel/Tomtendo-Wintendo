#include "stdafx.h"
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
	if ( !cpu.IsTraceLogOpen() && cpu.logToFile )
	{
		string dbgString;
		std::ofstream logFile;
		logFile.open( fileName + L".log" );
		cpu.dbgLog.ToString( dbgString, 0, 0, true );
		logFile << dbgString << endl;
		logFile.close();
	}
#endif
}


int wtSystem::Init( const wstring& filePath, const uint32_t resetVectorManual )
{
	Reset();

	SaveSRam();

	LoadNesFile( filePath, cart );

	ppu.Reset();
	ppu.RegisterSystem( this );

	apu.Reset();
	apu.RegisterSystem( this );

	cpu.Reset();
	cpu.RegisterSystem( this );

	LoadProgram( resetVectorManual );
	fileName = filePath;

	const size_t offset = fileName.find( L".nes", 0 );
	baseFileName = fileName.substr( 0, offset );

	LoadSRam();

	return 0;
}


void wtSystem::Shutdown()
{
}


void wtSystem::LoadProgram( const uint32_t resetVectorManual )
{
	memset( memory, 0, PhysicalMemorySize );

	cart->mapper = AssignMapper( cart->GetMapperId() );
	cart->mapper->system = this;
	cart->mapper->OnLoadCpu();
	cart->mapper->OnLoadPpu();

	if ( resetVectorManual == 0x10000 ) {
		cpu.resetVector = Combine( ReadMemory( ResetVectorAddr ), ReadMemory( ResetVectorAddr + 1 ) );
	}
	else {
		cpu.resetVector = static_cast<uint16_t>( resetVectorManual & 0xFFFF );
	}

	cpu.nmiVector = Combine( ReadMemory( NmiVectorAddr ), ReadMemory( NmiVectorAddr + 1 ) );
	cpu.irqVector = Combine( ReadMemory( IrqVectorAddr ), ReadMemory( IrqVectorAddr + 1 ) );
	cpu.PC = cpu.resetVector;

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
	return memory[ StackBase + cpu.SP ];
}


uint8_t wtSystem::GetMapperId() const
{
	return ( cart->h.controlBits1.mappedNumberUpper << 4 ) | cart->h.controlBits0.mapperNumberLower;
}


uint8_t wtSystem::GetMirrorMode() const
{
	return mirrorMode;
}


void wtSystem::SetMirrorMode( uint8_t mode )
{
	mirrorMode = mode;
}


bool wtSystem::MouseInRegion( const wtRect& region ) // TODO: draft code, kill later
{
	return ( ( input.mousePoint.x >= region.x ) && ( input.mousePoint.x < region.width ) && ( input.mousePoint.y >= region.y ) && ( input.mousePoint.y < region.height ) );
}


uint8_t wtSystem::ReadMemory( const uint16_t address )
{
	const uint16_t mAddr = MirrorAddress( address );
	if ( IsCartMemory( mAddr ) )
	{
		return cart->mapper->ReadRom( mAddr );
	}
	else if ( IsPpuRegister( mAddr ) )
	{
		return ppu.ReadReg( mAddr );
	}
	else if ( mAddr == 0x4017 )
	{
		// FIXME: what to return?
		// https://wiki.nesdev.com/w/index.php/APU_DMC#cite_note-2
		// This note describes the register conflict	
		ReadInput( mAddr );
		return apu.ReadReg( mAddr );
	}
	else if ( IsApuRegister( mAddr ) )
	{
		return apu.ReadReg( mAddr );
	}
	else if ( IsInputRegister( mAddr ) )
	{
		return ReadInput( mAddr );
	}
	else
	{
		assert( mAddr < PhysicalMemorySize );
		return memory[ mAddr ];
	}
}


uint8_t wtSystem::ReadZeroPage( const uint16_t address )
{
	assert( address <= ZeroPageEnd );
	return memory[ address ];
}


void wtSystem::WriteMemory( const uint16_t address, const uint16_t offset, const uint8_t value )
{
	const uint32_t fullAddr = address + offset;
	const uint16_t mAddr = MirrorAddress( fullAddr );
	if ( IsPpuRegister( mAddr ) )
	{
		ppu.WriteReg( mAddr, value );
	}
	else if ( wtSystem::IsDMA( mAddr ) )
	{
		ppu.IssueDMA( value );
		apu.WriteReg( mAddr, value );
	}
	else if ( mAddr == 0x4017 )
	{
		WriteInput( mAddr, value );
		apu.WriteReg( mAddr, value );
	}
	else if ( IsInputRegister( mAddr ) )
	{
		WriteInput( mAddr, value );
	}
	else if ( wtSystem::IsApuRegister( mAddr ) )
	{
		apu.WriteReg( mAddr, value );
	}
	else if ( cart->mapper->InWriteWindow( mAddr, offset ) )
	{
		cart->mapper->Write( mAddr, value );
	}
	else
	{
		WritePhysicalMemory( mAddr, value );
	}
}


void wtSystem::WritePhysicalMemory( const uint16_t address, const uint8_t value )
{
	assert( address < PhysicalMemorySize );
	memory[ address ] = value;
}


uint8_t wtSystem::ReadInput( const uint16_t address )
{
	uint8_t keyBuffer = 0;

	assert( InputRegister0 <= address );
	const uint32_t controllerIndex = ( address - InputRegister0 );
	const ControllerId controllerId = static_cast<ControllerId>( controllerIndex );

	if ( strobeOn )
	{
		keyBuffer = static_cast<uint8_t>( input.GetKeyBuffer( controllerId ) & static_cast<ButtonFlags>( 0X80 ) );
		btnShift[ controllerIndex ] = 0;
	}
	else
	{
		keyBuffer = static_cast<uint8_t>( input.GetKeyBuffer( controllerId ) >> static_cast<ButtonFlags>( 7 - btnShift[ controllerIndex ] ) ) & 0x01;
		++btnShift[ controllerIndex ];
		btnShift[ controllerIndex ] %= 8;
	}
	return keyBuffer;
}


void wtSystem::WriteInput( const uint16_t address, const uint8_t value )
{
	const bool prevStrobeOn = strobeOn;
	strobeOn = ( value & 0x01 );
	if ( prevStrobeOn && !strobeOn )
	{
		btnShift[ 0 ] = 0;
		btnShift[ 1 ] = 0;
	}
}


void wtSystem::GetFrameResult( wtFrameResult& outFrameResult )
{
	outFrameResult.frameBuffer		= &frameBuffer[ finishedFrameIx ];
	outFrameResult.nameTableSheet	= &nameTableSheet;
	outFrameResult.paletteDebug		= &paletteDebug;
	outFrameResult.patternTable0	= &patternTable0;
	outFrameResult.patternTable1	= &patternTable1;
	outFrameResult.pickedObj8x16	= &pickedObj8x16;
	outFrameResult.ppuDebug			= ppu.dbgInfo;

	GetState( outFrameResult.cpuDebug );
#if DEBUG_ADDR
	outFrameResult.dbgLog = nullptr;
	if( cpu.dbgLog.IsFinished() ) {
		outFrameResult.dbgLog		= &cpu.dbgLog;
	}
#endif
	outFrameResult.dbgInfo			= dbgInfo;
	outFrameResult.romHeader		= cart->h;
	outFrameResult.mirrorMode		= static_cast<wtMirrorMode>( GetMirrorMode() );
	outFrameResult.mapperId			= GetMapperId();

	if ( apu.frameOutput != nullptr )
	{
		outFrameResult.soundOutput = apu.frameOutput;
		apu.GetDebugInfo( outFrameResult.apuDebug );
		apu.frameOutput = nullptr;
	}

	outFrameResult.frameState		= &frameState;
	outFrameResult.currentFrame		= frameNumber;
	outFrameResult.stateCount		= static_cast<uint64_t>( states.size() );
	outFrameResult.playbackState	= playbackState;
	outFrameResult.dbgFrameBufferIx	= finishedFrameIx;
	outFrameResult.frameToggleCount = frameTogglesPerRun;
}


void wtSystem::GetState( cpuDebug_t& state )
{
	state.A = cpu.A;
	state.X = cpu.X;
	state.Y = cpu.Y;
	state.P = cpu.P;
	state.PC = cpu.PC;
	state.SP = cpu.SP;
	state.resetVector = cpu.resetVector;
	state.nmiVector = cpu.nmiVector;
	state.irqVector = cpu.irqVector;
}


const PPU& wtSystem::GetPPU() const
{
	return ppu;
}


const APU& wtSystem::GetAPU() const
{
	return apu;
}


wtInput* wtSystem::GetInput()
{
	return &input;
}


const config_t* wtSystem::GetConfig()
{
	return config;
}


bool wtSystem::HasNewFrame() const
{
	return toggledFrame;
}


wtDisplayImage* wtSystem::GetBackbuffer()
{
	return &frameBuffer[ currentFrameIx ];
}


void wtSystem::InitConfig( config_t& config )
{
	// System
	config.sys.flags			= (emulationFlags_t)( (uint32_t)emulationFlags_t::CLAMP_FPS | (uint32_t)emulationFlags_t::LIMIT_STALL );

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


void wtSystem::SetConfig( config_t& systemConfig )
{
	config = &systemConfig;
}


void wtSystem::RequestNMI( const uint16_t vector ) const
{
	cpu.interruptRequestNMI = true;
	cpu.irqAddr = vector;
}


void wtSystem::RequestNMI() const
{
	cpu.interruptRequestNMI = true;
	cpu.irqAddr = cpu.nmiVector;
}


void wtSystem::RequestIRQ() const
{
	cpu.interruptRequest = true;
}


void wtSystem::RequestDMA() const
{
	cpu.oamInProcess = true;
}


void wtSystem::RequestDmcTransfer() const
{
	cpu.dmcTransfer = true;
}


void wtSystem::SaveSRam()
{
	if ( ( cart.get() != nullptr ) && cart->HasSave() )
	{
		uint8_t saveBuffer[ KB( 8 ) ];
		for ( int32_t i = 0; i < KB( 8 ); ++i ) {
			saveBuffer[ i ] = cart->mapper->ReadRom( 0x6000 + i );
		}

		std::ofstream saveFile;
		saveFile.open( baseFileName + L".sav", ios::binary );
		saveFile.write( reinterpret_cast<char*>( saveBuffer ), KB( 8 ) );
		saveFile.close();
	}
}


void wtSystem::LoadSRam()
{
	if ( ( cart.get() != nullptr ) && cart->HasSave() )
	{
		uint8_t saveBuffer[ KB( 2 ) ];

		std::ifstream saveFile;
		saveFile.open( baseFileName + L".sav", ios::binary );

		if ( !saveFile.good() ) {
			return;
		}

		saveFile.read( reinterpret_cast<char*>( saveBuffer ), KB( 2 ) );
		saveFile.close();

		for ( int32_t i = 0; i < KB( 2 ); ++i ) {
			cart->mapper->Write( 0x6000 + i, saveBuffer[ i ] );
		}
	}
}


void wtSystem::RecordSate( wtStateBlob& state )
{
	Serializer serializer( MB_1, serializeMode_t::STORE );
	Serialize( serializer );
	state.Set( serializer, sysCycles );
}


void wtSystem::RestoreState( const wtStateBlob& state )
{
	if ( !state.IsValid() ) {
		return;
	}

	Serializer serializer( state.GetBufferSize(), serializeMode_t::LOAD );
	state.WriteTo( serializer );
	Serialize( serializer );
}


void wtSystem::SaveSate()
{
	wtStateBlob state;
	RecordSate( state );

	std::ofstream saveFile;
	saveFile.open( baseFileName + L".st", ios::binary );
	saveFile.write( reinterpret_cast<char*>( state.GetPtr() ), state.GetBufferSize() );
	saveFile.close();
}


void wtSystem::LoadState()
{
	std::ifstream loadFile;
	loadFile.open( baseFileName + L".st", ios::binary | ios::in );

	if ( !loadFile.good() ) {
		loadFile.close();
		return;
	}

	loadFile.seekg( 0, std::ios::end );
	const uint32_t len = static_cast<uint32_t>( loadFile.tellg() );

	Serializer serializer( len, serializeMode_t::LOAD );

	loadFile.seekg( 0, std::ios::beg );
	loadFile.read( reinterpret_cast<char*>( serializer.GetPtr() ), len );
	loadFile.close();

	Serialize( serializer );
}


bool wtSystem::Run( const masterCycle_t& nextCycle )
{
	bool isRunning = true;

	static const masterCycle_t ticks( CpuClockDivide );

	apu.Begin();

	// TODO: CHECK WRAP AROUND LOGIC
	while ( ( sysCycles < nextCycle ) && isRunning )
	{
		sysCycles += ticks;

		isRunning = cpu.Step( MasterToCpuCycle( sysCycles ) );
		ppu.Step( MasterToPpuCycle( sysCycles ) );
		apu.Step( MasterToCpuCycle( sysCycles ) );
	}
	apu.End();

#if DEBUG_MODE == 1
	dbgInfo.masterCpu = chrono::duration_cast<masterCycle_t>( cpu.cycle );
	dbgInfo.masterPpu = chrono::duration_cast<masterCycle_t>( ppu.cycle );
	dbgInfo.masterApu = chrono::duration_cast<masterCycle_t>( apu.cpuCycle );
#endif // #if DEBUG_MODE == 1

#if DEBUG_ADDR == 1
	if ( cpu.IsTraceLogOpen() )
	{
		cpu.logFrameCount--;
		cpu.dbgLog.NewFrame();
	}
#endif // #if DEBUG_ADDR == 1

	return isRunning;
}


string wtSystem::GetPrgBankDissambly( const uint8_t bankNum )
{
	std::stringstream debugStream;
	uint8_t* bankMem = cart->GetPrgRomBank( bankNum );
	uint16_t curByte = 0;

	while ( curByte < KB( 16 ) )
	{
		stringstream hexString;
		const uint32_t instrAddr = curByte;
		const uint32_t opCode = bankMem[ curByte ];

		opInfo_t instrInfo = cpu.opLUT[ opCode ];
		const uint32_t operandCnt = instrInfo.operands;
		const char* mnemonic = instrInfo.mnemonic;

		if ( operandCnt == 1 )
		{
			const uint32_t op0 = bankMem[ curByte + 1 ];
			hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << opCode << " " << setw( 2 ) << op0;
		}
		else if ( operandCnt == 2 )
		{
			const uint32_t op0 = bankMem[ curByte + 1 ];
			const uint32_t op1 = bankMem[ curByte + 2 ];
			hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << opCode << " " << setw( 2 ) << op0 << " " << setw( 2 ) << op1;
		}
		else
		{
			hexString << uppercase << setfill( '0' ) << setw( 2 ) << hex << opCode;
		}

		debugStream << "0x" << right << uppercase << setfill( '0' ) << setw( 4 ) << hex << instrAddr << setfill( ' ' ) << "  " << setw( 10 ) << left << hexString.str() << mnemonic << std::endl;

		curByte += 1 + operandCnt;
		assert( curByte <= ( KB( 16 ) + operandCnt + 1 ) );
	}

	return debugStream.str();
}


void wtSystem::GenerateRomDissambly( string prgRomAsm[ 128 ] )
{
	assert( cart->h.prgRomBanks <= 128 );
	for ( uint32_t bankNum = 0; bankNum < cart->h.prgRomBanks; ++bankNum )
	{
		prgRomAsm[ bankNum ] = GetPrgBankDissambly( bankNum );
	}
}


void wtSystem::GetChrRomPalette( const uint8_t paletteId, RGBA palette[ 4 ] )
{
	const uint16_t baseAddr = ( paletteId > 3 ) ? PPU::SpritePaletteAddr : PPU::PaletteBaseAddr;
	const uint16_t baseOffset = 4 * ( paletteId % 4 );
	const uint8_t p0 = ppu.ReadVram( baseAddr + baseOffset + 0 );
	const uint8_t p1 = ppu.ReadVram( baseAddr + baseOffset + 1 );
	const uint8_t p2 = ppu.ReadVram( baseAddr + baseOffset + 2 );
	const uint8_t p3 = ppu.ReadVram( baseAddr + baseOffset + 3 );

	palette[ 0 ] = ppu.palette[ p0 ];
	palette[ 1 ] = ppu.palette[ p1 ];
	palette[ 2 ] = ppu.palette[ p2 ];
	palette[ 3 ] = ppu.palette[ p3 ];
}


void wtSystem::GetGrayscalePalette( RGBA palette[ 4 ] )
{
	palette[ 0 ] = DefaultPalette[ 0x30 ];
	palette[ 1 ] = DefaultPalette[ 0x10 ];
	palette[ 2 ] = DefaultPalette[ 0x00 ];
	palette[ 3 ] = DefaultPalette[ 0x0D ];
}


void wtSystem::GenerateChrRomTables( wtPatternTableImage chrRom[ 32 ] )
{
	assert( cart->GetChrBankCount() <= 32 );

	RGBA palette[ 4 ];
	if ( config->ppu.chrPalette == -1 ) {
		GetGrayscalePalette( palette );
	}
	else {
		GetChrRomPalette( config->ppu.chrPalette, palette );
	}

	assert( cart->h.chrRomBanks <= 32 );
	for ( uint32_t bankNum = 0; bankNum < cart->h.chrRomBanks; ++bankNum ) {
		ppu.DrawDebugPatternTables( chrRom[ bankNum ], palette, bankNum, true );
	}
}


void wtSystem::RunStateControl( const bool toggledFrame )
{
	const replayStateCode_t stateCode = playbackState.replayState;

	if ( toggledFrame )
	{
	//	RecordSate( frameState );
	//	dbgInfo.stateCycle = sysCycles;
	}

	if ( stateCode == replayStateCode_t::REPLAY )
	{
		const bool replayFinished = ( playbackState.currentFrame >= playbackState.finalFrame );
		if ( replayFinished )
		{
			playbackState.replayState = replayStateCode_t::FINISHED;
		}
		else if( toggledFrame )
		{
			const wtStateBlob& state = states[ playbackState.currentFrame ];
			if( state.IsValid() )
			{
				GetBackbuffer()->Clear();
				RestoreState( states[ playbackState.currentFrame ] );
				playbackState.currentFrame += playbackState.pause ? 0 : 1;
			}
			else
			{
				playbackState.replayState = replayStateCode_t::FINISHED;
			}
		}
	}
	else if ( stateCode == replayStateCode_t::RECORD )
	{
		const bool hasNewState = ( toggledFrame && frameState.IsValid() );
		const bool canRecord = ( playbackState.currentFrame < playbackState.finalFrame );

		if( hasNewState && canRecord )
		{
			if( states.size() >= MaxStates ) {
				states.pop_front();
			}

			states.push_back( frameState );
			++playbackState.currentFrame;
		}
	}
	else if ( stateCode == replayStateCode_t::FINISHED )
	{
		states.clear();
		frameState.Reset();
		playbackState.replayState = replayStateCode_t::LIVE;
		playbackState.startFrame = -1;
		playbackState.currentFrame = -1;
		playbackState.finalFrame = -1;
	}
}


void wtSystem::UpdateDebugImages()
{
	RGBA palette[ 4 ];
	for ( uint32_t i = 0; i < 4; ++i )
	{
		palette[ i ] = ppu.palette[ ppu.ReadVram( PPU::PaletteBaseAddr + i ) ];
	}

	RGBA pickedPalette[ 4 ];
	GetChrRomPalette( ( ppu.dbgInfo.spritePicked.palette >> 2 ) + 4, pickedPalette );
	ppu.DrawDebugObject( &pickedObj8x16, pickedPalette, ppu.dbgInfo.spritePicked );

	if ( debugNTEnable ) {
		ppu.DrawDebugNametable( nameTableSheet );
	}
	ppu.DrawDebugPalette( paletteDebug );
	ppu.DrawDebugPatternTables( patternTable0, palette, 0, false );
	ppu.DrawDebugPatternTables( patternTable1, palette, 1, false );

	DebugPrintFlushLog();
}


void wtSystem::SaveFrameState()
{
	RecordSate( frameState );
	dbgInfo.stateCycle = sysCycles;
}


void wtSystem::ToggleFrame()
{
	finishedFrameIx = currentFrameIx;
	currentFrameIx = ( currentFrameIx + 1 ) % 3;
#if 0
	// Debug code. Should never see red flashes in final display
	frameBuffer[ currentFrameIx ].Clear( 0xFF0000FF );
#endif
	frameNumber++;
	toggledFrame = true;
	frameTogglesPerRun++;
}


int wtSystem::RunEpoch( const std::chrono::nanoseconds& runEpoch )
{
	ProcessCommands();

	const nano_t e = nano_t( runEpoch.count() );

	masterCycle_t cyclesPerFrame = masterCycle_t( overflowCycles );
	overflowCycles = 0;

	const bool clampFps = ( config->sys.flags & emulationFlags_t::CLAMP_FPS );
	const bool stallLimit = ( config->sys.flags & emulationFlags_t::LIMIT_STALL );

	if ( ( runEpoch > MaxFrameLatencyNs ) && stallLimit ) // Clamp simulation catch-up for hitches/debugging
	{
		cyclesPerFrame = NanoToCycle( MaxFrameLatencyNs.count() );
	}
	else if ( ( runEpoch > FrameLatencyNs ) && clampFps ) // Don't skip frames, instead even out bubbles over a few frames
	{
		cyclesPerFrame = NanoToCycle( FrameLatencyNs.count() );
		overflowCycles = ( runEpoch - FrameLatencyNs ).count();
	}
	else
	{
		cyclesPerFrame = masterCycle_t( NanoToCycle( e ) );
	}
	
	RunStateControl( toggledFrame );

	const masterCycle_t startCycle = sysCycles;
	const masterCycle_t nextCycle = sysCycles + cyclesPerFrame;
	
	toggledFrame = false;
	frameTogglesPerRun = 0;
	previousFrameNumber = frameNumber;

	dbgInfo.cycleBegin = sysCycles;

	Timer emuTime;
	emuTime.Start();
	bool isRunning = Run( nextCycle );
	emuTime.Stop();

	const masterCycle_t endCycle = sysCycles;
	overflowCycles += ( endCycle - nextCycle ).count();

	dbgInfo.cycleEnd = endCycle;

	const double frameTimeUs = emuTime.GetElapsedUs();
	dbgInfo.frameTimeUs = static_cast<uint32_t>( frameTimeUs );
	dbgInfo.totalTimeUs += dbgInfo.frameTimeUs;
	dbgInfo.simulationTimeUs = static_cast<uint32_t>( CycleToNano( endCycle - startCycle ).count() / 1000.0f );
	dbgInfo.realTimeUs = static_cast<uint32_t>( std::chrono::duration_cast<std::chrono::microseconds>( runEpoch ).count() );
	dbgInfo.frameNumber = frameNumber;
	dbgInfo.framePerRun += frameNumber - previousFrameNumber;
	dbgInfo.runInvocations++;

	DebugPrintFlushLog();

	if ( ( config->sys.flags & emulationFlags_t::HEADLESS ) != 0 ) {
		return isRunning;
	}

	if ( !isRunning )
	{
		SaveSRam();
		return false;
	}

	return isRunning;
}