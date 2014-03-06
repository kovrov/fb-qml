#ifndef LEVELCOMPONENT_H
#define LEVELCOMPONENT_H

#include <QtQuick/QQuickItem>
#include <QByteArray>

namespace FlashbackData { class Level; }



class LevelComponent : public QQuickItem
{
    Q_OBJECT
    Q_ENUMS (Direction)

public:
    enum Direction { UP=0x00, DOWN=0x40, RIGHT=0x80, LEFT=0xC0 };

    explicit LevelComponent(QQuickItem *parent = 0);
    ~LevelComponent();

    Q_INVOKABLE int adjacentRoomAt(int i, Direction dir);
    Q_INVOKABLE void load(int level);

    int initialRoom() const;
    Q_SIGNAL void initialRoomChanged();
    Q_PROPERTY (int initialRoom READ initialRoom NOTIFY initialRoomChanged) // read-only

signals:

public slots:

private:
    QSharedPointer<FlashbackData::Level> m_mevel;
};



#endif // LEVELCOMPONENT_H
