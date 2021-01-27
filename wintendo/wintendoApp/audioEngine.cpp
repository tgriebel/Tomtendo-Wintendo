
#include "audioEngine.h"

VoiceCallback voiceCallback;

void wtAudioEngine::Init()
{
	HRESULT hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
	if ( FAILED( hr ) )
	{
		wprintf( L"Failed to init COM: %#X\n", hr );
		return;
	}

	hr = XAudio2Create( &pXAudio2 );
	if ( FAILED( hr ) )
	{
		std::cout << "XAudio2Create failure: ";
		return;
	}

	hr = pXAudio2->CreateMasteringVoice( &pMasteringVoice );
	if ( FAILED( hr ) )
	{
		//_com_error err( hr );
		//std::cout << "CreateMasteringVoice failure: " << err.ErrorMessage();
		return;
	}

	// Create a source voice
	WAVEFORMATEX waveformat;
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.nChannels = 1;
	waveformat.nSamplesPerSec = wtAudioEngine::FreqHz;
	waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * wtAudioEngine::SampleSize;
	waveformat.nBlockAlign = wtAudioEngine::SampleSize;
	waveformat.wBitsPerSample = 8 * wtAudioEngine::SampleSize;
	waveformat.cbSize = 0;
	hr = pXAudio2->CreateSourceVoice( &pSourceVoice, &waveformat, XAUDIO2_VOICE_USEFILTER, 200.0f, &voiceCallback, NULL, NULL );
	if ( FAILED( hr ) )
	{
		std::cout << "CreateSourceVoice failure: ";
		return;
	}

	hr = pSourceVoice->Start( 0, XAUDIO2_COMMIT_ALL );

	if ( FAILED( hr ) )
	{
		std::cout << "Start failure: ";
		return;
	}

	for ( int32_t i = 0; i < wtAudioEngine::SndBufferCnt; ++i )
	{
		soundBufferBytesCnt[ i ] = 0;
		soundBufferState[ i ] = -2;
	}
}


void wtAudioEngine::EncodeSamples( wtSampleQueue& soundQueue )
{
	bool buffersFull = false;
	int32_t* destIx = &soundBufferBytesCnt[ currentSndBufferIx ];
	while ( !soundQueue.IsEmpty() )
	{
		assert( soundBufferState[ currentSndBufferIx ] != -1 );
		if ( soundBufferState[ currentSndBufferIx ] == -2 )
		{
			// Debug code
		//	memset( soundDataBuffer[currentSndBufferIx], 0xFF, BufferSize );
		}

		float rawSample = soundQueue.Deque();
		int16_t encodedSample = static_cast<int16_t>( rawSample );
		soundDataBuffer[ currentSndBufferIx ][ ( *destIx )++ ] = encodedSample & 0xFF;
		soundDataBuffer[ currentSndBufferIx ][ ( *destIx )++ ] = ( encodedSample >> 8 ) & 0xFF;
		dbgSoundFrameData.Write( rawSample );
		if ( dbgSoundFrameData.IsFull() )
		{
			dbgSoundFrameData.Reset();
		}

		int32_t target = static_cast<int32_t>( BytesPerSubmit - 1 );
		if ( *destIx >= target )
		{
			++totalAudioBuffers;
			soundBufferState[ currentSndBufferIx ] = 0;
			currentSndBufferIx = ( currentSndBufferIx + 1 ) % wtAudioEngine::SndBufferCnt;

			destIx = &soundBufferBytesCnt[ currentSndBufferIx ];
			( *destIx ) = 0;
		}
	}
}


bool wtAudioEngine::AudioSubmit()
{
	if ( pSourceVoice == nullptr )
		return false;

	if ( consumeBufferIx == -1 )
		return false;

	if ( soundBufferState[ consumeBufferIx ] == -2 )
		return false;

	pSourceVoice->GetState( &audioState, 0 );

	static bool stopped = false;
	if ( audioState.BuffersQueued <= 1 ) {
		pSourceVoice->Stop();
		stopped = true;
	}
	else if ( stopped ) {
		pSourceVoice->Start();
		stopped = false;
	}
	else if ( audioState.BuffersQueued >= ( wtAudioEngine::SndBufferCnt - 1 ) ) {
		pSourceVoice->FlushSourceBuffers();
	}

	const float freqScale = 1.00f;
	const float resampleRate = freqScale * ( wtAudioEngine::SourceFreqHz / (float)wtAudioEngine::FreqHz );

	static bool hold = false;

	int soundDataSizeInBytes = soundBufferBytesCnt[ consumeBufferIx ];

	dbgLastSoundSampleLength = static_cast<uint32_t>( 0.5f * soundDataSizeInBytes );

	XAUDIO2_BUFFER audioBuffer;
	memset( &audioBuffer, 0, sizeof( XAUDIO2_BUFFER ) );
	audioBuffer.AudioBytes = soundDataSizeInBytes;
	audioBuffer.pAudioData = (BYTE* const)&soundDataBuffer[ consumeBufferIx ];
	audioBuffer.pContext = &soundBufferState[ consumeBufferIx ];

	consumeBufferIx = ( consumeBufferIx + 1 ) % wtAudioEngine::SndBufferCnt;

	float originalFreq = 1.0f;
	XAUDIO2_FILTER_PARAMETERS parms;
	pSourceVoice->GetFrequencyRatio( &originalFreq );
	pSourceVoice->GetFilterParameters( &parms );

	XAUDIO2_VOICE_DETAILS voiceDetails;
	pSourceVoice->GetVoiceDetails( &voiceDetails );

	HRESULT hr = pSourceVoice->SubmitSourceBuffer( &audioBuffer );
	if ( FAILED( hr ) )
	{
		std::cout << "Source buffer creation failure";
		return false;
	}

	++totalAudioSubmits;

	UINT32 OperationID = UINT32( InterlockedIncrement( LPLONG( &OperationSetCounter ) ) );

	pSourceVoice->SetFrequencyRatio( resampleRate, XAUDIO2_COMMIT_ALL );
	pSourceVoice->SetVolume( 1.0f, XAUDIO2_COMMIT_ALL );
	
	XAUDIO2_FILTER_PARAMETERS hiPassFilterParameters0;
	hiPassFilterParameters0.Type = HighPassFilter;
	hiPassFilterParameters0.Frequency = XAudio2CutoffFrequencyToRadians( F1, FreqHz );
	hiPassFilterParameters0.OneOverQ = Q1;
	hr = pSourceVoice->SetFilterParameters( &hiPassFilterParameters0, XAUDIO2_COMMIT_ALL );

	if ( FAILED( hr ) )
	{
		//_com_error err( hr );
		//std::cout << "Highpass 90hz Filter failure: " << err.ErrorMessage();
	}

	XAUDIO2_FILTER_PARAMETERS hiPassFilterParameters1;
	hiPassFilterParameters1.Type = HighPassFilter;
	hiPassFilterParameters1.Frequency = XAudio2CutoffFrequencyToRadians( F2, FreqHz );
	hiPassFilterParameters1.OneOverQ = Q2;
	hr = pSourceVoice->SetFilterParameters( &hiPassFilterParameters1, XAUDIO2_COMMIT_ALL );

	if ( FAILED( hr ) )
	{
		//_com_error err( hr );
		//std::cout << "Highpass 440hz Filter failure: " << err.ErrorMessage();
	}

	XAUDIO2_FILTER_PARAMETERS lowpassFilterParameters;
	lowpassFilterParameters.Type = LowPassFilter;
	lowpassFilterParameters.Frequency = XAudio2CutoffFrequencyToRadians( F3, FreqHz );
	lowpassFilterParameters.OneOverQ = Q3;
	hr = pMasteringVoice->SetFilterParameters( &lowpassFilterParameters, XAUDIO2_COMMIT_ALL );

	if ( FAILED( hr ) )
	{
		//_com_error err( hr );
		//std::cout << "Lowpass Filter failure: " << err.ErrorMessage();
	}
	

	pXAudio2->CommitChanges( XAUDIO2_COMMIT_ALL );
	return true;
}