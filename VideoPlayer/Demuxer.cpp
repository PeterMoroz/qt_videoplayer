#include "Demuxer.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>

#include <stdexcept>

#include "Decoder.h"


Demuxer::Demuxer(const QString& fname)
{
	int rc = avformat_open_input(&formatContext, fname.toLocal8Bit(), NULL, NULL);
	if (rc != 0)
	{
		char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		qDebug() << QString::fromLocal8Bit("Failure of avformat_open_input : ")
			<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
		throw std::runtime_error("Demuxer - could not open file.");
	}

	rc = avformat_find_stream_info(formatContext, NULL);
	if (rc != 0)
	{
		char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		qDebug() << QString::fromLocal8Bit("Failure of avformat_find_stream_info : ")
			<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
		throw std::runtime_error("Demuxer - could not read stream info.");
	}

	int duration = formatContext->duration / AV_TIME_BASE;
	qDebug() << QString::fromLocal8Bit("duration (seconds): ") << duration;

	videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	AVStream *videoStream = formatContext->streams[videoStreamIndex];
	qDebug() << QString::fromLocal8Bit("-------- videostream --------");
	qDebug() << QString::fromLocal8Bit("index: ") << videoStreamIndex;
	qDebug() << QString::fromLocal8Bit("width: ") << videoStream->codecpar->width;
	qDebug() << QString::fromLocal8Bit("height: ") << videoStream->codecpar->height;
	qDebug() << QString::fromLocal8Bit("FPS: ") << av_q2d(videoStream->avg_frame_rate);
	qDebug() << QString::fromLocal8Bit("format: ") << videoStream->codecpar->format;

	audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	AVStream *audioStream = formatContext->streams[audioStreamIndex];
	qDebug() << QString::fromLocal8Bit("-------- audiostream --------");
	qDebug() << QString::fromLocal8Bit("index: ") << audioStreamIndex;
	qDebug() << QString::fromLocal8Bit("sample rate: ") << audioStream->codecpar->sample_rate;
	qDebug() << QString::fromLocal8Bit("format: ") << audioStream->codecpar->format;
	qDebug() << QString::fromLocal8Bit("channels: ") << audioStream->codecpar->channels;
	qDebug() << QString::fromLocal8Bit("codec_id: ") << audioStream->codecpar->codec_id;
	// qDebug() << QString::fromLocal8Bit("FPS: ") << avRationalToDouble(audioStream->avg_frame_rate);
	qDebug() << QString::fromLocal8Bit("frame_size: ") << audioStream->codecpar->frame_size;
}

Demuxer::~Demuxer()
{
	if (formatContext != NULL)
	{
		avformat_free_context(formatContext);
		formatContext = NULL;
	}
}

const AVCodecParameters* Demuxer::getAudioStreamParameters() const
{
	if (formatContext == NULL)
	{
		throw std::logic_error("Demuxer - format context is not opened yet.");
	}
	return audioStreamIndex != -1 ? formatContext->streams[audioStreamIndex]->codecpar : NULL;
}

const AVCodecParameters* Demuxer::getVideoStreamParameters() const
{
	if (formatContext == NULL)
	{
		throw std::logic_error("Demuxer - format context is not opened yet.");
	}
	return videoStreamIndex != -1 ? formatContext->streams[videoStreamIndex]->codecpar : NULL;
}

void Demuxer::process()
{
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	unsigned videoPacketCount = 0;
	unsigned audioPacketCount = 0;

	while (!needStop)
	{
		if (audioDecoder->queueOverrun() || videoDecoder->queueOverrun())
		{
			QThread::msleep(10);
		}

		int rc = av_read_frame(formatContext, &packet);
		if (rc == AVERROR_EOF)
			break;
		if (rc != 0)
		{
			char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			qDebug() << QString::fromLocal8Bit("Failure of av_read_frame : ")
				<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
			emit error(QString::fromLocal8Bit("Demuxer - could not read packet."));
			av_packet_unref(&packet);
			continue;
		}

		if (packet.stream_index == videoStreamIndex && videoDecoder)
		{
			videoDecoder->enquePacket(packet);
			videoPacketCount++;
		}
		else if (packet.stream_index == audioStreamIndex && audioDecoder)
		{
			audioDecoder->enquePacket(packet);
			audioPacketCount++;
		}
		else
		{
			qDebug() << QString::fromLocal8Bit("Demuxer - no decoder for packet with stream index ") << packet.stream_index;
			av_free_packet(&packet);
		}
	}

	qDebug() << QString::fromLocal8Bit("Demuxer - video packets processed: ") << videoPacketCount;
	qDebug() << QString::fromLocal8Bit("Demuxer - audio packets processed: ") << audioPacketCount;

	avformat_close_input(&formatContext);

	emit finished();
}
