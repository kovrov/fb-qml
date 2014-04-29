#include <QFile>
#include <QtEndian>

#include "data.h"
#include "datafs.h" // FIXME
#include "imageproviders.h"

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
}



QPixmap MenuImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED (requestedSize)
    QImage image(WIDTH, HEIGHT, QImage::Format_Indexed8);

    // read palette
    {
        QFile file(DataFS::fileInfo(id+".pal").filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "MenuImageProvider: failed to open file" << file.fileName();
            return QPixmap();
        }
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
        QFile file(DataFS::fileInfo(id+".map").filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "MenuImageProvider: failed to open file" << file.fileName();
            return QPixmap();
        }
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

    auto ids = id.split('/');
    int level = ids[0].toInt();
    int room = ids[1].toInt();
    qDebug() << "### requestPixmap level:" << level << "room:" << room;

    auto data_level = FlashbackData::Level::load(level);
    QImage image = data_level->roomBitmap(room);
    if (size)
        *size = image.size();
    return QPixmap::fromImage(image);
}



QPixmap IconImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED (requestedSize)
    static QMap<QString, QSharedPointer<FlashbackData::IconDecoder> > _cache;

    auto ids = id.split('/');
    const QString scope(ids[0]);
    int iconNum = ids[1].toInt();

    auto decoder(_cache.value(scope));
    if (!decoder) {
        decoder = FlashbackData::IconDecoder::load(scope);
        _cache[scope] = decoder;
    }

    QImage image = decoder->image(iconNum);
    if (size)
        *size = image.size();

    return QPixmap::fromImage(image);
}


//
QPixmap SprImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED (requestedSize)

    auto ids = id.split('/');
    auto scope = ids[0];
    auto index = ids[1].toInt();
    auto decoder = FlashbackData::SpriteDecoder::load(scope);
    QImage image = decoder->image(index);
    if (size)
        *size = image.size();

    return QPixmap::fromImage(image);
}



QPixmap SpcImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED (requestedSize)
    QImage res;

    auto ids = id.split('/');
    auto scope = ids[0];
    auto index = ids[1].toInt();

    FlashbackData::SPC *spc (new FlashbackData::SPC (DataFS::fileInfo(scope + ".spc")));
    const auto &spc_data = spc->data()[index];
    auto data_level = FlashbackData::Level::load(0);
    int slot = data_level->rp(spc_data.slot);
    const auto bank = data_level->mbk(slot);
    foreach (const auto &frame, spc_data.frames) {
        auto pge_flags = 0xC;
        quint8 sprite_flags = frame.flags;
        if (pge_flags & 2)
            sprite_flags ^= 0x10; // 0b10000
        quint8 h = (((sprite_flags >> 0) & 3) + 1) * 8;
        quint8 w = (((sprite_flags >> 2) & 3) + 1) * 8;
        const int size = w * h / 2;
        auto src_4bit = bank.mid(frame.bankOffset, size);
        QByteArray bitmap;
        for (int i = 0; i < size; ++i) {
            bitmap.push_back(src_4bit[i] >> 4);
            bitmap.push_back(src_4bit[i] & 15);
        }

        QImage image(w, h, QImage::Format_Indexed8);
        //image.setColorTable(data_level->colorTable(27));
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                image.setPixel(x, y, bitmap.at(x + y * w));
            }
        }
        res = image;
        break;
    }

    if (size)
        *size = res.size();

    return QPixmap::fromImage(res);
}
