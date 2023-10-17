#include "VideoDecoder.h"

#include "YUVDisplayPanel.h"

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
