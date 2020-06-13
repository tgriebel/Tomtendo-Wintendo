#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Windows.h>
#include <string>
#include <wchar.h>
#include <sstream>
#include <assert.h>

#include "bitmap.h"
#include "NesSystem.h"

wtSystem nesSystem;

int main()
{
	nesSystem.InitSystem( L"Games/Contra.nes" );
	nesSystem.headless = true;

	nesSystem.RunFrame();

	nesSystem.ShutdownSystem();
}