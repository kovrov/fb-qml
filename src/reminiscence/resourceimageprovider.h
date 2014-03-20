#ifndef RESOURCEIMAGEPROVIDER_H
#define RESOURCEIMAGEPROVIDER_H

#include <QtQuick/QQuickImageProvider>


class MenuImageProvider : public QQuickImageProvider
{
public:
    MenuImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
};


class LevelImageProvider : public QQuickImageProvider
{
public:
    LevelImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
};



#endif // RESOURCEIMAGEPROVIDER_H
