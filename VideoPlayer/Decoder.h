#pragma once

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

extern "C"
{
#include <libavcodec/avcodec.h>
}

class Decoder : public QObject
{
	Q_OBJECT

protected:
	explicit Decoder(const AVCodecParameters *params, const int maxAvailBytes);

public:
	virtual ~Decoder();

	void enquePacket(const AVPacket& packet);

	bool queueOverrun() const { return packetsSizeBytes > maxAvailableBytes; }

	void setNeedStop();

	double getClock() const { return clock; }

private:
	bool dequePacket(AVPacket &packet);

	virtual void processFrame(AVFrame *frame) = 0;
	virtual bool needData() const { return true; }

private slots:
	void process();

signals:
	void finished();
	void error(QString msg);

protected:
	AVCodecContext *codecContext = NULL;

private:
	const int maxAvailableBytes = 0;
	bool needStop = false;
	QList<AVPacket> packets;
	QMutex packetsMutex;
	QWaitCondition packetsWait;
	int packetsSizeBytes = 0;

protected:
	double clock = 0;
};
