#include "VideoPlayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::addLibraryPath("d:/projects/3rdparty/Qt5.6.0/5.6/msvc2015/plugins/");
    QApplication a(argc, argv);
    VideoPlayer w;
    w.show();
    return a.exec();
}
