#pragma once

#include "stdafx.h"
#include <wchar.h>
#include <Windows.h>
#include "common.h"
#include "NesSystem.h"

int InitSystem( const wstring& filePath );
int RunFrame();
void CopyImageBuffer( wtRawImage& destImageBuffer, wtImageTag tag );