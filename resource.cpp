#include <QColor>

#include "resource.h"
#include "data.h"



Stream::Stream(const QByteArray &data) : _data (data), _pos (0) {}


Stream Stream::fromBase64(const QByteArray &base64)
{
    return Stream(decode(QByteArray::fromBase64(base64)));
}


QByteArray Stream::decode(const QByteArray &array)
{
    QByteArray res;
    auto idx = 0;
    while (idx < array.length()) {
        const quint8 e = array[idx];
        ++idx;
        for (auto i = 0; i < 8 && idx < array.length(); ++i)
            if ((e & 1 << i) == 0) {
                res += array[idx];
                ++idx;
            }
            else {
                const quint16 f = quint16(array[idx] << 8) | quint8(array[idx+1]);
                idx += 2;
                for (auto j = 0; j < ((f >> 12) + 3); ++j) {
                    res += res[res.length() - (f & 0xFFF)];
                }
            }
    }
    return res;
}





POL::POL(Stream stream)
{
    parse_poly(stream);
}


void POL::parse_poly(Stream &stream)
{
    stream.seek(2);
    const auto shape_offset_index = stream.next<quint16>();

    stream.seek(6);
    const auto palette_index = stream.next<quint16>();

    stream.seek(14);
    const auto shape_data_index = stream.next<quint16>();

    stream.seek(10);
    const auto vertices_offset_index = stream.next<quint16>();

    stream.seek(18);
    const auto vertices_data_index = stream.next<quint16>();

    QVector<quint16> shape_offset_table;
    const size_t shape_offset_table_length = (palette_index - shape_offset_index) / sizeof(quint16);
    shape_offset_table.reserve(shape_offset_table_length);
    stream.seek(shape_offset_index);
    for (auto i=0; i < shape_offset_table_length; ++i)
        shape_offset_table.append(stream.next<quint16>());

    const size_t palettes_length = (shape_data_index - palette_index) / (sizeof(quint16) * 16);
    m_palettes.reserve(palettes_length);
    stream.seek(palette_index);
    for (auto i=0; i < palettes_length; ++i) {
        Palete palette;
        for (auto &color : palette.colors) {
            auto color_data = stream.next<quint16>();
            const quint8 t = (color_data == 0) ? 0 : 3;
            const int r = (((color_data & 0xF00) >> 6) | t) * 3;
            const int g = (((color_data & 0x0F0) >> 2) | t) * 3;
            const int b = (((color_data & 0x00F) << 2) | t) * 3;
            color.setRgb(r, g, b);
        }
        m_palettes.append(palette);
    }

    QVector<quint16> vertices_offset_table;
    const size_t vertices_offset_table_length = (vertices_data_index - vertices_offset_index) / sizeof(quint16);
    vertices_offset_table.reserve(vertices_offset_table_length);
    stream.seek(vertices_offset_index);
    for (auto i=0; i < vertices_offset_table_length; ++i)
        vertices_offset_table.append(stream.next<quint16>());

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

            // queue primitive
            auto pos = stream.pos();


            /// FIXME: store paths separately!

            const auto vertices_offset = vertices_offset_table[primitive_info & 0x3FFF];  // 00111111_11111111
            stream.seek(vertices_data_index + vertices_offset);
            const auto num_vertices = stream.next<quint8>();
            int vert_x = stream.next<qint16>();
            int vert_y = stream.next<qint16>();

            if (num_vertices & (1 << 7)) {
                const auto center_x = stream.next<qint16>();
                const auto center_y = stream.next<qint16>();
                poly_data.path.addEllipse(QPointF(vert_x, vert_y), center_x, center_y);
            }
            else if (num_vertices == 0) {
                poly_data.path.addRect(QRectF(vert_x, vert_y, 1, 1));
            }
            else {
                poly_data.path.moveTo(vert_x, vert_y);
                for (auto i = 0; i < num_vertices; ++i) {
                    vert_x += stream.next<qint8>();
                    vert_y += stream.next<qint8>();
                    poly_data.path.lineTo(vert_x, vert_y);
                }
            }
            stream.seek(pos);
            group.append(poly_data);
        }
        m_primitive_groups.append(group);
    }
}
