#pragma once

#define XAUDIO2_HELPER_FUNCTIONS

#include <windows.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <xaudio2.h>
#include <queue>
#include "wintendoApp.h"

struct wtAudioEngine
{
	static const uint32_t				SndBufferCnt		= 5;
	static const uint32_t				SamplesPerSubmit	= 100000;
	static const uint32_t				SampleSize			= sizeof( int16_t );
	static const uint32_t				BytesPerSubmit		= SampleSize * SamplesPerSubmit;
	static const uint32_t				BufferSize			= 1000 + BytesPerSubmit;
	static const uint32_t				FreqHz				= 48000;
	static const uint32_t				SourceFreqHz		= ApuSamplesPerSec;

	Microsoft::WRL::ComPtr<IXAudio2>	pXAudio2;
	IXAudio2MasteringVoice*				pMasteringVoice;
	IXAudio2SourceVoice*				pSourceVoice;
	XAUDIO2_VOICE_STATE					audioState;

	int32_t								soundBufferBytesCnt[ SndBufferCnt ];
	int32_t								soundBufferState[ SndBufferCnt ];
	byte								soundDataBuffer[ SndBufferCnt ][ BufferSize ];

	wtSoundBuffer						dbgSoundFrameData;

	volatile uint64_t					soundBufferCpyLock	= 0;
	UINT32								OperationSetCounter	= 0;

	int32_t								currentSndBufferIx	= 0;
	int32_t								consumeBufferIx		= 0;
	int32_t								totalAudioSubmits	= 0;
	int32_t								totalAudioBuffers	= 0;

	float								audioDuration		= 0.0f;
	bool								enableSound			= true;
	bool								audioStopped		= false;
	float								Q1					= 1.2f;		// [0, 1.5]
	float								Q2					= 1.2f;		// [0, 1.5]
	float								Q3					= 1.2f;		// [0, 1.5]
	float								F1					= 90.0f;	//
	float								F2					= 440.0f;	//
	float								F3					= 14000.0f;	//
	bool								logSnd				= false;
	uint32								dbgLastSoundSampleLength = 0;

	void								Init();
	void								EncodeSamples( wtSampleQueue& soundQueue );
	bool								AudioSubmit();
};


class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	Timer timer;
	HANDLE hBufferEndEvent;
	volatile LONG totalQueues;
	VoiceCallback() : hBufferEndEvent( CreateEvent( NULL, FALSE, FALSE, NULL ) ), totalQueues( 0 ) {}
	~VoiceCallback() { CloseHandle( hBufferEndEvent ); }

	void OnStreamEnd() { }

	void OnVoiceProcessingPassStart( UINT32 SamplesRequired ) {}
	void OnVoiceProcessingPassEnd() {}

	void OnBufferEnd( void* pBufferContext )
	{
		timer.Stop();
		//audio.audioDuration = timer.GetElapsed();

		if ( pBufferContext != nullptr )
		{
			*( (int32_t*)pBufferContext ) = -2;
		}

		SetEvent( hBufferEndEvent );
		InterlockedAdd( &totalQueues, -1 );
	}
	void OnBufferStart( void* pBufferContext )
	{
		timer.Start();
		InterlockedAdd( &totalQueues, 1 );
	}

	void OnLoopEnd( void* pBufferContext ) {}
	void OnVoiceError( void* pBufferContext, HRESULT Error ) {}
};

extern VoiceCallback voiceCallback;