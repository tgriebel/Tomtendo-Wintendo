#pragma once

#include "stdafx.h"
#include <stdint.h>

enum class ButtonFlags : uint8_t
{
	BUTTON_NONE		= 0X00,

	BUTTON_RIGHT	= 0X01,
	BUTTON_LEFT		= 0X02,
	BUTTON_DOWN		= 0X04,
	BUTTON_UP		= 0X08,
	
	BUTTON_START	= 0X10,
	BUTTON_SELECT	= 0X20,
	BUTTON_B		= 0X40,
	BUTTON_A		= 0X80,
};


inline ButtonFlags operator |( const ButtonFlags lhs, const ButtonFlags rhs )
{
	return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) | static_cast<uint8_t>( rhs ) );
}

inline ButtonFlags operator &( const ButtonFlags lhs, const ButtonFlags rhs )
{
	return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) & static_cast<uint8_t>( rhs ) );
}

inline ButtonFlags operator >>( const ButtonFlags lhs, const ButtonFlags rhs )
{
	return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) >> static_cast<uint8_t>( rhs ) );
}

inline ButtonFlags operator <<( const ButtonFlags lhs, const ButtonFlags rhs )
{
	return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) << static_cast<uint8_t>( rhs ) );
}

enum class ControllerId : uint8_t
{
	CONTROLLER_0 = 0X00,
	CONTROLLER_1 = 0X01,
	CONTROLLER_2 = 0X02,
	CONTROLLER_3 = 0X03,
	CONTROLLER_COUNT,
};


extern ButtonFlags keyBuffer[2];


inline ButtonFlags GetKeyBuffer( const ControllerId controllerId )
{
	const uint32_t mapKey = static_cast<uint32_t>( controllerId );
	return keyBuffer[mapKey];
}


// TODO: make thread safe -- look at CaptureKey function I started
// keyBuffer is only written by store key so it's guaranteed read only elsewhere
inline void StoreKey( const ControllerId controllerId, const ButtonFlags key )
{
	const uint32_t mapKey = static_cast<uint32_t>( controllerId );
	keyBuffer[mapKey] = keyBuffer[mapKey] | static_cast<ButtonFlags>( key );
}


inline void ReleaseKey( const ControllerId controllerId, const ButtonFlags key )
{
	const uint32_t mapKey = static_cast<uint32_t>( controllerId );
	keyBuffer[mapKey] = keyBuffer[mapKey] & static_cast<ButtonFlags>( ~static_cast<uint8_t>( key ) );
}