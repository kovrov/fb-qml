#include <QFile>
#include <QColor>
#include <QtEndian>
#include <QtAlgorithms>
#include <QWeakPointer>

#include "data.h"
#include "datafs.h"

#include <qdebug.h>



namespace  // internal
{
    enum { WIDTH = 256, HEIGHT = 224, BLOCKS_COUNT = 4, BLOCK_SIZE = (WIDTH * HEIGHT / BLOCKS_COUNT) };

    QByteArray decode_uncompressed_bitmap(const QByteArray &data)
    {
        QByteArray res;
        // following code block ripped from REminiscence project
        for (int y = 0; y < 224; ++y) {
            for (int x = 0; x < 64; ++x) {
                for (int i = 0; i < 4; ++i) {
                    res[y*224 + i + x * 4] = (quint8)data.at(i * 256 * 56 + x + 64 * y);
                }
            }
        }
        return res;
    }

    QByteArray unpack_block(const QByteArray &data)
    {
        const quint8 *src = (const quint8 *)data.data();
        const quint8 *end = src + data.length();
        QByteArray res;
        res.resize(WIDTH * 56);
        quint8 *dst = (quint8 *)res.data();
        while (src < end) {
            qint16 code = (qint8)*src++;
            if (code < 0) {
                const int len = 1 - code;
                memset(dst, *src++, len);
                dst += len;
            } else {
                ++code;
                memcpy(dst, src, code);
                src += code;
                dst += code;
            }
        }
        Q_ASSERT (dst - (quint8 *)res.data() == WIDTH * 56);
        return res;
    }

    QByteArray delphine_unpack(const QByteArray &data)
    {
        struct
        {
            QByteArray::const_iterator src;
            QByteArray::iterator dst;
            uint chunk;
            uint crc;

            uint rcr(uint input_carry)
            {
                uint output_carry = (chunk & 1);
                chunk >>= 1;
                if (input_carry) {
                    chunk |= (1 << 31);
                }
                return output_carry;
            }

            uint read_bit_from_src()
            {
                auto carry = rcr(0);
                if (chunk == 0) {
                    chunk = qFromBigEndian<uint>((const uchar*)&(*src));  src -= 4;
                    crc ^= chunk;
                    carry = rcr(1);
                }
                return carry;
            }

            uint read_bits_from_src(uint bit_count)
            {
                Q_ASSERT (bit_count <= sizeof(uint) * 8);
                uint c = 0;
                while (bit_count--) {
                    c <<= 1;
                    c |= read_bit_from_src();
                }
                return c;
            }

            void copy_bytes_from_src(uint num_bytes)
            {
                while (num_bytes--) {
                    *dst = read_bits_from_src(8);
                    --dst;
                }
            }

            void copy_bytes_from_window(uint num_bytes, uint offset)
            {
                while (num_bytes--) {
                    *dst = *(dst + offset);
                    --dst;
                }
            }
        } ctx;

        QByteArray res;
        ctx.src = data.constEnd() - 4;
        auto datasize = qFromBigEndian<uint>((const uchar*)&(*ctx.src));  ctx.src -= 4;
        res.resize(datasize);
        ctx.dst = res.end() - 1;
        ctx.crc = qFromBigEndian<uint>((const uchar*)&(*ctx.src));  ctx.src -= 4;
        ctx.chunk = qFromBigEndian<uint>((const uchar*)&(*ctx.src));  ctx.src -= 4;
        ctx.crc ^= ctx.chunk;

        while (ctx.dst >= res.begin()) {
            if (0 == ctx.read_bit_from_src()) {
                if (0 == ctx.read_bit_from_src()) {
                    auto num_bytes = 1 + ctx.read_bits_from_src(3);
                    ctx.copy_bytes_from_src(num_bytes);
                } else {
                    auto num_bytes = 2;
                    auto offset = ctx.read_bits_from_src(8);
                    ctx.copy_bytes_from_window(num_bytes, offset);
                }
            } else {
                auto c = ctx.read_bits_from_src(2);
                switch (c) {
                case 3: {  // b11
                    auto num_bytes = 9 + ctx.read_bits_from_src(8);
                    ctx.copy_bytes_from_src(num_bytes);
                  } break;
                case 2: {  // b10
                    auto num_bytes = 1 + ctx.read_bits_from_src(8);
                    auto offset = ctx.read_bits_from_src(12);
                    ctx.copy_bytes_from_window(num_bytes, offset);
                  } break;
                default: // b00, b01
                    auto num_bytes = c + 3;
                    auto offset = ctx.read_bits_from_src(c + 9);
                    ctx.copy_bytes_from_window(num_bytes, offset);
                }
            }
        }
        Q_ASSERT (ctx.crc == 0);
        return res;
    }
}



class LittleEndianStream
{
public:
    static LittleEndianStream fromFileInfo(const QFileInfo &fi)
    {
        QFile file(fi.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "LittleEndianStream: failed to open file" << file.fileName();
            return LittleEndianStream(QByteArray());
        }
        return LittleEndianStream(file.readAll());
    }
    void seek(int pos) { _pos = pos; }
    int pos() const { return _pos; }

    template<typename T>
    T next()
    {
        if (sizeof(T) == 1) {
            T res = (T)_data[_pos];
            _pos += 1;
            return res;
        } else if (sizeof(T) == 2) {
            auto data = (const uchar*)_data.constData();
            T res = qFromLittleEndian<T>(data + _pos);
            _pos += 2;
            return res;
        }
    }

private:
    LittleEndianStream(const QByteArray &data) : _data (data), _pos (0) {}

    const QByteArray _data;
    int _pos;
};



namespace FlashbackData  // internal
{
    static const struct {
        const QString name, name2;
        quint16 cutscene_id;
        quint8 spl;
    } _levels[] = {
        { "level1", "level1", 0x00, 1 },
        { "level2", "level2", 0x2F, 1 },
        { "level3", "level3", 0xFFFF, 3 },
        { "level4", "level4_1", 0x34, 3 },
        { "level4", "level4_2", 0x39, 3 },
        { "level5", "level5_1", 0x35, 4 },
        { "level5", "level5_2", 0xFFFF, 4 }};

    class CT
    {
    public:
        void load(int level)
        {
            QFile file(DataFS::fileInfo(_levels[level].name+".ct").filePath());
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "FlashbackData::CT: failed to open file" << file.fileName();
                return;
            }
            QByteArray data = delphine_unpack(file.readAll());
            adjacentRooms = data.left(256);
            collisionData = data.right(256);
        }
        QByteArray adjacentRooms;
        QByteArray collisionData;
    };


    class MAP
    {
    public:
        void load(int level)
        {
            QFile file(DataFS::fileInfo(_levels[level].name+".map").filePath());
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "FlashbackData::MAP: failed to open file" << file.fileName();
                return;
            }
            for (int room=0; room < 64; ++room) {
                file.seek(room * 6);
                quint8 raw_int32[4];
                file.read((char*)raw_int32, 4);
                const auto map_info = qFromLittleEndian<qint32>(raw_int32);
                if (map_info == 0)
                    continue;

                const bool packed = (map_info & 1 << 31) ? false : true;
                const auto offset = map_info & 0x7FFFFFFF;
                file.seek(offset);
                RawBitmap bitmap;
                file.read((char*)&bitmap.pal_slot_1, 1);
                file.read((char*)&bitmap.pal_slot_2, 1);
                file.read((char*)&bitmap.pal_slot_3, 1);
                file.read((char*)&bitmap.pal_slot_4, 1);
                if (level == 4 && room == 60) {  // workaround for wrong palette colors (fire)
                    bitmap.pal_slot_4 = 5;
                }
                if (packed) {
                    for (int i = 0; i < 4; ++i) {
                        quint8 raw_int16[2];
                        file.read((char*)raw_int16, 2);
                        auto size = qFromLittleEndian<quint16>(raw_int16);
                        bitmap.data.append(unpack_block(file.read(size)));
                    }
                } else {
                    Q_ASSERT (false); // breakpoint =)
                    bitmap.data = decode_uncompressed_bitmap(file.read(256 * 224));
                }
                rawBitmaps[room] = bitmap;
            }
        }
        struct RawBitmap { QByteArray data; qint8 pal_slot_1, pal_slot_2, pal_slot_3, pal_slot_4; };
        QMap<int, RawBitmap> rawBitmaps;
    };


    class PAL
    {
    public:
        void load(int level)
        {
            QFile file(DataFS::fileInfo(_levels[level].name+".pal").filePath());
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "FlashbackData::PAL: failed to open file" << file.fileName();
                return;
            }
            const auto colors_count = file.size() / 2;
            palette.reserve(colors_count);
            for (auto i = 0; i < colors_count; ++i) {
                quint8 data[2];
                file.read((char*)data, 2);
                const auto color_data = qFromBigEndian<quint16>(data);
                const auto t = (color_data == 0) ? 0 : 3;
                const auto r = (((color_data & 0x00F) << 2) | t) * 4;
                const auto g = (((color_data & 0x0F0) >> 2) | t) * 4;
                const auto b = (((color_data & 0xF00) >> 6) | t) * 4;
                palette.append(QColor(r, g, b).rgb());
            }
        }
        QVector<QRgb> palette;
    };


    class PGE
    {
    public:
        struct Init
        {
            /*

*/
            quint16 type;  // obj_type, used in getAniData()
            qint16 x, y;
            quint16 obj_node_number;  // _objectNodesMap[] OBJ
            quint16 life;
            qint16 counter_values[4];
            quint8 object_type; // enum 0 .. 12
            quint8 room;  // CT.adjacentRooms //init_room
            bool animated;  //room_location
            quint8 init_flags;
            quint8 colliding_icon; //col_findCurrentCollidingObject  //colliding_icon_num
            quint8 icon;  //icon_num
            quint8 object_id; // find object in inventory
            quint8 skill;
            quint8 mirror_x; // somehow affecting flags
            quint8 flags; // 1:FacingDir,
            quint8 unk1C; // collidable, collision_data_len
            quint16 text_num;
        };
        QVector<Init> init;

        void load(int level)
        {
            auto data = LittleEndianStream::fromFileInfo(DataFS::fileInfo(_levels[level].name2+".pge").filePath());
            uint pge_num = data.next<quint16>();
            qDebug() << "pge_num:" << pge_num;
            init.resize(pge_num);
            for (unsigned int i = 0; i < pge_num; ++i) {
                Init *pge = &init[i];
                pge->type = data.next<quint16>();
                pge->x = data.next<quint16>();
                pge->y = data.next<quint16>();
                pge->obj_node_number = data.next<quint16>();
                pge->life = data.next<quint16>();
                for (int lc = 0; lc < 4; ++lc) {
                    pge->counter_values[lc] = data.next<quint16>();
                }
                pge->object_type = data.next<quint8>();
                pge->room = data.next<quint8>();
                pge->animated = data.next<quint8>();
                pge->init_flags = data.next<quint8>();
                pge->colliding_icon = data.next<quint8>();
                pge->icon = data.next<quint8>();
                pge->object_id = data.next<quint8>();
                pge->skill = data.next<quint8>();
                pge->mirror_x = data.next<quint8>();
                pge->flags = data.next<quint8>();
                pge->unk1C = data.next<quint8>();
                data.next<quint8>();
                pge->text_num = data.next<quint16>();
            }
        }
    };


    class ICN
    {
    public:
        ICN(const QFileInfo &fi)
        {
            QFile f(fi.filePath());
            f.open(QIODevice::ReadOnly);
            QDataStream stream(&f);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint16 first;
            stream >> first;
            for (quint16 offset = first; f.pos() <= first; stream >> offset) {
                qint64 pos = f.pos();
                f.seek(offset + 2);
                QByteArray buffer(16 * 16 / 2, Qt::Uninitialized);
                stream.readRawData(buffer.data(), buffer.size());
                m_icons << buffer;
                f.seek(pos);
            }
        }
        const QByteArray & bitmap(int i) { return m_icons.at(i); }

    private:
        QList<QByteArray> m_icons;
    };
}



FlashbackData::Level::Level(int level)
  : m_CT (new CT),
    m_MAP (new MAP),
    m_PAL (new PAL),
    m_PGE (new PGE)
{
    m_CT->load(level);
    m_PAL->load(level);
    m_MAP->load(level);
    m_PGE->load(level);
}


FlashbackData::Level::~Level()
{
    delete m_CT;
    delete m_MAP;
    delete m_PAL;
    delete m_PGE;
}


QSharedPointer<FlashbackData::Level> FlashbackData::Level::load(int level)
{
    static QMap<int, QWeakPointer<FlashbackData::Level> > _cache;
    if (_cache.contains(level) && !_cache[level].isNull()) {
        return _cache[level].toStrongRef();
    }
    QSharedPointer<FlashbackData::Level> self(new Level(level));
    _cache[level] = self;
    return self;
}


int FlashbackData::Level::initialRoom() const
{
    qDebug() << "### Level::initialRoom:" << m_PGE->init[0].room;
    return m_PGE->init[0].room;
}


const QByteArray &FlashbackData::Level::adjacentRooms() const
{
    return m_CT->adjacentRooms;
}


QImage FlashbackData::Level::roomBitmap(int room) const
{
    enum { WIDTH = 256, HEIGHT = 224 };
    QImage image(WIDTH, HEIGHT, QImage::Format_Indexed8);

    if (!m_MAP->rawBitmaps.contains(room)) {
        qWarning() << "room bitmap not found:" << room;
        return image;
    }

    const auto &bitmap = m_MAP->rawBitmaps[room];
    QVector<QRgb> colors;
    colors.resize(256);
    const struct SectionSlotPair { int section, slot; } map[] = {
        // slot 0 is level background
        {0, bitmap.pal_slot_1},
        // no idea what slots 1-3 are for..
        {1, bitmap.pal_slot_2},
        {2, bitmap.pal_slot_3},
        {3, bitmap.pal_slot_4},
        // slot 4 is conrad palette, 5 is monster palette
        // slot 8 is level foreground (making other objects)
        {8, bitmap.pal_slot_1},
        // no idea what slots 9-11 are for, or if there are more..
        {9, bitmap.pal_slot_2},
        {10, bitmap.pal_slot_3},
        {11, bitmap.pal_slot_4}};
    for (size_t i=0; i < (sizeof(map)/sizeof(SectionSlotPair)); ++i) {
        for (auto j=0; j < 16; ++j)
            colors[map[i].section * 16 + j] = m_PAL->palette[map[i].slot * 16 + j];
    }
    image.setColorTable(colors);

    for (int y = 0; y < HEIGHT; ++y) {
        auto line = image.scanLine(y);
        qCopy(bitmap.data.constBegin() + y * WIDTH,
              bitmap.data.constBegin() + y * WIDTH + WIDTH, line);
    }

    foreach (const auto &pge, m_PGE->init) {
        if (pge.room != room)
            continue;
//        qDebug() << "[pge]"
//                 << "type:" << pge.type
//                 << "pos:" << pge.x << "," << pge.y
//                 << "obj_node:" << pge.obj_node_number
//                 << "life:" << pge.life
////                         << "counter_values:" << pge.counter_values[4]
//                 << "obj_t:" << pge.object_type
//                 << "room:" << pge.room
//                 << "location:" << pge.animated
//                 << "i_flags:" << pge.init_flags
//                 << "c_icon:" << pge.colliding_icon
//                 << "icon:" << pge.icon
//                 << "obj_id:" << pge.object_id
//                 << "skill:" << pge.skill
//                 << "mirror:" << pge.mirror_x
//                 << "flags:" << pge.flags
//                 << "_:" << pge.unk1C
//                 << "text:" << pge.text_num;
    }

    return image;
}



FlashbackData::IconDecoder::IconDecoder(const QString &scope) :
    m_ICN (new ICN (DataFS::fileInfo(scope + ".icn")))
{
}


FlashbackData::IconDecoder::~IconDecoder()
{
    delete m_ICN;
}


QSharedPointer<FlashbackData::IconDecoder> FlashbackData::IconDecoder::load(const QString &scope)
{
    static QMap<QString, QWeakPointer<FlashbackData::IconDecoder> > _cache;
    auto self(_cache.value(scope).toStrongRef());
    if (!self) {
        self.reset(new IconDecoder(scope));
        _cache[scope] = self;
    }
    return self;
}


QImage FlashbackData::IconDecoder::image(int iconNum) const
{
    enum { WIDTH = 16, HEIGHT = 16 };
    // 0xA0
    static const QVector<QRgb> item_colors = QVector<QRgb>()
            << QColor(0x00, 0x00, 0x00, 0).rgba()
            << QColor(0xCF, 0xCF, 0xCF).rgb()
            << QColor(0x4F, 0x2F, 0x0F).rgb()
            << QColor(0x4F, 0x4F, 0xAF).rgb()
            << QColor(0xCF, 0x6F, 0x6F).rgb()
            << QColor(0x8F, 0x8F, 0x8F).rgb()
            << QColor(0x6F, 0x6F, 0xCF).rgb()
            << QColor(0x6F, 0x4F, 0x2F).rgb()
            << QColor(0xEF, 0x0F, 0x4F).rgb()
            << QColor(0x8F, 0x0F, 0x4F).rgb()
            << QColor(0x4F, 0x4F, 0x4F).rgb()
            << QColor(0x0F, 0xEF, 0x0F).rgb()
            << QColor(0x0F, 0x6F, 0x0F).rgb()
            << QColor(0xEF, 0xEF, 0x0F).rgb()
            << QColor(0xAF, 0x2F, 0xEF).rgb()
            << QColor(0x00, 0x00, 0x00).rgb();
    // 0xF0
    static const QVector<QRgb> inventory_colors = QVector<QRgb>()
            << QColor(0x00, 0x00, 0x00, 0).rgba()
            << QColor(0x1F, 0x17, 0x2B).rgb()
            << QColor(0x2B, 0x1F, 0x37).rgb()
            << QColor(0x37, 0x2B, 0x47).rgb()
            << QColor(0x43, 0x37, 0x53).rgb()
            << QColor(0x4F, 0x43, 0x63).rgb()
            << QColor(0x5F, 0x53, 0x6F).rgb()
            << QColor(0x6F, 0x63, 0x7F).rgb()
            << QColor(0x7F, 0x73, 0x8B).rgb()
            << QColor(0x8F, 0x87, 0x9B).rgb()
            << QColor(0x9F, 0x97, 0xA7).rgb()
            << QColor(0xAF, 0xA7, 0xB3).rgb()
            << QColor(0xBF, 0xBB, 0xBF).rgb()
            << QColor(0xCF, 0xCF, 0xCF).rgb()
            << QColor(0x00, 0x33, 0x00).rgb()
            << QColor(0x17, 0x0F, 0x1F).rgb();

    QImage image(WIDTH, HEIGHT, QImage::Format_Indexed8);
    image.setColorTable(iconNum < 31 || iconNum > 75 ? item_colors : inventory_colors);

    const QByteArray &bitmap = m_ICN->bitmap(iconNum);
    for (int i = 0; i < HEIGHT * WIDTH / 2; ++i) {
        quint8 n = static_cast<quint8>(bitmap[i]);
        image.setPixel(i * 2 % 16, (i * 2) / 16, n >> 4);
        image.setPixel((i * 2 + 1) % 16, (i * 2 + 1) / 16, n & 0xF);
    }
    return image;
}
