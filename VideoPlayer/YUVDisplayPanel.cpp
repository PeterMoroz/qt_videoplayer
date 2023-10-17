#include "YUVDisplayPanel.h"

// #include <QtCore/QTimer>

#define STRINGIFY(x) #x

// vertex shader
const char *vertexShaderCode = STRINGIFY(
	attribute vec4 vertexIn;
	attribute vec2 textureIn;
	varying vec2 textureOut;
	void main(void)
	{
		gl_Position = vertexIn;
		textureOut = textureIn;
	}
);

// fragment shader
const char *fragmentShaderCode = STRINGIFY(
	precision mediump float;
	varying vec2 textureOut;
	uniform sampler2D tex_y;
	uniform sampler2D tex_u;
	uniform sampler2D tex_v;
	void main(void)
	{
		vec3 yuv;
		vec3 rgb;
		yuv.x = texture2D(tex_y, textureOut).r;
		yuv.y = texture2D(tex_u, textureOut).r - 0.5;
		yuv.z = texture2D(tex_v, textureOut).r - 0.5;
		rgb = mat3(1.0, 1.0, 1.0,
			0.0, -0.39465, 2.03211,
			1.13983, -0.58060, 0.0) * yuv;
		gl_FragColor = vec4(rgb, 1.0);
	}
);


// TO DO: it's possible that the pixildata should be guarded ny mutex,
// because the race condition when accessing the pixeldata is possible
// the one thread (decoder) invoke DrawFrame() whenever another one (event loop processing) invoke paintGL()

YUVDisplayPanel::YUVDisplayPanel(QWidget *parent /*= nullptr*/)
	: QOpenGLWidget(parent)
{
	QTimer *timer = new QTimer(this);
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(40);
}

void YUVDisplayPanel::Init(int w, int h)
{
	width = w;
	height = h;

	delete[] pixeldata[0];
	delete[] pixeldata[1];
	delete[] pixeldata[2];

	pixeldata[0] = new unsigned char[width * height];
	pixeldata[1] = new unsigned char[width * height / 4];
	pixeldata[2] = new unsigned char[width * height / 4];

	if (textures[0])
	{
		glDeleteTextures(3, textures);
	}
	
	glGenTextures(3, textures);

	// Y
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	// U
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	// V
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
}

void YUVDisplayPanel::DrawFrame(AVFrame *frame, int delay)
{
	if (width == frame->linesize[0])
	{
		memcpy(pixeldata[0], frame->data[0], width * height);
		memcpy(pixeldata[1], frame->data[1], width * height / 4);
		memcpy(pixeldata[2], frame->data[2], width * height / 4);
	}
	else
	{
		for (int i = 0; i < height; i++)
		{
			memcpy(pixeldata[0] + width * i, frame->data[0] + frame->linesize[0] * i, width);
		}
		for (int i = 0; i < height / 2; i++)
		{
			memcpy(pixeldata[1] + (width / 2) * i, frame->data[1] + frame->linesize[1] * i, width / 2);
			memcpy(pixeldata[2] + (width / 2) * i, frame->data[2] + frame->linesize[2] * i, width / 2);
		}
	}

	// update();
	// QTimer::singleShot(delay, this, SLOT(update()));
	// QTimer::singleShot(40, this, SLOT(update()));
}

void YUVDisplayPanel::initializeGL()
{
	initializeOpenGLFunctions();
	if (!shaderProgram.addShaderFromSourceCode(QGLShader::Vertex, vertexShaderCode))
	{
		qDebug() << QString::fromLocal8Bit("add shader from source code failed (vertex shader).");
	}

	if (!shaderProgram.addShaderFromSourceCode(QGLShader::Fragment, fragmentShaderCode))
	{
		qDebug() << QString::fromLocal8Bit("add shader from source code failed (fragment shader).");
	}

	shaderProgram.bindAttributeLocation("vertexIn", 3);
	shaderProgram.bindAttributeLocation("textureIn", 4);

	if (!shaderProgram.link())
	{
		qDebug() << QString::fromLocal8Bit("link shader program failed.");
	}

	if (!shaderProgram.bind())
	{
		qDebug() << QString::fromLocal8Bit("bind shader program failed.");
	}

	static const GLfloat vertexLocation[] = 
	{
		-1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f
	};

	static const GLfloat textureLocation[] =
	{
		0.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f
	};

	glVertexAttribPointer(3, 2, GL_FLOAT, 0, 0, vertexLocation);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 2, GL_FLOAT, 0, 0, textureLocation);
	glEnableVertexAttribArray(4);

	uniformLocations[0] = shaderProgram.uniformLocation("tex_y");
	uniformLocations[1] = shaderProgram.uniformLocation("tex_u");
	uniformLocations[2] = shaderProgram.uniformLocation("tex_v");
}

//void YUVDisplayPanel::resizeGL(int w, int h)
//{
//
//}

void YUVDisplayPanel::paintGL()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pixeldata[0]);
	glUniform1i(uniformLocations[0], 0);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, pixeldata[1]);
	glUniform1i(uniformLocations[1], 1);

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, pixeldata[2]);
	glUniform1i(uniformLocations[2], 2);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
