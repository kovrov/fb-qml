#ifndef RESOURCEIMAGEPROVIDER_H
#define RESOURCEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>



class MenuImageProvider : public QDeclarativeImageProvider
{
public:
    MenuImageProvider() : QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
};



class LevelImageProvider : public QDeclarativeImageProvider
{
public:
    LevelImageProvider() : QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
};



#endif // RESOURCEIMAGEPROVIDER_H
