#include <QApplication>
#include "StreamWidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // StreamWidget 内部现在包含了所有配置和自动启动逻辑
    StreamWidget w;
    w.setWindowTitle("3-Channel RTSP Monitor (Embedded Config)");
    w.show();

    return a.exec();
}
