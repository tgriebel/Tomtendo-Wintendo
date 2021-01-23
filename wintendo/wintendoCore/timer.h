#pragma once

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
		startTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() );
		endTimeMs = startTimeMs;
	}

	void Stop()
	{
		endTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() );
	}

	double GetElapsed()
	{
		return ( endTimeMs - startTimeMs ).count();
	}

private:
	std::chrono::milliseconds startTimeMs;
	std::chrono::milliseconds endTimeMs;
};