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
#include <chrono>

namespace Tomtendo
{
	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Reset()
		{
			startTimeNs = std::chrono::nanoseconds( 0 );
			endTimeNs = std::chrono::nanoseconds( 0 );
			totalTimeNs = std::chrono::nanoseconds( 0 );
		}

		void Start()
		{
			startTimeNs = std::chrono::system_clock::now().time_since_epoch();
			endTimeNs = startTimeNs;
		}

		void Stop()
		{
			endTimeNs = std::chrono::system_clock::now().time_since_epoch();
			totalTimeNs += ( endTimeNs - startTimeNs );
			startTimeNs = endTimeNs;
		}

		double GetElapsedNs()
		{
			endTimeNs = std::chrono::system_clock::now().time_since_epoch();
			return static_cast<double>( totalTimeNs.count() + ( endTimeNs - startTimeNs ).count() );
		}

		double GetElapsedUs()
		{
			return ( GetElapsedNs() / 1000.0f );
		}

		double GetElapsedMs()
		{
			return ( GetElapsedUs() / 1000.0f );
		}

	private:
		std::chrono::nanoseconds startTimeNs;
		std::chrono::nanoseconds endTimeNs;
		std::chrono::nanoseconds totalTimeNs;
	};
};