#include "VideoDecoder.h"

#include "YUVDisplayPanel.h"

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
	QObject::connect(this, &VideoDecoder::scheduleUpdateDisplay, display, &YUVDisplayPanel::scheduleUpdate);
}

void VideoDecoder::processFrame(AVFrame *frame)
{
	display->DrawFrame(frame);
	emit scheduleUpdateDisplay(40);
}
