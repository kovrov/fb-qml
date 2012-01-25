#include <QtGui/QApplication>
#include "qmlapplicationviewer.h"

#include "cutscenewidget.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QApplication> app(createApplication(argc, argv));

    qmlRegisterType<CutsceneWidget>("Flashback", 1, 0, "Cutscene");

    QmlApplicationViewer viewer;
    viewer.setOrientation(QmlApplicationViewer::ScreenOrientationAuto);
    viewer.setMainQmlFile(QLatin1String("qml/re/main.qml"));
    viewer.showExpanded();

    return app->exec();
}
