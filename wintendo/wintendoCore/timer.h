#pragma once
#include <chrono>

class Timer
{
public:
	Timer()
	{
		Start();
		Stop();
	}

	void Start()
	{
		startTimeUs = std::chrono::system_clock::now().time_since_epoch();
		endTimeUs = startTimeUs;
	}

	void Stop()
	{
		endTimeUs = std::chrono::system_clock::now().time_since_epoch();
	}

	double GetElapsedNs()
	{
		return static_cast<double>( ( endTimeUs - startTimeUs ).count() );
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
	std::chrono::nanoseconds startTimeUs;
	std::chrono::nanoseconds endTimeUs;
};