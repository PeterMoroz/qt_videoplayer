#pragma once

#include "Decoder.h"


class YUVDisplayPanel;

class VideoDecoder : public Decoder
{
	static const int MaxQueueSizeBytes = 5 * 256 * 1024;

	Q_OBJECT

public:
	explicit VideoDecoder(const AVCodecParameters *params, YUVDisplayPanel *displayPanel);

private:
	void processFrame(AVFrame *frame) override;

signals:
	void scheduleUpdateDisplay(int);

private:
	YUVDisplayPanel *display = NULL;
};
