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
	static wtSystem system;

	void InitConfig( config_t& config )
	{
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

	int Emulator::Boot( const std::wstring& filePath, const uint32_t resetVectorManual )
	{
		const int ret = system.Init( filePath );
		if( ret == 0 )
		{
			system.AttachInputHandler( &input );
			return true;
		}
		return false;
	}

	int Emulator::RunEpoch( const std::chrono::nanoseconds& runCycles )
	{
		return system.RunEpoch( runCycles );
	}

	void Emulator::GetFrameResult( wtFrameResult& outFrameResult )
	{
		system.GetFrameResult( outFrameResult );
	}

	void Emulator::SetConfig( config_t& cfg )
	{
		system.SetConfig( cfg );
	}

	void Emulator::SubmitCommand( const sysCmd_t& cmd )
	{
		system.SubmitCommand( cmd );
	}

	void Emulator::UpdateDebugImages()
	{
		system.UpdateDebugImages();
	}

	void Emulator::GenerateRomDissambly( std::string prgRomAsm[ 128 ] )
	{
		assert( system.cart->h.prgRomBanks <= 128 );
		for ( uint32_t bankNum = 0; bankNum < system.cart->h.prgRomBanks; ++bankNum )
		{
			prgRomAsm[ bankNum ] = system.GetPrgBankDissambly( bankNum );
		}
	}

	void Emulator::GenerateChrRomTables( wtPatternTableImage chrRom[ 32 ] )
	{
		assert( system.cart->GetChrBankCount() <= 32 );

		RGBA palette[ 4 ];
		if ( system.GetConfig()->ppu.chrPalette == -1 ) {
			system.GetGrayscalePalette( palette );
		}
		else {
			system.GetChrRomPalette( system.GetConfig()->ppu.chrPalette, palette );
		}

		assert( system.cart->h.chrRomBanks <= 32 );
		for ( uint32_t bankNum = 0; bankNum < system.cart->h.chrRomBanks; ++bankNum ) {
			system.GetPPU().DrawDebugPatternTables( chrRom[ bankNum ], palette, bankNum, true );
		}
	}
		
	ButtonFlags Input::GetKeyBuffer( const ControllerId controllerId ) const
	{
		const uint32_t mapKey = static_cast<uint32_t>( controllerId );
		return keyBuffer[ mapKey ];
	}

	mouse_t Input::GetMouse() const
	{
		return mousePoint;
	}

	void Input::BindKey( const char key, const ControllerId controllerId, const ButtonFlags button )
	{
		keyMap[ key ] = keyBinding_t( controllerId, button );
	}

	void Input::StoreKey( const uint32_t key )
	{
		keyBinding_t keyBinding = keyMap[ key ];
		const uint32_t mapKey = static_cast<uint32_t>( keyBinding.first );
		keyBuffer[ mapKey ] = keyBuffer[ mapKey ] | static_cast<ButtonFlags>( keyBinding.second );
	}

	void Input::ReleaseKey( const uint32_t key )
	{
		keyBinding_t keyBinding = keyMap[ key ];
		const uint32_t mapKey = static_cast<uint32_t>( keyBinding.first );
		keyBuffer[ mapKey ] = keyBuffer[ mapKey ] & static_cast<ButtonFlags>( ~static_cast<uint8_t>( keyBinding.second ) );
	}

	void Input::StoreMouseClick( const int32_t x, const int32_t y )
	{
		mousePoint = mouse_t( { x, y } );
	}

	void Input::ClearMouseClick()
	{
		mousePoint = mouse_t( { -1, -1 } );
	}
};