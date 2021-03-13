#pragma once
#include <stdint.h>

enum class replayStateCode_t : uint8_t
{
	LIVE,
	RECORD,
	REPLAY,
	FINISHED,
};

struct playbackState_t
{
	replayStateCode_t	replayState;
	int64_t				startFrame;
	int64_t				currentFrame;
	int64_t				finalFrame;
	bool				pause;
};