#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Windows.h>
#include <string>
#include <wchar.h>
#include <sstream>

#include "bitmap.h"
#include "NesSystem.h"

wtSystem nesSystem;

int main()
{
	nesSystem.Init( L"Games/Contra.nes" );

	config_t cfg;
	wtSystem::InitConfig( cfg );
	cfg.sys.flags = emulationFlags_t::HEADLESS;
	nesSystem.SetConfig( cfg );

	nesSystem.RunEpoch( FrameLatencyNs );

	nesSystem.Shutdown();
}