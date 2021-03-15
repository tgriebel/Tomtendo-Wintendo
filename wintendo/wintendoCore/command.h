#pragma once
#include "common.h"

enum class sysCmdType_t
{
	LOAD_STATE,
	SAVE_STATE,
	RECORD,
	REPLAY,
	START_TRACE,
	STOP_TRACE,
};

struct sysCmd_t
{
	static const uint32_t MaxParms = 10;

	union parm_t
	{
		int64_t		i;
		double		f;
		uint64_t	u;
	};

	sysCmdType_t	type;
	parm_t			parms[ MaxParms ];
};