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

#include "wintendoApp.h"

#include <fstream>

extern wtAppInterface	app;

static void TestRomUnit( std::wstring& testFilePath )
{
	using namespace Tomtendo;
	using namespace std::chrono_literals;

	static wtFrameResult testFr;
	app.systemConfig = DefaultConfig();
	Boot( app.system, testFilePath.c_str(), 0xC000 );
	SetConfig( app.system, app.systemConfig );

	sysCmd_t traceCmd;
	traceCmd.type = sysCmdType_t::START_TRACE;
	traceCmd.parms[ 0 ].u = 1;
	SubmitCommand( app.system, traceCmd );

	std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>( 60s );

	RunEpoch( app.system, ns );
	GetFrameResult( app.system, testFr );
	std::string logText;
	logText.resize( 0 );
	logText.reserve( 400 * testFr.dbgLog->GetRecordCount() );
	testFr.dbgLog->ToString( logText, 0, testFr.dbgLog->GetRecordCount(), true );
	std::ofstream log( "testNes.log" );
	log << logText;
	log.close();
	Shutdown( app.system );
	app.TerminateEmulator();
}