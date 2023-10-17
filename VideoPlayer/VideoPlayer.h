#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>

#include <QtCore/QThread>


class Demuxer;
class Decoder;

class YUVDisplayPanel;

class VideoPlayer : public QMainWindow
{
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = Q_NULLPTR);

private slots:
	void onError(QString msg);

	void onDemuxerFinished();
	void onAudioDecoderFinished();
	void onVideoDecoderFinished();

	void onOpenFile();
	void onPlay();
	void onProgressPressed();
	void onProgressReleased();

	void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
	QPushButton *buttonOpenFile = Q_NULLPTR;
	QPushButton *buttonPlay = Q_NULLPTR;
	QSlider *sliderProgress = Q_NULLPTR;
	bool isProgressPressed = false;
	Demuxer *demuxer = Q_NULLPTR;
	Decoder *videoDecoder = Q_NULLPTR;
	Decoder *audioDecoder = Q_NULLPTR;

	QThread *demuxerThread = Q_NULLPTR;
	QThread *videoDecoderThread = Q_NULLPTR;
	QThread *audioDecoderThread = Q_NULLPTR;

	YUVDisplayPanel *displayPanel = Q_NULLPTR;

	static const int WindowWidth = 1280;
	static const int WindowHeight = 760;
};
