#pragma once

#include <QtMultimedia/QAudioOutput>

class AudioPlaybackDevice final
{
private:
	AudioPlaybackDevice();

public:

	~AudioPlaybackDevice();

	bool Init(int sampleRate, int sampleSize, int channels);
	void Release();
	int getAvailableSpace() const;
	int write(const char *data, int len);

	static AudioPlaybackDevice& getInstance();

private:
	int sampleRate;
	int sampleSize;
	int channels;

	QAudioOutput *output = Q_NULLPTR;
	QIODevice *device = Q_NULLPTR;
};
