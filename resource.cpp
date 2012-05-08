#include <QFile>
#include <QFileInfo>
#include <QColor>
#include <qdebug.h>

#include "resource.h"
#include "data.h"



BigEndianStream::BigEndianStream(const QByteArray &data) : _data (data), _pos (0) {}


BigEndianStream BigEndianStream::fromFileInfo(const QFileInfo &fi)
{
    QFile file(fi.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "BigEndianStream: failed to open file" << file.fileName();
        return BigEndianStream(QByteArray());
    }
    return BigEndianStream(file.readAll());
}





QVector<Palete> extract_palettes(BigEndianStream &stream)
{
    stream.seek(6);
    const auto palette_index = stream.next<quint16>();
    stream.seek(14);
    const auto shape_data_index = stream.next<quint16>();

    const size_t palettes_length = (shape_data_index - palette_index) / (sizeof(quint16) * 16);
    QVector<Palete> palettes;
    palettes.reserve(palettes_length);
    stream.seek(palette_index);
    for (size_t i=0; i < palettes_length; ++i) {
        Palete palette;
        for (size_t j=0; j < (sizeof(palette.colors)/sizeof(QColor)); ++j) {
            auto color_data = stream.next<quint16>();
            const quint8 t = (color_data == 0) ? 0 : 3;
            const int r = (((color_data & 0xF00) >> 6) | t) * 4;
            const int g = (((color_data & 0x0F0) >> 2) | t) * 4;
            const int b = (((color_data & 0x00F) << 2) | t) * 4;
            palette.colors[j].setRgb(r, g, b);
        }
        palettes.append(palette);
    }
    return palettes;
}



QList< QList<Primitive> > extract_primitive_groups(BigEndianStream &stream)
{
    stream.seek(2);
    const auto shape_offset_index = stream.next<quint16>();

    stream.seek(6);
    const auto palette_index = stream.next<quint16>();

    stream.seek(14);
    const auto shape_data_index = stream.next<quint16>();

    QVector<quint16> shape_offset_table;
    const size_t shape_offset_table_length = (palette_index - shape_offset_index) / sizeof(quint16);
    shape_offset_table.reserve(shape_offset_table_length);
    stream.seek(shape_offset_index);
    for (size_t i=0; i < shape_offset_table_length; ++i)
        shape_offset_table.append(stream.next<quint16>());

    QList< QList<Primitive> > primitive_groups;
    foreach (const auto &shape_offset, shape_offset_table) {
        QList<Primitive> group;
        stream.seek(shape_data_index + shape_offset);
        const auto primitive_count = stream.next<quint16>();
        for (int i=0; i < primitive_count; ++i) {
            const auto primitive_info = stream.next<quint16>();
            int x = 0, y = 0;
            if (primitive_info & (1 << 15)) {
                x = stream.next<qint16>();
                y = stream.next<qint16>();
            }

            Primitive poly_data;
            poly_data.has_alpha = primitive_info & (1 << 14);
            poly_data.color_index = stream.next<quint8>();
            poly_data.transform = QTransform::fromTranslate(x, y);
            poly_data.path_index = primitive_info & 0x3FFF;  // 00111111_11111111
            group.append(poly_data);
        }
        primitive_groups.append(group);
    }

    return primitive_groups;
}



QList<QPainterPath> extract_paths(BigEndianStream &stream)
{
    stream.seek(10);
    const auto vertices_offset_index = stream.next<quint16>();

    stream.seek(18);
    const auto vertices_data_index = stream.next<quint16>();

    QVector<quint16> vertices_offset_table;
    const size_t vertices_offset_table_length = (vertices_data_index - vertices_offset_index) / sizeof(quint16);
    vertices_offset_table.reserve(vertices_offset_table_length);
    stream.seek(vertices_offset_index);
    for (size_t i=0; i < vertices_offset_table_length; ++i)
        vertices_offset_table.append(stream.next<quint16>());


    QList<QPainterPath> paths;
    foreach (const auto &vertices_offset, vertices_offset_table) {

        stream.seek(vertices_data_index + vertices_offset);
        const auto num_vertices = stream.next<quint8>();
        int vert_x = stream.next<qint16>();
        int vert_y = stream.next<qint16>();

        QPainterPath path;
        if (num_vertices & (1 << 7)) {
            const auto center_x = stream.next<qint16>();
            const auto center_y = stream.next<qint16>();
            path.addEllipse(QPointF(vert_x, vert_y), center_x, center_y);
        }
        else if (num_vertices == 0) {
            path.addRect(QRectF(vert_x, vert_y, 1, 1));
        }
        else {
            path.moveTo(vert_x, vert_y);
            for (auto i = 0; i < num_vertices; ++i) {
                vert_x += stream.next<qint8>();
                vert_y += stream.next<qint8>();
                path.lineTo(vert_x, vert_y);
            }
        }

        paths.append(path);
    }

    return paths;
}



POL::POL(BigEndianStream stream)
{
    m_palettes = extract_palettes(stream);
    m_paths = extract_paths(stream);
    m_primitives = extract_primitive_groups(stream);
}
