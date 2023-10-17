#include "AudioPlaybackDevice.h"

#include <QtMultimedia/QAudioFormat>

AudioPlaybackDevice::AudioPlaybackDevice()
{

}

AudioPlaybackDevice::~AudioPlaybackDevice()
{

}

bool AudioPlaybackDevice::Init(int srate, int ssize, int nchannels)
{
	Release();
	sampleRate = srate;
	sampleSize = ssize;
	channels = nchannels;

	QAudioFormat audioFormat;

	audioFormat.setSampleRate(sampleRate);
	audioFormat.setSampleSize(sampleSize);
	audioFormat.setChannelCount(channels);
	audioFormat.setCodec("audio/pcm");
	audioFormat.setByteOrder(QAudioFormat::LittleEndian);
	audioFormat.setSampleType(QAudioFormat::SignedInt);

	output = new QAudioOutput(audioFormat);
	device = output->start();
	return device != nullptr;
}

void AudioPlaybackDevice::Release()
{
	if (device)
	{
		device->close();
		device = Q_NULLPTR;
	}

	if (output)
	{
		output->stop();
		delete output;
		output = Q_NULLPTR;
	}
}

int AudioPlaybackDevice::getAvailableSpace() const
{
	return output != Q_NULLPTR ? output->bytesFree() : 0;
}

int AudioPlaybackDevice::write(const char *data, int len)
{
	return device != Q_NULLPTR ? device->write(data, len) : 0;
}

AudioPlaybackDevice& AudioPlaybackDevice::getInstance()
{
	static AudioPlaybackDevice instance;
	return instance;
}
