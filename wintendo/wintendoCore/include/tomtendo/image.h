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

#include <cstdint>

namespace Tomtendo
{
	struct RGBA
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t alpha;
	};

	union Pixel
	{
		uint8_t vec[ 4 ];
		uint32_t rawABGR; // ABGR format
		RGBA rgba;

		inline uint32_t AsHexColor()
		{
			Pixel abgr;
			abgr.rgba = { rgba.alpha, rgba.red, rgba.green, rgba.blue };
			return abgr.rawABGR;
		}
	};

	struct wtPoint
	{
		int32_t x;
		int32_t y;
	};

	struct wtRect
	{
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
	};

	class wtRawImageInterface
	{
	public:
		virtual void					SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel ) = 0;
		virtual void					Set( const uint32_t index, const Pixel value ) = 0;
		virtual const Pixel&			Get( const uint32_t index ) = 0;
		virtual void					Clear( const uint32_t r8g8b8a8 = 0 ) = 0;

		virtual const uint32_t* const	GetRawBuffer() const = 0;
		virtual uint32_t				GetWidth() const = 0;
		virtual uint32_t				GetHeight() const = 0;
		virtual uint32_t				GetBufferLength() const = 0;
		virtual const char*				GetDebugName() const = 0;
		virtual void					SetDebugName( const char* debugName ) = 0;
	};

	template< uint32_t N, uint32_t M >
	class wtRawImage : public wtRawImageInterface
	{
	public:

		wtRawImage()
		{
			Clear();
			name = "";
		}

		wtRawImage( const char* name_ )
		{
			Clear();
			name = name_;
		}

		wtRawImage& operator=( const wtRawImage& _image )
		{
			if( this != &_image )
			{
				for ( uint32_t i = 0; i < _image.length; ++i ) {
					buffer[ i ].rawABGR = _image.buffer[ i ].rawABGR;
				}
				name = _image.name;
			}
			return *this;
		}

		void SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel )
		{
			const uint32_t index = ( x + y * width );
			assert( index < length );

			if ( index < length )
			{
				buffer[ index ] = pixel;
			}
		}

		inline void Set( const uint32_t index, const Pixel value )
		{
			assert( index < length );

			//if ( index < length )
			{
				buffer[ index ] = value;
			}
		}

		const Pixel& Get( const uint32_t index )
		{
			assert( index < length );
			if ( index < length )
			{
				return buffer[ index ];
			}
			return buffer[ 0 ];
		}

		inline void SetDebugName( const char* debugName )
		{
			name = debugName;
		}

		void Clear( const uint32_t r8g8b8a8 = 0 )
		{
			for ( uint32_t i = 0; i < length; ++i )
			{
				buffer[ i ].rawABGR = r8g8b8a8;
			}
		}

		inline const uint32_t* const GetRawBuffer() const
		{
			return &buffer[ 0 ].rawABGR;
		}

		inline uint32_t GetWidth() const
		{
			return width;
		}

		inline uint32_t GetHeight() const
		{
			return height;
		}

		inline uint32_t GetBufferLength() const
		{
			return length;
		}

		inline const char* GetDebugName() const
		{
			return name;
		}

		inline wtRect GetRect()
		{
			return { 0, 0, width, height };
		}

	private:
		static const uint32_t width = N;
		static const uint32_t height = M;
		static const uint32_t length = N * M;
		Pixel buffer[ length ];
		const char* name;
	};

	using wtDisplayImage		= wtRawImage<256, 240>;
	using wtNameTableImage		= wtRawImage<2 * 256, 2 * 240>;
	using wtPaletteImage		= wtRawImage<16, 2>;
	using wtPatternTableImage	= wtRawImage<128, 128>;
	using wt16x8ChrImage		= wtRawImage<8, 16>;
	using wt8x8ChrImage			= wtRawImage<8, 8>;
};