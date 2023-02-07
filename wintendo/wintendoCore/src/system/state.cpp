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

#include "../../include/tomtendo/interface.h"

namespace Tomtendo
{
	bool wtStateBlob::IsValid() const
	{
		return ( byteCount > 0 );
	}

	uint32_t  wtStateBlob::GetBufferSize() const
	{
		return byteCount;
	}

	uint8_t* wtStateBlob::GetPtr()
	{
		return bytes;
	}

	void  wtStateBlob::Set( Serializer& s, const masterCycle_t sysCycle )
	{
		Reset();
		byteCount = s.CurrentSize();
		bytes = new uint8_t[ byteCount ];
		memcpy( bytes, s.GetPtr(), byteCount );

		serializerHeader_t::section_t* memSection;
		s.FindLabel( STATE_MEMORY_LABEL, &memSection );
		header.memory = bytes + memSection->offset;
		header.memorySize = memSection->size;

		serializerHeader_t::section_t* vramSection;
		s.FindLabel( STATE_VRAM_LABEL, &vramSection );
		header.vram = bytes + vramSection->offset;
		header.vramSize = vramSection->size;

		cycle = sysCycle;
	}

	void  wtStateBlob::WriteTo( Serializer& s ) const
	{
		assert( s.BufferSize() >= byteCount );
		if ( s.BufferSize() < byteCount ) {
			return;
		}
		const serializeMode_t mode = s.GetMode();
		s.SetMode( serializeMode_t::STORE );
		s.NextArray( bytes, byteCount );
		s.SetPosition( 0 );
		s.SetMode( mode );
	}

	void  wtStateBlob::Reset()
	{
		if ( bytes == nullptr )
		{
			delete[] bytes;
			byteCount = 0;
		}
		cycle = masterCycle_t( 0 );
	}
};