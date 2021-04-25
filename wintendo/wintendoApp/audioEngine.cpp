
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
		soundBufferState[ i ] = SOUND_STATE_EMPTY;
	}
}


void wtAudioEngine::Shutdown()
{
	if( pXAudio2.Get() ) {
		pXAudio2->StopEngine();
		pXAudio2->Release();
	}
}


void wtAudioEngine::EncodeSamples( wtSampleQueue& soundQueue )
{
	bool buffersFull = false;
	int32_t* destIx = &soundBufferBytesCnt[ currentSndBufferIx ];
	for( uint32_t sampleIx = 0; sampleIx < soundQueue.GetSampleCnt(); ++sampleIx )
	{
		assert( soundBufferState[ currentSndBufferIx ] != SOUND_STATE_SUBMITTED );

		float rawSample = soundQueue.Peek( sampleIx );
		int16_t encodedSample = static_cast<int16_t>( rawSample );
		soundDataBuffer[ currentSndBufferIx ][ ( *destIx )++ ] = encodedSample & 0xFF;
		soundDataBuffer[ currentSndBufferIx ][ ( *destIx )++ ] = ( encodedSample >> 8 ) & 0xFF;
		
		int32_t target = static_cast<int32_t>( BytesPerSubmit - 1 );
		if ( *destIx >= target )
		{
			++totalAudioBuffers;
			soundBufferState[ currentSndBufferIx ] = SOUND_STATE_READY;
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

	if ( soundBufferState[ consumeBufferIx ] == SOUND_STATE_EMPTY )
		return false;

	pSourceVoice->GetState( &audioState, 0 );

	static bool stopped = false;
	if ( audioState.BuffersQueued <= 2 ) {
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
		OutputDebugStringA( "Highpass 90hz Filter failure\n" );
	}

	XAUDIO2_FILTER_PARAMETERS hiPassFilterParameters1;
	hiPassFilterParameters1.Type = HighPassFilter;
	hiPassFilterParameters1.Frequency = XAudio2CutoffFrequencyToRadians( F2, FreqHz );
	hiPassFilterParameters1.OneOverQ = Q2;
	hr = pSourceVoice->SetFilterParameters( &hiPassFilterParameters1, XAUDIO2_COMMIT_ALL );

	if ( FAILED( hr ) )
	{
		//_com_error err( hr );
		//err.ErrorMessage();	
		OutputDebugStringA( "Highpass 440hz Filter failure\n" );
	}

	XAUDIO2_FILTER_PARAMETERS lowpassFilterParameters;
	lowpassFilterParameters.Type = LowPassFilter;
	lowpassFilterParameters.Frequency = XAudio2CutoffFrequencyToRadians( F3, FreqHz );
	lowpassFilterParameters.OneOverQ = Q3;
	//hr = pMasteringVoice->SetFilterParameters( &lowpassFilterParameters, XAUDIO2_COMMIT_ALL );

	if ( FAILED( hr ) )
	{
		OutputDebugStringA( "Lowpass Filter failure\n" );
	}
	

	pXAudio2->CommitChanges( XAUDIO2_COMMIT_ALL );
	return true;
}


void LogApu( wtFrameResult& frameResult )
{
	stringstream sndLogName;
	sndLogName << "apuLogs/" << "snd" << frameResult.currentFrame << ".csv";

	ofstream sndLog;
	sndLog.open( sndLogName.str(), ios::out | ios::binary );

	const wtSampleQueue& pulse1 = frameResult.soundOutput->dbgPulse1;
	const wtSampleQueue& pulse2 = frameResult.soundOutput->dbgPulse2;
	const wtSampleQueue& triangle = frameResult.soundOutput->dbgTri;
	const wtSampleQueue& noise = frameResult.soundOutput->dbgNoise;
	const wtSampleQueue& dmc = frameResult.soundOutput->dbgDmc;
	const wtSampleQueue& mixed = frameResult.soundOutput->dbgMixed;

	sndLog << "ix,pulse1,pulse2,triangle,noise,dmc,mixed\n";

	const uint32_t sampleCnt = mixed.GetSampleCnt();
	for ( uint32_t i = 0; i < sampleCnt; ++i )
	{
		sndLog << i << "," << pulse1.Peek( i ) << "," << pulse2.Peek( i ) << "," << triangle.Peek( i );
		sndLog << "," << noise.Peek( i ) << "," << dmc.Peek( i ) << "," << mixed.Peek( i ) << "\n";
	}

	sndLog.close();
}