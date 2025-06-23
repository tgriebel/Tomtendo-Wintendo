/*
* MIT License
*
* Copyright( c ) 2023 Thomas Griebel
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this softwareand associated documentation files( the "Software" ), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright noticeand this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "../include/tomtendo/interface.h"
#include "system/NesSystem.h"

namespace Tomtendo
{
	config_t DefaultConfig()
	{
		config_t config;

		// System
		config.sys.flags = (emulationFlags_t)( (uint32_t)emulationFlags_t::CLAMP_FPS | (uint32_t)emulationFlags_t::LIMIT_STALL );

		// PPU
		config.ppu.chrPalette = 0;
		config.ppu.showBG = true;
		config.ppu.showSprite = true;
		config.ppu.spriteLimit = PPU::SecondarySprites;

		// APU
		config.apu.frequencyScale = 1.0f;
		config.apu.volume = 1.0f;
		config.apu.waveShift = 0;
		config.apu.disableSweep = false;
		config.apu.disableEnvelope = false;
		config.apu.mutePulse1 = false;
		config.apu.mutePulse2 = false;
		config.apu.muteTri = false;
		config.apu.muteNoise = false;
		config.apu.muteDMC = false;
		config.apu.dbgChannelBits = 0;

		return config;
	}

	void CreateEmulatorInstance( Emulator** emulatorInstance )
	{
		if ( emulatorInstance != nullptr )
		{
			if( *emulatorInstance != nullptr ) {
				delete *emulatorInstance;
			}
			*emulatorInstance = new Emulator();
		}
	}

	void DestroyEmulatorInstance( Emulator** emulatorInstance )
	{
		if ( ( emulatorInstance != nullptr ) && ( *emulatorInstance != nullptr ) )
		{
			Shutdown( *emulatorInstance );
			delete *emulatorInstance;
			*emulatorInstance = nullptr;
		}
	}

	uint32_t ScreenWidth()
	{
		return PPU::ScreenWidth;
	}

	uint32_t ScreenHeight()
	{
		return PPU::ScreenHeight;
	}

	uint32_t SpriteLimit()
	{
		return PPU::TotalSprites;
	}

	void Shutdown( Emulator* emu )
	{
		if( emu == nullptr ) {
			return;
		}
		if ( system != nullptr ) {
			delete emu->system;
		}
	}

	bool Boot( Emulator* emu, const wchar_t* filePath, const uint32_t resetVectorManual )
	{
		if ( emu == nullptr ) {
			return false;
		}
		if( emu->system != nullptr ) {
			delete emu->system;
		}
		emu->system = new wtSystem();
		const int ret = emu->system->Init( filePath );
		if( ret == 0 )
		{
			emu->system->AttachInputHandler( &emu->input );
			return true;
		}
		return false;
	}

	int RunEpoch( Emulator* emu, const std::chrono::nanoseconds& runCycles )
	{
		if ( emu == nullptr ) {
			return 0;
		}
		return emu->system->RunEpoch( runCycles );
	}

	void GetFrameResult( Emulator* emu, wtFrameResult& outFrameResult )
	{
		if ( emu == nullptr ) {
			return;
		}
		emu->system->GetFrameResult( outFrameResult );
	}

	void SetConfig( Emulator* emu, config_t& cfg )
	{
		if ( emu == nullptr ) {
			return;
		}
		emu->system->SetConfig( cfg );
	}

	void SubmitCommand( Emulator* emu, const sysCmd_t& cmd )
	{
		if ( emu == nullptr ) {
			return;
		}
		emu->system->SubmitCommand( cmd );
	}

	void UpdateDebugImages( Emulator* emu )
	{
		if ( emu == nullptr ) {
			return;
		}
		emu->system->UpdateDebugImages();
	}

	void GenerateRomDissambly( Emulator* emu, std::string prgRomAsm[ 128 ] )
	{
		if ( emu == nullptr ) {
			return;
		}

		assert( emu->system->cart->h.prgRomBanks <= 128 );
		for ( uint32_t bankNum = 0; bankNum < emu->system->cart->h.prgRomBanks; ++bankNum )
		{
			prgRomAsm[ bankNum ] = emu->system->GetPrgBankDissambly( bankNum );
		}
	}

	void GenerateChrRomTables( Emulator* emu, wtPatternTableImage chrRom[ 32 ] )
	{
		if ( emu == nullptr ) {
			return;
		}

		assert( emu->system->cart->GetChrBankCount() <= 32 );

		RGBA palette[ 4 ];
		if ( emu->system->GetConfig()->ppu.chrPalette == -1 ) {
			emu->system->GetGrayscalePalette( palette );
		}
		else {
			emu->system->GetChrRomPalette( emu->system->GetConfig()->ppu.chrPalette, palette );
		}

		assert( emu->system->cart->h.chrRomBanks <= 32 );
		for ( uint32_t bankNum = 0; bankNum < emu->system->cart->h.chrRomBanks; ++bankNum ) {
			emu->system->GetPPU().DrawDebugPatternTables( chrRom[ bankNum ], palette, bankNum, true );
		}
	}
		
	ButtonFlags GetKeyBuffer( const Input* input, const ControllerId controllerId )
	{
		const uint32_t mapKey = static_cast<uint32_t>( controllerId );
		return input->keyBuffer[ mapKey ];
	}

	mouse_t GetMouse( const Input* input )
	{
		return input->mousePoint;
	}

	void BindKey( Input* input, const char key, const ControllerId controllerId, const ButtonFlags button )
	{
		input->keyMap[ key ].controllerId = controllerId;
		input->keyMap[ key ].buttonFlags = button;
	}

	void StoreKey( Input* input, const uint32_t key )
	{
		keyBinding_t keyBinding = input->keyMap[ key ];
		const uint32_t mapKey = static_cast<uint32_t>( keyBinding.controllerId );
		input->keyBuffer[ mapKey ] = input->keyBuffer[ mapKey ] | static_cast<ButtonFlags>( keyBinding.buttonFlags );
	}

	void ReleaseKey( Input* input, const uint32_t key )
	{
		keyBinding_t keyBinding = input->keyMap[ key ];
		const uint32_t mapKey = static_cast<uint32_t>( keyBinding.controllerId );
		input->keyBuffer[ mapKey ] = input->keyBuffer[ mapKey ] & static_cast<ButtonFlags>( ~static_cast<uint8_t>( keyBinding.buttonFlags ) );
	}

	void StoreMouseClick( Input* input, const int32_t x, const int32_t y )
	{
		input->mousePoint = mouse_t( { x, y } );
	}

	void ClearMouseClick( Input* input )
	{
		input->mousePoint = mouse_t( { -1, -1 } );
	}
};