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
    /* level
    ANI
    BNQ
    CT  adjacent rooms, collision
    LEV
    MAP bitmap, parette table
    MBK BankData (to draw objects)
    OBJ
    PAL palettes
    PGE objects
    RP
    SGD
    TBN
    */
    /* global
    ADL
    BIN
    CMD
    FIB fibonacci table? SoundFx
    FNT font
    ICN inventory items
    INS
    MID
    MOD
    OFF
    POL
    PRF
    SPC animated sprtes
    SPR
    TXT
    VCE
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


    class IconDecoder
    {
    public:
        static QSharedPointer<IconDecoder> load(const QString &scope);
        ~IconDecoder();
        QImage image(int iconNum) const;

    private:
        IconDecoder(const QString &name);

        class ICN *m_ICN;
    };
}



#endif // DATA_H
