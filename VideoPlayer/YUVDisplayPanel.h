#pragma once

#include <QtWidgets/QOpenGLWidget>
#include <QtGui/QOpenGLFunctions>
#include <QtOpenGL/QGLShaderProgram>

extern "C"
{
#include <libavutil/frame.h>
}


class YUVDisplayPanel : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	YUVDisplayPanel(QWidget *parent = nullptr);
	virtual ~YUVDisplayPanel() = default;

	void Init(int w, int h);
	void DrawFrame(AVFrame *frame);

protected:
	void initializeGL() override;
	// void resizeGL(int w, int h) override;
	void paintGL() override;

public slots:
	void scheduleUpdate(int delay);


private:
	int width = 0;
	int height = 0;

	QGLShaderProgram shaderProgram;
	GLuint uniformLocations[3] = { 0 };
	GLuint textures[3] = { 0 };

	unsigned char* pixeldata[3] = { nullptr };
};
