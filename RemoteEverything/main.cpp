#include "RemoteEverything.h"
#include <QtWidgets/QApplication>
#include <QDateTime>
#include <windows.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RemoteEverything w;
    w.show();
    return a.exec();
}
