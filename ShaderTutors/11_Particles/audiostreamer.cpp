//=============================================================================================================
#include "audiostreamer.h"
#include <iostream>

AudioStreamer::AudioStreamer()
{
	running = true;
}
//=============================================================================================================
AudioStreamer::~AudioStreamer()
{
	Destroy();
}
//=============================================================================================================
void AudioStreamer::Destroy()
{
	soundguard.Lock();
	
	for( soundlist::iterator it = sound.begin(); it != sound.end(); ++it )
	{
		if( *it )
			delete (*it);
	}

	sound.clear();
	soundguard.Unlock();

	running = false;
}
//=============================================================================================================
Sound* AudioStreamer::LoadSound(IXAudio2* xaudio2, const std::string& file)
{
	if( !xaudio2 )
	{
		SND_ERROR("AudioStreamer::LoadSound(): NULL == xaudio2");
		return false;
	}

	FILE* infile = NULL;
	fopen_s(&infile, file.c_str(), "rb");

	if( !infile )
	{
		SND_ERROR("AudioStreamer::LoadSound(): Could not open file");
		return NULL;
	}

	OggVorbis_File oggfile;
	int stream;
	long readbytes = 0;
	long totalread = 0;

	ov_open(infile, &oggfile, NULL, 0);
	vorbis_info* info = ov_info(&oggfile, -1);

	Sound* s = new Sound();

	s->format.nChannels = info->channels;
	s->format.cbSize = sizeof(WAVEFORMATEX);
	s->format.wBitsPerSample = 16;
	s->format.nSamplesPerSec = info->rate;
	s->format.nAvgBytesPerSec = info->rate * 2 * info->channels;
	s->format.nBlockAlign = 2 * info->channels;
	s->format.wFormatTag = 1;
	
	s->totalsize = (unsigned int)ov_pcm_total(&oggfile, -1) * 2 * info->channels;
	s->data[0] = new char[s->totalsize];

	do
	{
		readbytes = ov_read(&oggfile, s->data[0] + totalread, s->totalsize - totalread, 0, 2, 1, &stream);
		totalread += readbytes;
	}
	while( readbytes > 0 && (unsigned int)totalread < s->totalsize );

	ov_clear(&oggfile);
	
	// create xaudio voice object
	if( FAILED(xaudio2->CreateSourceVoice(&s->voice, &s->format, 0, 2.0f, s)) )
	{
		SND_ERROR("AudioStreamer::LoadSound(): Could not create source voice");

		delete s;
		return NULL;
	}

	XAUDIO2_BUFFER audbuff;
	memset(&audbuff, 0, sizeof(XAUDIO2_BUFFER));

	audbuff.pAudioData = (const BYTE*)s->data[0];
	audbuff.Flags = XAUDIO2_END_OF_STREAM;
	audbuff.AudioBytes = s->totalsize;
	audbuff.LoopCount = XAUDIO2_LOOP_INFINITE;

	if( FAILED(s->voice->SubmitSourceBuffer(&audbuff)) )
	{
		SND_ERROR("AudioStreamer::LoadSound(): Could not submit buffer");

		delete s;
		return NULL;
	}

	s->canplay = true;

	soundguard.Lock();
	sound.push_back(s);
	soundguard.Unlock();

	return s;
}
//=============================================================================================================
Sound* AudioStreamer::LoadSoundStream(IXAudio2* xaudio2, const std::string& file)
{
	FILE* infile = NULL;
	fopen_s(&infile, file.c_str(), "rb");

	if( !infile )
		return NULL;

	Sound* s = new Sound();

	ov_open(infile, &s->oggfile, NULL, 0);
	vorbis_info* info = ov_info(&s->oggfile, -1);

	s->format.nChannels = info->channels;
	s->format.cbSize = sizeof(WAVEFORMATEX);
	s->format.wBitsPerSample = 16;
	s->format.nSamplesPerSec = info->rate;
	s->format.nAvgBytesPerSec = info->rate * 2 * info->channels;
	s->format.nBlockAlign = 2 * info->channels;
	s->format.wFormatTag = 1;

	s->isstream = true;
	s->totalsize = (unsigned int)ov_pcm_total(&s->oggfile, -1) * 2 * info->channels;
	s->canplay = false;

	size_t size = min(s->totalsize, STREAMING_BUFFER_SIZE);

	s->data[0] = new char[size];
	s->data[1] = new char[size];
	s->data[2] = new char[size];

	if( FAILED(xaudio2->CreateSourceVoice(&s->voice, &s->format, 0, 2.0f, s)) )
	{
		SND_ERROR("AudioStreamer::LoadSoundStream(): Could not create voice");

		delete s;
		return NULL;
	}

	soundguard.Lock();
	sound.push_back(s);
	soundguard.Unlock();

	return s;
}
//=============================================================================================================
void AudioStreamer::Update()
{
	Sound* s;
	XAUDIO2_VOICE_STATE state;
	soundlist canstream;
	
	int stream;
	long readbytes;
	long totalread;
	long toread;
	bool play;

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	while( running )
	{
		canstream.clear();

		// collect sound that need more data
		soundguard.Lock();

		for( soundlist::iterator it = sound.begin(); it != sound.end(); ++it )
		{
			s = (*it);

			if( s->isstream )
			{
				if( (s->readsize < s->totalsize) && (s->freebuffers > 0) )
					canstream.push_back(s);
			}
		}

		soundguard.Unlock();

		// update collected sound
		if( canstream.size() > 0 )
		{
			for( soundlist::iterator it = canstream.begin(); it != canstream.end(); ++it )
			{
				// TODO: kell-e lockolni a soundot? (ld. Sound::OnBufferEnd())
				s = (*it);

				totalread = 0;
				readbytes = 0;
				toread = min(STREAMING_BUFFER_SIZE, s->totalsize - s->readsize);
				
				// fill a free buffer with data
				do
				{
					readbytes = ov_read(&s->oggfile, s->data[s->currentbuffer] + totalread, toread - totalread, 0, 2, 1, &stream);
					totalread += readbytes;
				}
				while( readbytes > 0 && totalread < toread );

				s->readsize += totalread;
				SND_DEBUG("buffer " << s->currentbuffer << " loaded");

				// pull buffer to queue if possible
				// otherwise the sound is gonna pull it
				s->voice->GetState(&state);

				if( state.BuffersQueued < MAX_BUFFER_COUNT - 1 )
				{
					s->PullBuffer(s->currentbuffer);
					SND_DEBUG("buffer " << s->currentbuffer << " pulled (instantly)");
					
					// lock it so noone can modify s->playing
					s->playguard.Lock();
					{
						s->canplay = true;
						play = (s->playrequest && !s->playing);
					}
					s->playguard.Unlock();

					if( play )
						s->Play();
				}
				
				s->currentbuffer = (s->currentbuffer + 1) % MAX_BUFFER_COUNT;
				--s->freebuffers;
			}
		}
		else
			Sleep(100);
	}

	CoUninitialize();
}
//=============================================================================================================
