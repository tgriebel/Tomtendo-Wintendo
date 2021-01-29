#pragma once

#include <memory>
#include "common.h"
#include "mappers/NROM.h"
#include "mappers/MMC1.h"
#include "mappers/MMC3.h"
#include "mappers/UNROM.h"

unique_ptr<wtMapper> wtSystem::AssignMapper( const uint32_t mapperId )
{
	switch ( mapperId )
	{
		default:
		case 0: return make_unique<NROM>( mapperId );	break;
		case 1:	return make_unique<MMC1>( mapperId );	break;
		case 2:	return make_unique<UNROM>( mapperId );	break;
		case 4:	return make_unique<MMC3>( mapperId );	break;
	}
}