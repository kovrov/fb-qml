#include <QFile>
#include <QtAlgorithms>
#include <QtEndian>

#include "datafs.h"
#include "resourceimageprovider.h"

#include <qdebug.h>



namespace
{
    enum { WIDTH = 256, HEIGHT = 224, BLOCKS_COUNT = 4, BLOCK_SIZE = (WIDTH * HEIGHT / BLOCKS_COUNT) };


    void decode_uncompressed_bitmap(QImage &image, const QByteArray &data)
    {
        // following code block ripped from REminiscence project
        for (int y = 0; y < HEIGHT; ++y) {
            auto line = image.scanLine(y);
            for (int x = 0; x < 64; ++x) {
                for (int i = 0; i < BLOCKS_COUNT; ++i) {
                    line[i + x * BLOCKS_COUNT] = (quint8)data.at(i * WIDTH * 56 + x + 64 * y);
                }
            }
        }
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
}



QPixmap MenuImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED (requestedSize)
    QImage image(WIDTH, HEIGHT, QImage::Format_Indexed8);

    // read palette
    {
        QFile file(DataFS::fileInfo(QString("%1.pal").arg(id)).filePath());
        file.open(QIODevice::ReadOnly);
        Q_ASSERT (file.size() == (3 * 256));

        QVector<QRgb> colors;
        const auto colors_count = file.size() / 3;
        colors.reserve(colors_count);
        for (auto i = 0; i < colors_count; ++i) {
            quint8 data[3];
            file.read((char*)data, 3);
            colors.append(QColor(data[0], data[1], data[2]).rgb());
        }
        image.setColorTable(colors);
    }

    // read bitmap indices
    {
        QFile file(DataFS::fileInfo(QString("%1.map").arg(id)).filePath());
        file.open(QIODevice::ReadOnly);
        Q_ASSERT (file.size() == WIDTH * HEIGHT);
        decode_uncompressed_bitmap(image, file.readAll());

    }

    if (size)
        *size = image.size();
    return QPixmap::fromImage(image);
}



QPixmap LevelImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED (requestedSize)
    QImage image(WIDTH, HEIGHT, QImage::Format_Indexed8);

    auto ids = id.split('/');
    int level = ids[0].toInt();
    int room = ids[1].toInt();
    qDebug() << "### level:" << level << "room:" << room;

    QVector<QRgb> level_palette;
    // read palette
    {
        QFile file(DataFS::fileInfo(QString("level%1.pal").arg(level)).filePath());
        file.open(QIODevice::ReadOnly);

        const auto colors_count = file.size() / 2;
        level_palette.reserve(colors_count);
        for (auto i = 0; i < colors_count; ++i) {
            quint8 data[2];
            file.read((char*)data, 2);
            const auto color_data = qFromBigEndian<quint16>(data);
            const auto t = (color_data == 0) ? 0 : 3;
            const auto r = (((color_data & 0x00F) << 2) | t) * 4;
            const auto g = (((color_data & 0x0F0) >> 2) | t) * 4;
            const auto b = (((color_data & 0xF00) >> 6) | t) * 4;
            level_palette.append(QColor(r, g, b).rgb());
        }
    }

    // read bitmap indices
    {
        QFile file(DataFS::fileInfo(QString("level%1.map").arg(level)).filePath());
        file.open(QIODevice::ReadOnly);
        Q_ASSERT (room < 0x40);
        file.seek(room * 6);
        quint8 raw_int32[4];
        file.read((char*)raw_int32, 4);
        const auto map_info = qFromLittleEndian<qint32>(raw_int32);
        Q_ASSERT (map_info != 0);
        const bool packed = (map_info & 1 << 31) ? false : true;
        const auto offset = map_info & 0x7FFFFFFF;
        file.seek(offset);

        qint8 pal_slot_1, pal_slot_2, pal_slot_3, pal_slot_4;
        file.read((char*)&pal_slot_1, 1);
        file.read((char*)&pal_slot_2, 1);
        file.read((char*)&pal_slot_3, 1);
        file.read((char*)&pal_slot_4, 1);
        if (level == 4 && room == 60) {  // workaround for wrong palette colors (fire)
            pal_slot_4 = 5;
        }

        QVector<QRgb> colors;
        colors.resize(256);
        const struct SectionSlotPair { int section, slot; } map[] = {
            // slot 0 is level background
            {0, pal_slot_1},
            // no idea what slots 1-3 are for..
            {1, pal_slot_2},
            {2, pal_slot_3},
            {3, pal_slot_4},
            // slot 4 is conrad palette, 5 is monster palette
            // slot 8 is level foreground (making other objects)
            {8, pal_slot_1},
            // no idea what slots 9-11 are for, or if there are more..
            {9, pal_slot_2},
            {10, pal_slot_3},
            {11, pal_slot_4}};
        for (size_t i=0; i < (sizeof(map)/sizeof(SectionSlotPair)); ++i) {
            for (auto j=0; j < 16; ++j)
                colors[map[i].section * 16 + j] = level_palette[map[i].slot * 16 + j];
        }
        image.setColorTable(colors);

        if (packed) {
            for (int i = 0; i < BLOCKS_COUNT; ++i) {
                quint8 raw_int16[2];
                file.read((char*)raw_int16, 2);
                auto size = qFromLittleEndian<quint16>(raw_int16);
                QByteArray encoded = unpack_block(file.read(size));
                Q_ASSERT (encoded.length() == WIDTH * 56);
                for (int y = 0; y < 56; ++y) {
                    auto line = image.scanLine(i*56 + y);
                    qCopy(encoded.begin() + y * WIDTH, encoded.begin() + y * WIDTH + WIDTH, line);
                }
            }
        }
        else {
            decode_uncompressed_bitmap(image, file.read(WIDTH * HEIGHT));
        }
    }

    if (size)
        *size = image.size();
    return QPixmap::fromImage(image);
}
