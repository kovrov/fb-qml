#include <QtGui/QApplication>
#include <QDeclarativeEngine>
#include "qmlapplicationviewer.h"

#include "cutscenewidget.h"
#include "resourceimageprovider.h"



Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QApplication> app(createApplication(argc, argv));

    qmlRegisterType<CutsceneWidget>("Flashback", 1, 0, "Cutscene");

    QmlApplicationViewer viewer;
    viewer.engine()->addImageProvider(QLatin1String("menu"), new MenuImageProvider);
    viewer.engine()->addImageProvider(QLatin1String("level"), new LevelImageProvider);
    viewer.setOrientation(QmlApplicationViewer::ScreenOrientationAuto);
    viewer.setMainQmlFile(QLatin1String("qml/re/main.qml"));
    viewer.showExpanded();

    return app->exec();
}
