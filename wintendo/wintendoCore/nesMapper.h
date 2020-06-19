#pragma once

#include <memory>
#include "common.h"
#include "mappers/NROM.h"
#include "mappers/MMC1.h"
#include "mappers/UNROM.h"

unique_ptr<wtMapper> wtSystem::AssignMapper( const uint32_t mapperId )
{
	switch ( mapperId )
	{
	case 1:
		return make_unique<MMC1>();
		break;

	case 2:
		return make_unique<UNROM>();
		break;

	case 0:
	default:
		return make_unique<NROM>();
		break;
	}
}