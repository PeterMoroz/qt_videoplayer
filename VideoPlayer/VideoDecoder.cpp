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
}

void VideoDecoder::processFrame(AVFrame *frame)
{
	display->DrawFrame(frame);
}
