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

	// TODO: redesign interface. GetElapsed() should return time since start even without stopping
	// Stop() does not reset timer currently to support this
	void Stop()
	{
		endTimeUs = std::chrono::system_clock::now().time_since_epoch();
	}

	double GetElapsedNs()
	{
		Stop();
		return static_cast<double>( ( endTimeUs - startTimeUs ).count() );
	}

	double GetElapsedUs()
	{
		Stop();
		return ( GetElapsedNs() / 1000.0f );
	}

	double GetElapsedMs()
	{
		Stop();
		return ( GetElapsedUs() / 1000.0f );
	}

private:
	std::chrono::nanoseconds startTimeUs;
	std::chrono::nanoseconds endTimeUs;
};