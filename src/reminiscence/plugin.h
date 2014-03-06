#ifndef REMINISCENCE_PLUGIN_H
#define REMINISCENCE_PLUGIN_H

#include <QQmlExtensionPlugin>


class ReminiscencePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri);
    void initializeEngine(QQmlEngine *engine, const char *uri);
};

#endif // REMINISCENCE_PLUGIN_H
