#pragma once

extern "C"
{
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

class AudioResampler final
{
public:
	explicit AudioResampler(const AVCodecParameters *params, AVSampleFormat outFmt, int outSampleRate, int outChannelsNum);

	~AudioResampler();

	bool Resample(AVFrame *frame, unsigned char *samplesBuffer, int& samplesSize);


private:
	AVSampleFormat outFormat = AV_SAMPLE_FMT_NONE;
	int outChannels = 0;
	SwrContext *swrContext = NULL;
};
