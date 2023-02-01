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

#define XAUDIO2_HELPER_FUNCTIONS

#include <windows.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <xaudio2.h>
#include <queue>
#include <comdef.h>
#include "wintendoApp.h"

enum SoundState
{
	SOUND_STATE_EMPTY = -2,
	SOUND_STATE_SUBMITTED = -1,
	SOUND_STATE_READY = 0,
};

struct wtAudioEngine
{
	static const uint32_t				SndBufferCnt		= 10;
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

	UINT32								OperationSetCounter	= 0;

	int32_t								currentSndBufferIx	= 0;
	int32_t								consumeBufferIx		= 0;
	int32_t								totalAudioSubmits	= 0;
	int32_t								totalAudioBuffers	= 0;
	uint32_t							emulatorFrame		= 0;
	uint32_t							frameResultIx		= 0;

#if defined(_DEBUG)
	bool								enableSound			= false;
#else
	bool								enableSound			= true;
#endif
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
	void								Shutdown();
	void								EncodeSamples( wtSampleQueue& soundQueue );
	bool								AudioSubmit();
};


class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	Timer timer;
	float totalDuration;
	HANDLE hBufferEndEvent;
	volatile LONG totalQueues;
	volatile LONG processedQueues;
	VoiceCallback() : hBufferEndEvent( CreateEvent( NULL, FALSE, FALSE, NULL ) ), totalQueues( 0 ), processedQueues( 0 ), totalDuration( 0.0f ) {}
	~VoiceCallback() { CloseHandle( hBufferEndEvent ); }

	void OnStreamEnd() { }

	void OnVoiceProcessingPassStart( UINT32 SamplesRequired ) {}
	void OnVoiceProcessingPassEnd() {}

	void OnBufferEnd( void* pBufferContext )
	{
		totalDuration += static_cast<float>( timer.GetElapsedMs() );

		if ( pBufferContext != nullptr )
		{
			*( (int32_t*)pBufferContext ) = SOUND_STATE_EMPTY;
		}

		SetEvent( hBufferEndEvent );
		InterlockedAdd( &totalQueues, -1 );
		InterlockedAdd( &processedQueues, 1 );
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

void LogApu( wtFrameResult& frameResult );