#include <qqml.h>
#include <QtQml/QQmlEngine>

#include "cutscenewidget.h"
#include "levelcomponent.h"
#include "resourceimageprovider.h"
#include "plugin.h"


void ReminiscencePlugin::registerTypes(const char *uri)
{
    // @uri reminiscence
    qmlRegisterType<CutsceneWidget>(uri, 1, 0, "Cutscene");
    qmlRegisterType<LevelComponent>(uri, 1, 0, "Level");
}


void ReminiscencePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED (uri)
    engine->addImageProvider(QLatin1String("menu"), new MenuImageProvider);
    engine->addImageProvider(QLatin1String("level"), new LevelImageProvider);
    engine->addImageProvider(QLatin1String("icon"), new IconImageProvider);
}
