
#ifndef _AUDIOSTREAMER_H_
#define _AUDIOSTREAMER_H_

#include <xaudio2.h>
#include <vorbis/vorbisfile.h>
#include <string>
#include <list>

#include "thread.h"

#define STREAMING_BUFFER_SIZE	65536
#define MAX_BUFFER_COUNT		3

#define SND_ERROR(x)  { std::cout << "* SOUND: Error: " << x << "!\n"; }
#define SAFE_DELETE_ARRAY(x) { if( x ) { delete[] (x); (x) = NULL; } }

#ifdef _DEBUG
#	define SND_DEBUG(x)  { std::cout << "* SOUND: " << x << "\n"; }
#else
#	define SND_DEBUG(x)
#endif

class Sound : public IXAudio2VoiceCallback
{
	friend class AudioStreamer;
	
private:
	WAVEFORMATEX			format;
	OggVorbis_File			oggfile;
	Guard					playguard;
	IXAudio2SourceVoice*	voice;
	char*					data[MAX_BUFFER_COUNT];	// shared
	int						ids[MAX_BUFFER_COUNT];

	unsigned int totalsize;
	unsigned int readsize;
	unsigned int currentbuffer;
	unsigned int freebuffers;

	bool isstream;
	bool canplay;		// shared
	bool playing;		// shared
	bool playrequest;	// shared

	STDMETHOD_(void, OnVoiceProcessingPassStart )(UINT32) {}
	STDMETHOD_(void, OnVoiceProcessingPassEnd)() {}
	STDMETHOD_(void, OnStreamEnd)() {}
	STDMETHOD_(void, OnBufferStart)(void*) {}
	STDMETHOD_(void, OnLoopEnd)(void*) {}
	STDMETHOD_(void, OnVoiceError)(void*, HRESULT) {}
	STDMETHOD_(void, OnBufferEnd)(void* context);

	void PullBuffer(int id);
	
public:
	Sound();
	~Sound();

	void Play();

	inline IXAudio2SourceVoice* GetVoice() {
		return voice;
	}
};

class AudioStreamer
{
private:
	typedef std::list<Sound*> soundlist;

	soundlist	sound;		// shared
	Guard		soundguard;
	bool		running;	// shared

public:
	AudioStreamer();
	~AudioStreamer();

	Sound* LoadSound(IXAudio2* xaudio2, const std::string& file);
	Sound* LoadSoundStream(IXAudio2* xaudio2, const std::string& file);

	void Destroy();
	void Update();
};

#endif
