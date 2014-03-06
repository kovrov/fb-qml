#include <QFile>
#include <QtEndian>

#include "data.h"
#include "datafs.h" // FIXME
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
