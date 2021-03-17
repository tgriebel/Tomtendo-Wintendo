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
	nesSystem.Init( L"Games/Contra.nes", wtSystemFlags::HEADLESS );

	nesSystem.RunFrame();

	nesSystem.Shutdown();
}