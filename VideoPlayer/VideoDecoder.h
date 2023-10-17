#pragma once

#include "Decoder.h"


class YUVDisplayPanel;

class VideoDecoder : public Decoder
{
	static const int MaxQueueSizeBytes = 5 * 256 * 1024;

public:
	explicit VideoDecoder(const AVCodecParameters *params, YUVDisplayPanel *displayPanel);

private:
	void processFrame(AVFrame *frame) override;

private:
	YUVDisplayPanel *display = NULL;

	double prevDelay = 0.0;
	double prevPTS = 0.0;
	double frameTimer = 0.0;
};
