#include "VideoPlayer.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QFileDialog>

#include <QtCore/QDebug>
// #include <QtCore/QTimer>
#include <QtWidgets/QMessageBox>

#include "Demuxer.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "YUVDisplayPanel.h"


extern "C"
{
#include <libavcodec/avcodec.h>
}


VideoPlayer::VideoPlayer(QWidget *parent)
    : QMainWindow(parent)
{
	const QList<QScreen*> screens = QGuiApplication::screens();
	const QRect rect = screens.at(0)->geometry();
	setGeometry((rect.width() - WindowWidth) / 2, (rect.height() - WindowHeight) / 2, WindowWidth, WindowHeight);

	QMenu *menuFile = menuBar()->addMenu(QString::fromLocal8Bit("File"));
	QAction *actionOpen = new QAction(QString::fromLocal8Bit("Open..."), this);
	menuFile->addAction(actionOpen);
	connect(actionOpen, &QAction::triggered, this, &VideoPlayer::onOpenFile);

	buttonOpenFile = new QPushButton(QString::fromLocal8Bit("Open File"));
	buttonPlay = new QPushButton(QString::fromLocal8Bit("Play/Pause"));
	buttonPlay->setEnabled(false);
	connect(buttonOpenFile, &QPushButton::clicked, this, &VideoPlayer::onOpenFile);
	connect(buttonPlay, &QPushButton::clicked, this, &VideoPlayer::onPlay);

	sliderProgress = new QSlider(this);
	sliderProgress->setOrientation(Qt::Horizontal);
	sliderProgress->setMinimum(0);
	sliderProgress->setMaximum(100);
	sliderProgress->setSingleStep(1);
	connect(sliderProgress, &QSlider::sliderPressed, this, &VideoPlayer::onProgressPressed);
	connect(sliderProgress, &QSlider::sliderReleased, this, &VideoPlayer::onProgressReleased);

	statusBar()->addWidget(sliderProgress);
	statusBar()->addWidget(buttonOpenFile);
	statusBar()->addWidget(buttonPlay);

	displayPanel = new YUVDisplayPanel();
	setCentralWidget(displayPanel);
}

void VideoPlayer::onError(QString msg)
{
	QMessageBox::critical(this, QString::fromLocal8Bit("Fatal error"), msg);
}

void VideoPlayer::onDemuxerFinished()
{
	QMessageBox::information(this, QString::fromLocal8Bit("Signal"), QString::fromLocal8Bit("Demuxer finished"));
	if (videoDecoder)
	{
		videoDecoder->setNeedStop();
	}
	if (audioDecoder)
	{
		audioDecoder->setNeedStop();
	}
}

void VideoPlayer::onAudioDecoderFinished()
{
	QMessageBox::information(this, QString::fromLocal8Bit("Signal"), QString::fromLocal8Bit("Audio decoder finished"));
}

void VideoPlayer::onVideoDecoderFinished()
{
	QMessageBox::information(this, QString::fromLocal8Bit("Signal"), QString::fromLocal8Bit("Video decoder finished"));
}

void VideoPlayer::onOpenFile()
{
	QString fname = QFileDialog::getOpenFileName(
		this,
		QString::fromLocal8Bit("Open media file"),
		QString::fromLocal8Bit("."),
		QString::fromLocal8Bit("MPEG-4 (*.mp4);; WebM (*.webm);; All files(*.*)"));

	if (fname.isEmpty())
	{
		return;
	}

	setWindowTitle(fname);
	buttonPlay->setEnabled(true);
	buttonPlay->setText("Play");

	try {
		demuxer = new Demuxer(fname);
		const AVCodecParameters *videoCodecParameters = demuxer->getVideoStreamParameters();
		videoDecoder = new VideoDecoder(videoCodecParameters, displayPanel);
		const AVCodecParameters *audioCodecParameters = demuxer->getAudioStreamParameters();
		audioDecoder = new AudioDecoder(audioCodecParameters);
	}
	catch (const std::exception& ex) {
		qDebug() << QString::fromLocal8Bit("Exception when opening file: ")
			<< QString::fromLocal8Bit(ex.what());
		QMessageBox::critical(this, QString::fromLocal8Bit("Error when opening file"), QString::fromLocal8Bit(ex.what()));
		return;
	}

	demuxerThread = new QThread;
	demuxer->moveToThread(demuxerThread);
	connect(demuxer, SIGNAL(error(QString)), this, SLOT(onError(QString)));
	connect(demuxer, SIGNAL(finished()), this, SLOT(onDemuxerFinished()));
	connect(demuxerThread, SIGNAL(started()), demuxer, SLOT(process()));
	connect(demuxer, SIGNAL(finished()), demuxerThread, SLOT(quit()));
	connect(demuxer, SIGNAL(finished()), demuxer, SLOT(deleteLater()));
	connect(demuxerThread, SIGNAL(finished()), demuxerThread, SLOT(deleteLater()));

	if (videoDecoder)
	{
		videoDecoderThread = new QThread;
		videoDecoder->moveToThread(videoDecoderThread);
		connect(videoDecoder, SIGNAL(error(QString)), this, SLOT(onError(QString)));
		connect(videoDecoder, SIGNAL(finished()), this, SLOT(onVideoDecoderFinished()));
		connect(videoDecoderThread, SIGNAL(started()), videoDecoder, SLOT(process()));
		connect(videoDecoder, SIGNAL(finished()), videoDecoderThread, SLOT(quit()));
		connect(videoDecoder, SIGNAL(finished()), videoDecoder, SLOT(deleteLater()));
		connect(videoDecoderThread, SIGNAL(finished()), videoDecoderThread, SLOT(deleteLater()));
		demuxer->setVideoDecoder(videoDecoder);
	}

	if (audioDecoder)
	{
		audioDecoderThread = new QThread;
		audioDecoder->moveToThread(audioDecoderThread);
		connect(audioDecoder, SIGNAL(error(QString)), this, SLOT(onError(QString)));
		connect(audioDecoder, SIGNAL(finished()), this, SLOT(onAudioDecoderFinished()));
		connect(audioDecoderThread, SIGNAL(started()), audioDecoder, SLOT(process()));
		connect(audioDecoder, SIGNAL(finished()), audioDecoderThread, SLOT(quit()));
		connect(audioDecoder, SIGNAL(finished()), audioDecoder, SLOT(deleteLater()));
		connect(audioDecoderThread, SIGNAL(finished()), audioDecoderThread, SLOT(deleteLater()));
		demuxer->setAudioDecoder(audioDecoder);
	}
}

void VideoPlayer::onPlay()
{
	if (demuxerThread)
	{
		demuxerThread->start();
	}

	if (audioDecoderThread)
	{
		audioDecoderThread->start();
	}

	if (videoDecoderThread)
	{
		videoDecoderThread->start();
	}
}

void VideoPlayer::onProgressPressed()
{
	isProgressPressed = true;
}

void VideoPlayer::onProgressReleased()
{
	isProgressPressed = false;
	double pos = static_cast<double>(sliderProgress->value()) / static_cast<double>(sliderProgress->maximum());
	qDebug() << QString::fromLocal8Bit("Seek to ") << pos;
}

void VideoPlayer::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (isFullScreen())
	{
		showNormal();
	}
	else
	{
		showFullScreen();
	}
}
