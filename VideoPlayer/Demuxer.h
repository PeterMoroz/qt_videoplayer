#pragma once

#include <QtCore/QObject>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class Decoder;

class Demuxer final : public QObject 
{
	Q_OBJECT

public:
	explicit Demuxer(const QString& fname);
	~Demuxer();

	const AVCodecParameters *getAudioStreamParameters() const;
	const AVCodecParameters *getVideoStreamParameters() const;

	void setVideoDecoder(Decoder *decoder)
	{
		videoDecoder = decoder;
	}

	void setAudioDecoder(Decoder *decoder)
	{
		audioDecoder = decoder;
	}

private slots:
	void process();

signals:
	void finished();
	void error(QString msg);

private:
	AVFormatContext *formatContext = NULL;
	bool needStop = false;
	int videoStreamIndex = -1;
	int audioStreamIndex = -1;

	Decoder *videoDecoder = NULL;
	Decoder *audioDecoder = NULL;
};
