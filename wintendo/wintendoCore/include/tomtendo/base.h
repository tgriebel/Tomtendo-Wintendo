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

#pragma once

#include <cstdint>

#ifdef STATIC_BUILD
#undef TOMTENDO_LIBRARY_EXPORTS
#endif

#ifdef TOMTENDO_LIBRARY_EXPORTS
#define EXPORT_DLL extern "C" __declspec(dllexport)
#define EXPORT_CLASS_DLL __declspec(dllexport)
#elif !defined(STATIC_BUILD)
#define EXPORT_DLL __declspec(dllimport)
#define EXPORT_CLASS_DLL __declspec(dllimport)
#else
#define EXPORT_DLL
#define EXPORT_CLASS_DLL
#endif

#ifdef IMPORT_WIN
#include <windows.h> 
#include <stdio.h>

#define RuntimeImportDllFunction( library, runtimeInterface, name ) runtimeInterface->##name = (runtimeDllInterface_t::PFN_##name)GetProcAddress( library, #name );
#endif