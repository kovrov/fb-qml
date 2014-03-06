#ifndef DATA_H
#define DATA_H

#include <QByteArray>
#include <QImage>
#include <QSharedPointer>


/**
 * Collection of routines used to access/decode original resources
 */
namespace FlashbackData
{
    /* level topology:
    BNQ
    CT  adjacent rooms, collision
    LEV
    MAP bitmap, parette table
    MBK
    PAL palettes
    RP
    */
    /* level objects:
    ANI
    OBJ
    PGE objects
    TBN
    */
    class Level
    {
    public:

        static QSharedPointer<Level> load(int level);
        ~Level();
        int initialRoom() const;
        const QByteArray &adjacentRooms() const;
        QImage roomBitmap(int room) const;

    private:
        Level(int level);

        class CT *m_CT;    // adjacent rooms, collision
        class MAP *m_MAP;  // bitmap, parette table
        class PAL *m_PAL;  // palettes
        class PGE *m_PGE;  // objects
    };
}



#endif // DATA_H
