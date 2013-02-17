//=============================================================================================================
#include "audiostreamer.h"
#include <iostream>

Sound::Sound()
{
	voice = NULL;
	isstream = false;
	canplay = false;
	playrequest = false;
	playing = false;

	totalsize = 0;
	readsize = 0;
	currentbuffer = 0;
	freebuffers = MAX_BUFFER_COUNT;

	memset(data, 0, sizeof(data));
	memset(&format, 0, sizeof(WAVEFORMATEX));
}
//=============================================================================================================
Sound::~Sound()
{
	playguard.Lock();
	canplay = false;

	if( voice )
	{
		voice->DestroyVoice();
		voice = NULL;
	}

	for( int i = 0; i < MAX_BUFFER_COUNT; ++i )
		SAFE_DELETE_ARRAY(data[i]);

	if( isstream )
		ov_clear(&oggfile);

	playguard.Unlock();
}
//=============================================================================================================
void Sound::PullBuffer(int id)
{
	XAUDIO2_BUFFER audbuff;
	memset(&audbuff, 0, sizeof(XAUDIO2_BUFFER));

	ids[id] = id;
	audbuff.pAudioData = (const BYTE*)data[id];

	if( readsize >= totalsize )
		audbuff.Flags = XAUDIO2_END_OF_STREAM;

	audbuff.AudioBytes = STREAMING_BUFFER_SIZE;
	audbuff.pContext = &ids[id];

	if( FAILED(voice->SubmitSourceBuffer(&audbuff)) )
		SND_ERROR("AudioStreamer::Update(): Could not submit buffer");
}
//=============================================================================================================
void Sound::Play()
{
	playguard.Lock();
	{
		playrequest = true;

		if( voice && canplay && !playing )
		{
			playing = true;
			voice->Start(0);
		}
	}
	playguard.Unlock();
}
//=============================================================================================================
COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE Sound::OnBufferEnd(void* context)
{
	// TODO: ide is kell lock? (freebuffers-t irja)

	if( context )
	{
		int id = *reinterpret_cast<int*>(context);

		SND_DEBUG("buffer " << id << " finished processing");
		
		// pull the last buffer if it's waiting
		if( freebuffers == 0 )
		{
			int buffind = (MAX_BUFFER_COUNT + (currentbuffer - 1)) % MAX_BUFFER_COUNT;

			PullBuffer(buffind);
			SND_DEBUG("buffer " << buffind << " pulled (pull)");
		}

		++freebuffers;
	}
	else
		SND_ERROR("Sound::OnBufferEnd(): Invalid context");
}
//=============================================================================================================
