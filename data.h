#ifndef DATA_H
#define DATA_H

#include <QByteArray>
#include <QVector>
#include <QRgb>
#include <QImage>


/**
 * Collection of routines used to access/decode original resources
 */
namespace FlashbackData
{
    /*
    ANI
    BNQ
    CT  adjacent rooms, collision
    LEV
    MAP bitmap, parette table
    MBK
    OBJ
    PAK
    PAL palettes
    PGE
    RP
    SGD
    TBN
    */
    class Level
    {
    public:
        Level();
        ~Level();
        void load(int level);
        const QByteArray &adjacentRooms();
        QImage roomBitmap(int room);

    private:
        class CT *m_CT;    // adjacent rooms, collision
        class MAP *m_MAP;  // bitmap, parette table
        class PAL *m_PAL;  // palettes
    };
}



#endif // DATA_H
