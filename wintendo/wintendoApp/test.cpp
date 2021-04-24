#include "wintendoApp.h"
extern wtAppInterface	app;

static void TestRomUnit( std::wstring& testFilePath )
{
	static wtFrameResult testFr;
	wtSystem::InitConfig( app.systemConfig );
	app.system->Init( testFilePath, 0xC000 );
	app.system->SetConfig( app.systemConfig );

	sysCmd_t traceCmd;
	traceCmd.type = sysCmdType_t::START_TRACE;
	traceCmd.parms[ 0 ].u = 1;
	app.system->SubmitCommand( traceCmd );

	app.system->Run( MasterClockHz / 60 );
	app.system->GetFrameResult( testFr );
	std::string logText;
	logText.resize( 0 );
	logText.reserve( 400 * testFr.dbgLog->GetRecordCount() );
	testFr.dbgLog->ToString( logText, 0, testFr.dbgLog->GetRecordCount(), true );
	std::ofstream log( "testNes.log" );
	log << logText;
	log.close();
	app.TerminateEmulator();
}