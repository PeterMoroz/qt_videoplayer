#include "VideoDecoder.h"

#include "YUVDisplayPanel.h"
#include "ReferenceClock.h"

#include <QtCore/QDebug>


extern "C"
{
#include <libavutil/time.h>
}

VideoDecoder::VideoDecoder(const AVCodecParameters *params, YUVDisplayPanel *displayPanel)
	: Decoder(params, MaxQueueSizeBytes)
	, display(displayPanel)
{
	display->Init(params->width, params->height);
	frameTimer = static_cast<double>(av_gettime()) / 1000000.0;
	prevDelay = 40e-3;
}

void VideoDecoder::processFrame(AVFrame *frame)
{
	// adjust clock
	double frameDelay = av_q2d(codecContext->time_base);
	frameDelay += frame->repeat_pict * (frameDelay * 0.5);
	clock += frameDelay;

	// synchronize with reference clock
	double pts = 0.0;
	if (frame->pts != AV_NOPTS_VALUE)
	{
		pts = av_frame_get_best_effort_timestamp(frame);
	}
	pts *= av_q2d(codecContext->time_base);
	if (pts == 0.0)
	{
		pts = clock;
	}

	double delay = pts - prevPTS;
	if (delay <= 0.0 || delay >= 1.0)
	{
		delay = prevDelay;
	}

	prevDelay = delay;
	prevPTS = pts;

	static const double SyncThreshold = 0.01;
	static const double NosyncThreshold = 10.0;

	double refClock = ReferenceClock::getInstance().getValue();
	double diff = pts - refClock;
	double syncThreshold = (delay > SyncThreshold ? delay : SyncThreshold);
	if (std::fabs(diff) < NosyncThreshold)
	{
		if (diff <= -syncThreshold)
		{
			delay = 0.0;
		}
		else if (diff >= syncThreshold)
		{
			delay = 2.0 * delay;
		}
	}

	qDebug() << QString::fromLocal8Bit("delay: ") << delay;

	frameTimer += delay;
	// double actualDelay = frameTimer - (av_gettime() / 1000000.0);
	double actualDelay = (av_gettime() / 1000000.0) - frameTimer;
	qDebug() << QString::fromLocal8Bit("actual delay: ") << actualDelay;
	if (actualDelay < 0.010)
	{
		actualDelay = 0.010;
	}

	qDebug() << QString::fromLocal8Bit("actual delay: ") << actualDelay << " | " << static_cast<int>(actualDelay * 1000 + 0.5);
	// qDebug() << QString::fromLocal8Bit("current time: ") << (av_gettime() / 1000000);

	display->DrawFrame(frame, static_cast<int>(actualDelay * 1000 + 0.5));
}
