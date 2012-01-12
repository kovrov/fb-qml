#include "polywidget.h"
#include "data.h"

#include <QTime>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <qdebug.h>



struct Primitive
{
    int x;
    int y;
    int num;
    int color;
};



QByteArray decode(const QByteArray &a)
{
    QByteArray res;
    auto idx = 0;
    while (idx < a.length()) {
        const quint8 e = a[idx];
        ++idx;
        for (auto i = 0; i < 8 && idx < a.length(); ++i)
            if ((e & 1 << i) == 0) {
                res += a[idx];
                ++idx;
            }
            else {
                // WTF?: const quint32 xxx = quint16(16 << 8) | char(241);
                const quint16 f = quint16(a[idx] << 8) | quint8(a[idx+1]);
                idx += 2;
                for (auto j = 0; j < (f >> 12) + 3; ++j) {
                    res += res[res.length() - (f & 0xFFF)];
                }
            }
    }
    return res;
}



class DPoly
{
public:
    DPoly()
      : m_playing (false),
        m_yield (0),
        m_clear (false),
        m_buf (NULL),
        m_scale (2),
        m_opcode (255),
        m_pos (0),
        m_primitives_clear (false)
    {
        m_cmd = decode(QByteArray::fromBase64(dat_cmd));
        m_pol = decode(QByteArray::fromBase64(dat_pol));
    }

    void start()
    {
        m_pos = 2;
        m_playing = true;
        setDefaultPalette();
    }

    void pause()
    {
        m_playing = !m_playing;
    }

private:
    quint8 read_uint8(const QByteArray &a, quint32 idx)
    {
        return a[idx];
    }

    qint8 read_int8(const QByteArray &a, quint32 idx)
    {
        return a[idx];
    }

    quint16 read_uint16(const QByteArray &a, quint32 idx)
    {
        return quint16(a[idx] << 8) | quint8(a[idx+1]);
    }

    qint16 read_int16(const QByteArray &a, quint32 idx)
    {
        return quint16(a[idx] << 8) | quint8(a[idx+1]);
    }

    quint8 readNextByte()
    {
        quint8 res = read_uint8(m_cmd, m_pos);
        m_pos += 1;
        return res;
    }

    quint16 readNextWord()
    {
        quint16 res = read_uint16(m_cmd, m_pos);
        m_pos += 2;
        return res;
    }

public:
    bool doTick()
    {
        if (!m_playing)
            return false;

        if (m_yield != 0) {
            m_yield -= 1;
            return false;
        }

        while (m_yield == 0) {
            auto a = readNextByte();
            if (a & 128) {
                m_pos = 2;
                setDefaultPalette();
            }
            else {
                switch (m_opcode = a >> 2, m_opcode) {
                case 0:
                case 5:
                case 9:
                    return true;// update needed
                case 1:
                    m_clear = bool(readNextByte());
                    clearScreen();
                    break;
                case 2:
                    m_yield = readNextByte() * 4;
                    break;
                case 3:{
                    auto a = readNextWord();
                    qint16 c = 0;
                    qint16 b = 0;
                    if (a & 32768) {
                        a &= 32767;
                        c = (qint16)readNextWord();
                        b = (qint16)readNextWord();
                    }
                    drawShape(a, c, b);
                } break;
                case 4: {
                    auto a = readNextByte();
                    auto c = readNextByte();
                    setPalette(a, c);
                } break;
                case 10: {
                    auto a = readNextWord();
                    qint16 b = 0;
                    qint16 c = 0;
                    if (a & 32768) {
                        a &= 32767;
                        c = (qint16)readNextWord();
                        b = (qint16)readNextWord();
                    }
                    auto e = 512 + readNextWord();
                    auto d = readNextByte();
                    auto f = readNextByte();
                    //drawShapeScale(a, c, b, e, d, f);
                } break;
                case 11: {
                    auto a = readNextWord();
                    qint16 b = 0;
                    qint16 c = 0;
                    if (a & 32768) {
                        a &= 32767;
                        c = (qint16)readNextWord();
                        b = (qint16)readNextWord();
                    }
                    auto e = 512;
                    a & 16384 && (a &= 16383, e += readNextWord());
                    auto d = readNextByte();
                    auto f = readNextByte();
                    auto g = readNextWord();
                    auto h = 90;
                    a & 8192 && (a &= 8191, h = readNextWord());
                    auto i = 180;
                    a & 4096 && (a &= 4095, i = readNextWord());
                    //drawShapeScaleRotate(a, c, b, e, d, f, g, h, i);
                } break;
                case 12:
                    m_yield = 10;
                    break;
                default:
                    m_playing = false;
                }
            }
        }
        return false;
    }

    void updateScreen(QPainter &painter, const QRect &rect)
    {
        QColor black(0, 0, 0);
        painter.fillRect(rect, black);
        foreach (const auto &primitive, m_primitives) {
            drawPrimitive(primitive, painter);
        }

        if (m_clear) {
            m_primitives.clear();
            m_primitives_clear = true;
        }
        else {
            m_primitives = m_savedPrimitives; // cow
        }

        m_yield = 5;
    }

private:
    void clearScreen()
    {
        if (m_clear) {
            m_primitives_clear = true;
        }
    }

    void drawShape(int offset, int shape_x, int shape_y)
    {
        const auto shape_offset_index = read_uint16(m_pol, 2);
        offset = read_uint16(m_pol, shape_offset_index + offset * 2);
        const auto shape_data_index = read_uint16(m_pol, 14);
        auto read_counter = shape_data_index + offset;
        auto shape_size = read_uint16(m_pol, read_counter);
        read_counter += 2;
        for (int i=0; i < shape_size; ++i) {
            auto const primitive_header = read_uint16(m_pol, read_counter);
            read_counter += 2;
            int x = 0, y = 0;
            if (primitive_header & (1 << 15)) {
                x = read_int16(m_pol, read_counter);
                read_counter += 2;
                y = read_int16(m_pol, read_counter);
                read_counter += 2;
            }
            bool unused = primitive_header & (1 << 14);
            auto color_index = read_uint8(m_pol, read_counter);
            read_counter++;
            if (!m_clear)
                color_index += 16;
            // queue primitive
            Primitive p = {shape_x + x, shape_y + y, primitive_header & 0x3FFF, color_index};
            m_primitives.append(p);
        }
        if (m_clear) {
            m_savedPrimitives = m_primitives; // cow
        }
    }

    void drawShapeScale()
    {
    }

    void drawShapeScaleRotate()
    {
    }

    void setDefaultPalette()
    {
        for (auto i = 0; i < 16; i++) {
            m_palette[i] = m_palette[16 + i] = QColor(i, i, i);
        }
    }

    void setPalette(int a, int c)
    {
        auto cursor = read_uint16(m_pol, 6) + a * 32;
        const auto off = (c == 0) ? 16 : 0;
        for (size_t i = 0; i < 16; i++) {
            const auto color = read_uint16(m_pol, cursor); cursor += 2;
            const quint8 t = (color == 0) ? 0 : 3;
            const int r = (((color & 0xF00) >> 6) | t) * 3;
            const int g = (((color & 0x0F0) >> 2) | t) * 3;
            const int b = (((color & 0x00F) << 2) | t) * 3;
            m_palette[off+i] = QColor(r, g, b);
        }
    }

    void drawPrimitive(const Primitive &primitive, QPainter &painter)
    {
        const auto vertices_offset_index = read_uint16(m_pol, 10);
        const auto num_off = read_uint16(m_pol, vertices_offset_index + primitive.num * 2);
        const auto vertices_data_index = read_uint16(m_pol, 18);
        auto cursor = vertices_data_index + num_off;
        auto num_vertices = read_uint8(m_pol, cursor); cursor++;
        int x = primitive.x + (read_int16(m_pol, cursor)); cursor += 2;
        int y = primitive.y + (read_int16(m_pol, cursor)); cursor += 2;
        painter.setBrush(m_palette[primitive.color]);
        painter.setPen(m_palette[primitive.color]);
        painter.save();
        painter.scale(m_scale, m_scale);
        if (num_vertices & 128) {
            painter.translate(x, y);
            auto center_x = (read_int16(m_pol, cursor)); cursor += 2;
            auto center_y = (read_int16(m_pol, cursor)); cursor += 2;
            painter.drawEllipse(QPointF(0,0), center_x, center_y);
        }
        else if (num_vertices == 0) {
            painter.fillRect(QRectF(x, y, m_scale, m_scale), m_palette[primitive.color]);
        }
        else {
            QPainterPath path(QPointF(x, y));
            for (auto i = 0; i < num_vertices; ++i) {
                auto g = (read_int8(m_pol, cursor)); cursor++;
                auto h = (read_int8(m_pol, cursor)); cursor++;
                x += g;
                y += h;
                path.lineTo(x, y);
            }
            path.closeSubpath();
            if (num_vertices <= 2) {
                painter.strokePath(path, m_palette[primitive.color]);
            }
            else {
                painter.fillPath(path, m_palette[primitive.color]);
            }
        }
        painter.restore();
    }

    bool m_playing;
    int m_yield;
    bool m_clear;
    void *m_buf;
    QColor m_palette[32];
    int m_scale;
    int m_opcode;
    QList<Primitive> m_primitives;
    QList<Primitive> m_savedPrimitives;
    QByteArray m_cmd;
    QByteArray m_pol;
    int m_pos;
    bool m_primitives_clear;
};



PolyWidget::PolyWidget(QWidget *parent)
  : QWidget (parent),
    m_poly (new DPoly)
{
    m_poly->start();
    startTimer(20);
}


PolyWidget::~PolyWidget()
{
    delete m_poly;
}


void PolyWidget::timerEvent(QTimerEvent *)
{
    if (m_poly->doTick())
        update();
}


void PolyWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    m_poly->updateScreen(painter, event->rect());
}
