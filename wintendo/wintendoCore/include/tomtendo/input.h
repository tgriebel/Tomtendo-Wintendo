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

	using keyBinding_t = std::pair<ControllerId, ButtonFlags>;

	class Input
	{
		private:
			ButtonFlags							keyBuffer[ 2 ];
			mouse_t								mousePoint;
			std::map<uint32_t, keyBinding_t>	keyMap;

		public:
			ButtonFlags			GetKeyBuffer( const ControllerId controllerId ) const;
			mouse_t				GetMouse() const;
			void				BindKey( const char key, const ControllerId controllerId, const ButtonFlags button );		
			void				StoreKey( const uint32_t key );
			void				ReleaseKey( const uint32_t key );
			void				StoreMouseClick( const int32_t x, const int32_t y );
			void				ClearMouseClick();
	};
};