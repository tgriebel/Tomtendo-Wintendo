/*
* MIT License
*
* Copyright( c ) 2017-2021 Thomas Griebel
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

#include <string>
#include <map>
#include <queue>
#include <thread>
#include <chrono>
#include "assert.h"
#include "../include/tomtendo/interface.h"

using namespace Tomtendo;

#define NES_MODE			(1)
#define DEBUG_MODE			(0)
#define DEBUG_ADDR			(1)
#define MIRROR_OPTIMIZATION	(1)

const uint32_t KB_1		= 1024;
const uint32_t MB_1		= 1024 * KB_1;

#define KB(n) ( n * KB_1 )
#define BIT_MASK(n)	( 1 << n )
#define SELECT_BIT( word, bit ) ( (word) & (BIT_MASK_##bit) ) >> (BIT_##bit)

#ifdef _MSC_BUILD
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif

#define DEFINE_ENUM_OPERATORS( enumType, intType )														\
inline enumType operator|=( enumType lhs, enumType rhs )												\
{																										\
	return static_cast< enumType >( static_cast< intType >( lhs ) | static_cast< intType >( rhs ) );	\
}																										\
																										\
inline bool operator&( enumType lhs, enumType rhs )														\
{																										\
	return ( ( static_cast< intType >( lhs ) & static_cast< intType >( rhs ) ) != 0 );					\
}																										\
																										\
inline enumType operator>>( const enumType lhs, const enumType rhs )									\
{																										\
	return static_cast<enumType>( static_cast<intType>( lhs ) >> static_cast<intType>( rhs ) );			\
}																										\
																										\
inline enumType operator<<( const enumType lhs, const enumType rhs )									\
{																										\
	return static_cast<enumType>( static_cast<intType>( lhs ) << static_cast<intType>( rhs ) );			\
}


enum class wtImageTag
{
	FRAME_BUFFER,
	NAMETABLE,
	PALETTE,
	PATTERN_TABLE_0,
	PATTERN_TABLE_1
};

FORCE_INLINE uint16_t Combine( const uint8_t lsb, const uint8_t msb )
{
	return ( ( ( msb << 8 ) | lsb ) & 0xFFFF );
}

FORCE_INLINE bool InRange( const uint16_t addr, const uint16_t loAddr, const uint16_t hiAddr )
{
	return ( addr >= loAddr ) && ( addr <= hiAddr );
}