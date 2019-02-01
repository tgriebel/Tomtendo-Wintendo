#pragma once

#include "stdafx.h"
#include <Windows.h>
#include "common.h"
#include "NesSystem.h"

int InitSystem();
int RunFrame();
void CopyFrameBuffer( uint32_t frameBuffer[], const size_t destSize );
void CopyNametable( uint32_t destBuffer[], const size_t destSize );
void StoreKey( uint8_t keys );
