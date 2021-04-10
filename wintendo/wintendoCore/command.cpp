#include "command.h"
#include "NesSystem.h"

void wtSystem::ProcessCommands()
{
	const size_t cmdCount = commands.size();
	for ( size_t i = 0; i < cmdCount; ++i )
	{
		sysCmd_t& cmd = commands[ i ];
		switch( cmd.type )
		{
			case sysCmdType_t::LOAD_STATE:
			{
				LoadState();
			}
			break;

			case sysCmdType_t::SAVE_STATE:
			{
				SaveSate();
			}
			break;

			case sysCmdType_t::RECORD:
			{
				if( playbackState.replayState == replayStateCode_t::LIVE )
				{
					const int64_t frameCount = cmd.parms[ 0 ].i;
					playbackState.replayState = replayStateCode_t::RECORD;
					playbackState.startFrame = 0;
					playbackState.currentFrame = 0;
					if ( frameCount < 0 ) {
						playbackState.finalFrame = INT64_MAX;
					} else {
						playbackState.finalFrame = frameCount;
					}
				}
			}
			break;

			case sysCmdType_t::REPLAY:
			{
				const int64_t frameCount = cmd.parms[ 0 ].i;
				const bool pause = ( cmd.parms[ 1 ].u > 0 );
				playbackState.replayState = replayStateCode_t::REPLAY;
				playbackState.startFrame = frameCount;
				playbackState.currentFrame = frameCount;
				playbackState.finalFrame = static_cast<int64_t>( states.size() ) - 1;
				playbackState.pause = pause;
			}
			break;

			case sysCmdType_t::START_TRACE:
			{
				const uint32_t frameCount = static_cast<uint32_t>( cmd.parms[0].u );
				if ( ( frameCount > 0 ) && !cpu.IsTraceLogOpen() ) {
					cpu.StartTraceLog( frameCount );
				}
			}
			break;

			case sysCmdType_t::STOP_TRACE:
			{
				if ( cpu.IsTraceLogOpen() ) {
					cpu.StopTraceLog();
				}
			}
			break;

			default: break;
		}
		commands.pop_front();
	}
}


void wtSystem::SubmitCommand( const sysCmd_t& cmd )
{
	// Should be ok to insert async since it just appends to the end
	commands.push_back( cmd );
}