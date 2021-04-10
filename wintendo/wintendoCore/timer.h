#pragma once
#include <chrono>

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