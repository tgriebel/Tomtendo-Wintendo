#pragma once
#include "base.h"

#include "../../stdafx.h"
#include <stdint.h>
#include <map>

namespace Tomtendo
{
	enum class ControllerId : uint8_t
	{
		CONTROLLER_0 = 0X00,
		CONTROLLER_1 = 0X01,
		CONTROLLER_2 = 0X02,
		CONTROLLER_3 = 0X03,
		CONTROLLER_COUNT,
		CONTROLLER_INVALID,
	};

	enum class ButtonFlags : uint8_t
	{
		BUTTON_NONE = 0X00,

		BUTTON_RIGHT = 0X01,
		BUTTON_LEFT = 0X02,
		BUTTON_DOWN = 0X04,
		BUTTON_UP = 0X08,

		BUTTON_START = 0X10,
		BUTTON_SELECT = 0X20,
		BUTTON_B = 0X40,
		BUTTON_A = 0X80,
	};

	struct mouse_t
	{
		int32_t x;
		int32_t y;
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

	struct EXPORT_CLASS_DLL keyBinding_t
	{
		ControllerId	controllerId	= ControllerId::CONTROLLER_INVALID;
		ButtonFlags		buttonFlags		= ButtonFlags::BUTTON_NONE;
	};

	struct EXPORT_CLASS_DLL Input
	{
		ButtonFlags		keyBuffer[ 2 ];
		mouse_t			mousePoint;
		keyBinding_t	keyMap[ 256 ];
	};

	EXPORT_DLL ButtonFlags	GetKeyBuffer( const Input* input, const ControllerId controllerId );
	EXPORT_DLL mouse_t		GetMouse( const Input* input );
	EXPORT_DLL void			BindKey( Input* input, const char key, const ControllerId controllerId, const ButtonFlags button );
	EXPORT_DLL void			StoreKey( Input* input, const uint32_t key );
	EXPORT_DLL void			ReleaseKey( Input* input, const uint32_t key );
	EXPORT_DLL void			StoreMouseClick( Input* input, const int32_t x, const int32_t y );
	EXPORT_DLL void			ClearMouseClick( Input* input );
};