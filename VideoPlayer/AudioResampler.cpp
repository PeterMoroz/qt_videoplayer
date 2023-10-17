#include "AudioResampler.h"

#include <QtCore/QDebug>

#include <stdexcept>


AudioResampler::AudioResampler(const AVCodecParameters *params, AVSampleFormat outFmt, int outSampleRate, int outChannelsNum)
	: outFormat(outFmt)
	, outChannels(outChannelsNum)
{
	swrContext = swr_alloc_set_opts(NULL,
		av_get_default_channel_layout(outChannels), outFormat, outSampleRate,
		av_get_default_channel_layout(params->channels), (AVSampleFormat)params->format, params->sample_rate,
		0, NULL);
	if (swrContext == NULL)
	{
		qDebug() << QString::fromLocal8Bit("Could not allocate swr context.");
		throw std::runtime_error("AudioResampler - Could not allocate swr context.");
	}

	int rc = swr_init(swrContext);
	if (rc != 0)
	{
		char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		qDebug() << QString::fromLocal8Bit("Failure of swr_init : ")
			<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
		throw std::runtime_error("AudioResampler - could not initialize swr context.");
	}
}

AudioResampler::~AudioResampler()
{
	if (swrContext)
	{
		swr_free(&swrContext);
		swrContext = NULL;
	}
}

bool AudioResampler::Resample(AVFrame *frame, unsigned char *samplesBuffer, int& samplesSize)
{
	uint8_t *out[2] = { NULL };
	out[0] = samplesBuffer;
	int outSamplesCount = frame->nb_samples;
	int rc = swr_convert(swrContext, out, outSamplesCount, (const uint8_t **)frame->data, frame->nb_samples);
	if (rc < 0)
	{ 
		char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		qDebug() << QString::fromLocal8Bit("Failure of swr_convert : ")
			<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
		return false;
	}

	samplesSize = rc * outChannels * av_get_bytes_per_sample(outFormat);
	return true;
}
