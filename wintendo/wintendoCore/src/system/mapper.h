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

#include <memory>
#include "../common.h"
#include "../mappers/NROM.h"
#include "../mappers/MMC1.h"
#include "../mappers/MMC3.h"
#include "../mappers/UNROM.h"

std::unique_ptr<wtMapper> wtSystem::AssignMapper( const uint32_t mapperId )
{
	switch ( mapperId )
	{
		default:
		case 0: return std::make_unique<NROM>( mapperId );	break;
		case 1:	return std::make_unique<MMC1>( mapperId );	break;
		case 2:	return std::make_unique<UNROM>( mapperId );	break;
		case 4:	return std::make_unique<MMC3>( mapperId );	break;
	}
}