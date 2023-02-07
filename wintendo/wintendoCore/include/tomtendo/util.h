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
#include <stdint.h>
#include "assert.h"

namespace Tomtendo
{
	template < uint16_t B >
	class BitCounter
	{
	public:
		BitCounter() {
			Reload();
		}

		void Inc() {
			bits++;
		}

		void Dec() {
			bits--;
		}

		void Reload( uint16_t value = 0 )
		{
			bits = value;
			unused = 0;
		}

		uint16_t Value() {
			return bits;
		}

		bool IsZero() {
			return ( Value() == 0 );
		}
	private:
		uint16_t bits : B;
		uint16_t unused : ( 16 - B );
	};


	template< uint16_t B >
	class wtShiftReg
	{
	public:

		wtShiftReg()
		{
			Clear();
		}

		void Shift( const bool bitValue )
		{
			reg >>= 1;
			reg &= LMask;
			reg |= ( static_cast<uint32_t>( bitValue ) << ( Bits - 1 ) ) & ~LMask;

			++shifts;
		}

		uint32_t GetShiftCnt() const
		{
			return shifts;
		}

		bool IsFull() const
		{
			return ( shifts >= Bits );
		}

		bool GetBitValue( const uint16_t bit ) const
		{
			return ( ( reg >> bit ) & 0x01 );
		}

		uint32_t GetValue() const
		{
			return reg & Mask;
		}

		void Set( const uint32_t value )
		{
			reg = value;
			shifts = 0;
		}

		void Clear()
		{
			reg = 0;
			shifts = 0;
		}

	private:
		uint32_t reg;
		uint32_t shifts;
		static const uint16_t Bits = B;

		static constexpr uint32_t CalcMask()
		{
			uint32_t mask = 0x02;
			for ( uint32_t i = 1; i < B; ++i )
			{
				mask <<= 1;
			}
			return ( mask - 1 );
		}

		static const uint32_t Mask = CalcMask();
		static const uint32_t LMask = ( Mask >> 1 );
		static const uint32_t RMask = ~0x01ul;
	};


	template< typename T, uint32_t SIZE >
	class wtBuffer
	{
	public:
		void Clear()
		{
			Reset();
			for ( uint32_t i = 0; i < SIZE; ++i )
			{
				samples[ i ] = 0;
			}
		}

		void Reset()
		{
			currentIndex = 0;
			memset( samples, 0xFF, sizeof( samples[ 0 ] ) * SIZE );
		}

		void Write( const T& sample )
		{
			samples[ currentIndex ] = sample;
			currentIndex++;
			assert( currentIndex <= SIZE );
		}

		float Read( const uint32_t index ) const
		{
			assert( index <= SIZE );
			return samples[ index ];
		}

		const T* GetRawBuffer()
		{
			return &samples[ 0 ];
		}

		uint32_t GetSampleCnt() const
		{
			return currentIndex;
		}

		bool IsFull() const
		{
			return ( currentIndex == SIZE );
		}

	private:
		T			samples[ SIZE ];
		uint32_t	currentIndex;
	};


	template< typename T, uint32_t SIZE >
	class wtQueue
	{
	public:
		wtQueue()
		{
			Reset();
		}

		float Peek( const uint32_t sampleIx ) const
		{
			if( sampleIx >= GetSampleCnt() ) {
				return 0.0f;
			}

			const int32_t index = ( begin + sampleIx ) % SIZE;
			return samples[ index ];
		}

		void EnqueFIFO( const T& sample )
		{
			if ( IsFull() ) {
				Deque();
			}
			Enque( sample );
		}

		void Enque( const T& sample )
		{
			if ( IsFull() )
				return;

			if ( begin == -1 )
			{
				begin = 0;
			}

			end = ( end + 1 ) % SIZE;
			samples[ end ] = sample;
		}

		float Deque()
		{
			if ( IsEmpty() )
				return 0.0f;

			const float retValue = samples[ begin ];
			samples[ begin ] = 0;
			if ( begin == end )
			{
				begin = -1;
				end = -1;
			}
			else
			{
				begin = ( begin + 1 ) % SIZE;
			}

			return retValue;
		}

		bool IsEmpty() const
		{
			return ( GetSampleCnt() == 0 );
		}

		bool IsFull()
		{
			return ( GetSampleCnt() == ( SIZE - 1 ) );
		}

		uint32_t GetSampleCnt() const
		{
			if ( ( begin == -1 ) || ( end == -1 ) )
			{
				return 0;
			}
			else if ( end >= begin )
			{
				return ( ( end - begin ) + 1 );
			}
			else
			{
				return ( SIZE - ( begin - end ) + 1 );
			}
		}

		void Reset()
		{
			begin = -1;
			end = -1;
		}

	private:
		T		samples[ SIZE ];
		int32_t	begin;
		int32_t	end;
	};
};