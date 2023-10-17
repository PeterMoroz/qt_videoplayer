#include "AudioDecoder.h"
#include "AudioResampler.h"
#include "AudioPlaybackDevice.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>

AudioDecoder::AudioDecoder(const AVCodecParameters *params)
	: Decoder(params, MaxQueueSizeBytes)
{
	resampler = new AudioResampler(params, AV_SAMPLE_FMT_S16, SampleRate, ChannelsNum);
	samplesBuffer = new unsigned char[SamplesBufferSize];

	if (!AudioPlaybackDevice::getInstance().Init(SampleRate, 16, ChannelsNum))
	{
		qDebug() << QString::fromLocal8Bit("Could not initialize AudioPlaybackDevice");
		throw std::logic_error("AudioDecoder - could not initialize AudioPlaybackDevice");
	}
}

AudioDecoder::~AudioDecoder()
{
	delete resampler;
	delete [] samplesBuffer;
	AudioPlaybackDevice::getInstance().Release();
}

void AudioDecoder::processFrame(AVFrame *frame)
{
	int samplesByteSize = 0;
	if (resampler->Resample(frame, samplesBuffer, samplesByteSize))
	{
		if (!samplesByteSize)
		{
			//qDebug() << QString::fromLocal8Bit("AudioDecoder - samplesByteSize == 0.");
			return;
		}
		AudioPlaybackDevice& audioPlaybackDevice = AudioPlaybackDevice::getInstance();
		while (true)
		{ 
			int availableSpace = audioPlaybackDevice.getAvailableSpace();
			//qDebug() << QString::fromLocal8Bit("available space: ") << availableSpace;
			//qDebug() << QString::fromLocal8Bit("samplesByteSize: ") << samplesByteSize;
			if (samplesByteSize <= availableSpace)
			{
				int written = audioPlaybackDevice.write((char *)samplesBuffer, samplesByteSize);
				//qDebug() << QString::fromLocal8Bit("written: ") << written;
				return;
			}
			else
			{
				int written = audioPlaybackDevice.write((char *)samplesBuffer, availableSpace);
				// qDebug() << QString::fromLocal8Bit("written: ") << written;
				int remainBytes = samplesByteSize - written;
				std::memcpy(samplesBuffer, samplesBuffer + written, remainBytes);
				samplesByteSize -= written;

				static const int Divisor = SampleRate * ChannelsNum * 2;	// each sample is 16-bit or 2 bytes wide
				clock -= static_cast<double>(written) / static_cast<double>(Divisor);

				if (samplesByteSize == 0)
				{
					return;
				}
				QThread::msleep(100);
			}
		}
	}
	else
	{
		qDebug() << QString::fromLocal8Bit("AudioDecoder - resample failed.");
	}
}

bool AudioDecoder::needData() const
{
	return AudioPlaybackDevice::getInstance().getAvailableSpace() > 0;
}
