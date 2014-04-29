#ifndef DATA_H
#define DATA_H

#include <QtCore/QByteArray>
#include <QtCore/QSharedPointer>
#include <QtGui/QImage>
#include <QtCore/QHash>

class QFileInfo;


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
    SPM (amiga) SPM -delphine_unpack-> SPR
    SPR {offset,x,y,flags{w,h}}
    TXT
    VCE
    */

    class OFF
    {
    public:
        OFF(const QFileInfo &fi);
        const QHash<quint16, quint32> & hash() const { return m_off; }
    private:
        // [pge.anim_number] -> spr(+off)
        QHash<quint16, quint32> m_off;
    };


    class SPR
    {
    public:
        SPR(const QFileInfo &fi);
        const QByteArray & bytes() const { return m_spr;}
    private:
        QByteArray m_spr;
    };


    class SPC
    {
        struct FrameData {
            int bankOffset;
            int x;
            int y;
            int flags;
        };

        struct Data {
            int slot;
            int x;
            int y;
            QVector<FrameData> frames;
        };

    public:
        SPC(const QFileInfo &fi);
        const QList<Data> & data() const { return m_spc;}
    private:

        QList<SPC::Data> m_spc; // count
        //QByteArray m_spc;
    };


    class RP
    {
    public:
        RP(const QFileInfo &fi);
        int at(int i) const { return m_rp[i];}
    private:
        QByteArray m_rp;
    };


    class MBK
    {
        struct Header {
            size_t begin_offset;
            size_t end_offset;
            size_t size;
            bool compressed;
            QByteArray rawBuffer;
        };
    public:
        MBK(const QFileInfo &fi);
        QByteArray at(int i) const;
    private:
        QHash<int, Header> m_data;
    };


    class Level
    {
    public:

        static QSharedPointer<Level> load(int level);
        ~Level();
        int initialRoom() const;
        const QByteArray &adjacentRooms() const;
        QImage roomBitmap(int room) const;
        int rp(int index) const;
        QByteArray mbk(int slot) const;

    private:
        Level(int level);

        class CT *m_CT;    // adjacent rooms, collision
        class MAP *m_MAP;  // bitmap, parette table
        class PAL *m_PAL;  // palettes
        class PGE *m_PGE;  // objects
        class MBK *m_MBK;  // animated sprites data
        class RP *m_RP;  // sprite slots table
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


    class SpriteDecoder
    {
    public:
        static QSharedPointer<SpriteDecoder> load(const QString &scope);
        ~SpriteDecoder();
        QImage image(int number) const;

    private:
        SpriteDecoder(const QString &name);

        //class SPC *m_SPC;
        class SPR *m_SPR;
        class OFF *m_OFF;
    };
}



#endif // DATA_H
