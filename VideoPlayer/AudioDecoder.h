#pragma once

#include "Decoder.h"
#include "ClockSource.h"

class AudioResampler;

class AudioDecoder : public Decoder, public ClockSource
{
	static const int MaxQueueSizeBytes = 5 * 16 * 1024;
	static const size_t SamplesBufferSize = 32768;	// just an arbitrary size, might be needed to adjust it

	static const int SampleRate = 44100;	// 48000
	static const int ChannelsNum = 2;

public:
	explicit AudioDecoder(const AVCodecParameters *params);
	~AudioDecoder();

	double getClock() const override
	{
		return clock;
	}

private:
	void processFrame(AVFrame *frame) override;
	bool needData() const override;

private:
	AudioResampler *resampler = NULL;
	unsigned char *samplesBuffer = NULL;
};
