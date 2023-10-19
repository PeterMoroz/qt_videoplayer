#include "Decoder.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QMutexLocker>

#include <stdexcept>

Decoder::Decoder(const AVCodecParameters *params, const int maxAvailBytes)
	: maxAvailableBytes(maxAvailBytes)
{
	AVCodec *codec = avcodec_find_decoder(params->codec_id);
	if (codec == NULL)
	{
		qDebug() << QString::fromLocal8Bit("Could not find decoder for the codec: ")
			<< QString::fromLocal8Bit(avcodec_get_name(params->codec_id));
		throw std::logic_error("Decoder - could not find decoder for the given codec");
	}

	codecContext = avcodec_alloc_context3(codec);
	if (codecContext == NULL)
	{
		qDebug() << QString::fromLocal8Bit("Failure of avcodec_alloc_context3");
		throw std::runtime_error("Decoder - could not allocate codec context.");
	}

	int rc = avcodec_parameters_to_context(codecContext, params);
	if (rc < 0)
	{
		char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		qDebug() << QString::fromLocal8Bit("Failure of avcodec_parameters_to_context : ")
			<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
		throw std::runtime_error("Decoder - could not copy codec parameters into context.");
	}

	// what is the purpose of the field thread_count ?
	// codecContext->thread_count = 8;

	rc = avcodec_open2(codecContext, codec, NULL);
	if (rc < 0)
	{
		char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		qDebug() << QString::fromLocal8Bit("Failure of avcodec_open2 : ")
			<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
		throw std::runtime_error("Decoder - could not open codec context.");
	}

	qDebug() << QString::fromLocal8Bit("codec: ") << QString::fromLocal8Bit(codec->name);
	qDebug() << QString::fromLocal8Bit("timebase: ") << av_q2d(codecContext->time_base);
}

Decoder::~Decoder()
{
	if (codecContext)
	{
		avcodec_free_context(&codecContext);
		codecContext = NULL;
	}

	qDebug() << QString::fromLocal8Bit("Decoder d-tor.");
}

void Decoder::enquePacket(const AVPacket &packet)
{
	QMutexLocker lock(&packetsMutex);
	packets.push_back(packet);
	packetsSizeBytes += packet.size;
	packetsWait.wakeOne();
}

void Decoder::setNeedStop()
{
	needStop = true;
	packetsWait.wakeOne();
}


bool Decoder::dequePacket(AVPacket &packet)
{
	QMutexLocker lock(&packetsMutex);
	while (packets.empty() && !needStop)
	{
		packetsWait.wait(&packetsMutex);
	}

	if (packets.empty())
	{
		return false;
	}

	packet = packets.front();
	packets.pop_front();
	packetsSizeBytes -= packet.size;
	return true;
}

void Decoder::process()
{
	AVFrame *frame = av_frame_alloc();
	while (true)
	{
		if (!needData())
		{
			QThread::msleep(40);
		}
		AVPacket packet;
		if (!dequePacket(packet) && needStop)
		{
			break;
		}

		// qDebug() << "Decoder - packet PTS: " << packet.pts;
		int rc = avcodec_send_packet(codecContext, &packet);
		if (rc != 0)
		{
			av_free_packet(&packet);
			char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			qDebug() << QString::fromLocal8Bit("Failure of avcodec_send_packet : ")
				<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
			emit error(QString::fromLocal8Bit("Decoder - could not send packet."));
			continue;
		}

		qDebug() << "Decoder - codec name: " << avcodec_get_name(codecContext->codec_id)
			<< ", packet PTS: " << packet.pts << ", packet time: " << av_q2d(codecContext->time_base) * packet.pts;

		av_free_packet(&packet);

		//unsigned rcvFrameCount = 0;
		while (rc >= 0)
		{
			rc = avcodec_receive_frame(codecContext, frame);
			//if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF)
			//	break;
			if (rc == AVERROR(EAGAIN))
				continue;
			if (rc == AVERROR_EOF)
				break;
			if (rc < 0)
			{
				char errbuffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
				qDebug() << QString::fromLocal8Bit("Failure of avcodec_receive_frame : ")
					<< QString::fromLocal8Bit(av_make_error_string(errbuffer, sizeof(errbuffer), rc));
				emit error(QString::fromLocal8Bit("Decoder - could not receive frame."));
				break;
			}

			// rcvFrameCount++;
			qDebug() << "Decoder - codec name: " << avcodec_get_name(codecContext->codec_id)
				<< ", frame PTS: " << frame->pts << ", frame time: " << av_q2d(codecContext->time_base) * frame->pts;
			processFrame(frame);
		}
		// qDebug() << QString::fromLocal8Bit("the frame count received from decoder: ") << rcvFrameCount;
	}

	av_frame_free(&frame);

	qDebug() << "Decoder - codec name: " << avcodec_get_name(codecContext->codec_id) << " finished.";

	emit finished();
}
