#pragma once

#include "stdafx.h"
#include <stdint.h>
#include <map>

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

typedef std::pair<ControllerId, ButtonFlags> wtKeyBinding_t;

class wtInput
{
private:
	std::map<uint32_t, wtKeyBinding_t>	keyMap;

public:
	ButtonFlags							keyBuffer[ 2 ];
	wtPoint								mousePoint;

	inline ButtonFlags GetKeyBuffer( const ControllerId controllerId )
	{
		const uint32_t mapKey = static_cast<uint32_t>( controllerId );
		return keyBuffer[mapKey];
	}


	inline void BindKey( const char key, const ControllerId controllerId, const ButtonFlags button )
	{
		keyMap[key] = wtKeyBinding_t( controllerId, button );
	}


	inline wtKeyBinding_t& GetBinding( const uint32_t key )
	{
		return keyMap[key];
	}


	inline void StoreKey( const uint32_t key )
	{
		wtKeyBinding_t& keyBinding = GetBinding( key );
		const uint32_t mapKey = static_cast<uint32_t>( keyBinding.first );
		keyBuffer[mapKey] = keyBuffer[mapKey] | static_cast<ButtonFlags>( keyBinding.second );
	}


	inline void ReleaseKey( const uint32_t key )
	{
		wtKeyBinding_t& keyBinding = GetBinding( key );
		const uint32_t mapKey = static_cast<uint32_t>( keyBinding.first );
		keyBuffer[mapKey] = keyBuffer[mapKey] & static_cast<ButtonFlags>( ~static_cast<uint8_t>( keyBinding.second ) );
	}


	inline void StoreMouseClick( const wtPoint& point )
	{
		mousePoint = point;
	}

	inline void ClearMouseClick()
	{
		mousePoint = wtPoint( { -1, -1 } );
	}
};