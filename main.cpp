#include <QtGui/QApplication>
#include "polywidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PolyWidget w;
    w.show();

    return a.exec();
}
