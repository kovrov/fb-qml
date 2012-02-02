#ifndef LEVELCOMPONENT_H
#define LEVELCOMPONENT_H

#include <QDeclarativeItem>
#include <QByteArray>

namespace FlashbackData { class Level; }



class LevelComponent : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS (Direction)

public:
    enum Direction { UP=0x00, DOWN=0x40, RIGHT=0x80, LEFT=0xC0 };

    explicit LevelComponent(QDeclarativeItem *parent = 0);
    ~LevelComponent();

    Q_INVOKABLE int adjacentRoomAt(int i, Direction dir);
    Q_INVOKABLE void load(int level);

signals:

public slots:

private:
    FlashbackData::Level *m_mevel;
};



#endif // LEVELCOMPONENT_H
