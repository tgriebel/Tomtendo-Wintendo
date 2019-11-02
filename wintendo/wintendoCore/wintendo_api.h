#pragma once

#include "stdafx.h"
#include <wchar.h>
#include <Windows.h>
#include "common.h"
#include "NesSystem.h"

int InitSystem( const char* filePath );
int RunFrame();
void CopyFrameBuffer( uint32_t frameBuffer[], const size_t destSize );
void CopyNametable( uint32_t destBuffer[], const size_t destSize );
void CopyPalette( uint32_t destBuffer[], const size_t destSize );
void CopyPatternTable0( uint32_t destBuffer[], const size_t destSize );
void CopyPatternTable1( uint32_t destBuffer[], const size_t destSize );
void SetGameName( const char* name );