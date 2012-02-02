#include <QFile>
#include <QColor>
#include <QtEndian>
#include <QtAlgorithms>

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
            }
            else {
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
                }
                else {
                    auto num_bytes = 2;
                    auto offset = ctx.read_bits_from_src(8);
                    ctx.copy_bytes_from_window(num_bytes, offset);
                }
            }
            else {
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



namespace FlashbackData  // internal
{
    class CT
    {
    public:
        void load(int level)
        {
            QFile file(DataFS::fileInfo(QString("level%1.ct").arg(level)).filePath());
            file.open(QIODevice::ReadOnly);
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
            QFile file(DataFS::fileInfo(QString("level%1.map").arg(level)).filePath());
            file.open(QIODevice::ReadOnly);
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
                }
                else {
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
            QFile file(DataFS::fileInfo(QString("level%1.pal").arg(level)).filePath());
            file.open(QIODevice::ReadOnly);

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
}



FlashbackData::Level::Level()
  : m_CT (new CT),
    m_MAP (new MAP),
    m_PAL (new PAL)
{
}


FlashbackData::Level::~Level()
{
    delete m_CT;
    delete m_MAP;
    delete m_PAL;
}


void FlashbackData::Level::load(int level)
{
    m_CT->load(level);
    m_PAL->load(level);
    m_MAP->load(level);
}


const QByteArray &FlashbackData::Level::adjacentRooms()
{
    return m_CT->adjacentRooms;
}


QImage FlashbackData::Level::roomBitmap(int room)
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

    return image;
}
