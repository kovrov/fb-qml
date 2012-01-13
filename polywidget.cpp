#include "polywidget.h"
#include "data.h"

#include <QTime>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <qdebug.h>



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



class Stream
{
public:
    static
    Stream fromBase64(const QByteArray &base64)
    {
        return Stream(decode(QByteArray::fromBase64(base64)));
    }

    void seek(int pos) { _pos = pos; }

    template<typename T>
    T next()
    {
        if (sizeof(T) == 1) {
            T res = (T)_data[_pos];
            _pos += 1;
            return res;
        }
        else if (sizeof(T) == 2) {
            T res = (T)(quint16(_data[_pos] << 8) | quint8(_data[_pos+1]));
            _pos += 2;
            return res;
        }
    }

private:
    Stream(const QByteArray &data) : _data (data), _pos (0) {}
    const QByteArray _data;
    int _pos;
};



struct Primitive
{
    int x;
    int y;
    int num;
    int color;
};



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
        m_cmd (Stream::fromBase64(dat_cmd)),
        m_pol (Stream::fromBase64(dat_pol)),
        m_primitives_clear (false)
    {
    }

    void start()
    {
        m_cmd.seek(2);
        m_playing = true;
        setDefaultPalette();
    }

    void pause()
    {
        m_playing = !m_playing;
    }

    bool doTick()
    {
        if (!m_playing)
            return false;

        if (m_yield != 0) {
            m_yield -= 1;
            return false;
        }

        while (m_yield == 0) {
            auto a = m_cmd.next<quint8>();
            if (a & (1 << 7)) {
                m_cmd.seek(2);
                setDefaultPalette();
            }
            else {
                switch (m_opcode = a >> 2, m_opcode) {
                case 0:
                case 5:
                case 9:
                    return true;// update needed
                case 1:
                    m_clear = bool(m_cmd.next<quint8>());
                    clearScreen();
                    break;
                case 2:
                    m_yield = m_cmd.next<quint8>() * 4;
                    break;
                case 3:{
                    auto a = m_cmd.next<quint16>();
                    qint16 c = 0;
                    qint16 b = 0;
                    if (a & (1 << 15)) {
                        a &= 0x7FFF;
                        c = m_cmd.next<qint16>();
                        b = m_cmd.next<qint16>();
                    }
                    drawShape(a, c, b);
                } break;
                case 4: {
                    auto a = m_cmd.next<quint8>();
                    auto c = m_cmd.next<quint8>();
                    setPalette(a, c);
                } break;
                case 10: {
                    auto a = m_cmd.next<quint16>();
                    qint16 b = 0;
                    qint16 c = 0;
                    if (a & (1 << 15)) {
                        a &= 0x7FFF;
                        c = m_cmd.next<qint16>();
                        b = m_cmd.next<qint16>();
                    }
                    auto e = 512 + m_cmd.next<quint16>();
                    auto d = m_cmd.next<quint8>();
                    auto f = m_cmd.next<quint8>();
                    //drawShapeScale(a, c, b, e, d, f);
                } break;
                case 11: {
                    auto a = m_cmd.next<quint16>();
                    qint16 b = 0;
                    qint16 c = 0;
                    if (a & (1 << 15)) {
                        a &= 0x7FFF;
                        c = m_cmd.next<qint16>();
                        b = m_cmd.next<qint16>();
                    }
                    quint16 e = 512;
                    if (a & (1 << 14)) {
                        a &= 0x3FFF;
                        e += m_cmd.next<quint16>();
                    }
                    auto d = m_cmd.next<quint8>();
                    auto f = m_cmd.next<quint8>();
                    auto g = m_cmd.next<quint16>();
                    quint16 h = 90;
                    if (a & (1 << 13)) {
                        a &= 0x1FFF;
                        h = m_cmd.next<quint16>();
                    }
                    quint16 i = 180;
                    if (a & (1 << 12)) {
                        a &= 0xFFF;
                        i = m_cmd.next<quint16>();
                    }
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
        m_pol.seek(2);
        const auto shape_offset_index = m_pol.next<quint16>();
        m_pol.seek(shape_offset_index + offset * 2);
        offset = m_pol.next<quint16>();
        m_pol.seek(14);
        const auto shape_data_index = m_pol.next<quint16>();
        m_pol.seek(shape_data_index + offset);
        auto shape_size = m_pol.next<quint16>();
        for (int i=0; i < shape_size; ++i) {
            auto const primitive_header = m_pol.next<quint16>();
            int x = 0, y = 0;
            if (primitive_header & (1 << 15)) {
                x = m_pol.next<qint16>();
                y = m_pol.next<qint16>();
            }
            bool unused = primitive_header & (1 << 14);
            auto color_index = m_pol.next<quint8>();
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

    void setPalette(int offset, int c)
    {
        m_pol.seek(6);
        const auto palette_index = m_pol.next<quint16>();
        m_pol.seek(palette_index + offset * 32);
        const auto off = (c == 0) ? 16 : 0;
        for (size_t i = 0; i < 16; i++) {
            const auto color = m_pol.next<quint16>();
            const quint8 t = (color == 0) ? 0 : 3;
            const int r = (((color & 0xF00) >> 6) | t) * 3;
            const int g = (((color & 0x0F0) >> 2) | t) * 3;
            const int b = (((color & 0x00F) << 2) | t) * 3;
            m_palette[off+i] = QColor(r, g, b);
        }
    }

    void drawPrimitive(const Primitive &primitive, QPainter &painter)
    {
        m_pol.seek(10);
        const auto vertices_offset_index = m_pol.next<quint16>();
        m_pol.seek(vertices_offset_index + primitive.num * 2);
        const auto num_off = m_pol.next<quint16>();
        m_pol.seek(18);
        const auto vertices_data_index = m_pol.next<quint16>();
        m_pol.seek(vertices_data_index + num_off);
        auto num_vertices = m_pol.next<quint8>();
        int x = primitive.x + m_pol.next<qint16>();
        int y = primitive.y + m_pol.next<qint16>();
        painter.setBrush(m_palette[primitive.color]);
        painter.setPen(m_palette[primitive.color]);
        painter.save();
        painter.scale(m_scale, m_scale);
        if (num_vertices & (1 << 7)) {
            painter.translate(x, y);
            auto center_x = m_pol.next<qint16>();
            auto center_y = m_pol.next<qint16>();
            painter.drawEllipse(QPointF(0,0), center_x, center_y);
        }
        else if (num_vertices == 0) {
            painter.fillRect(QRectF(x, y, m_scale, m_scale), m_palette[primitive.color]);
        }
        else {
            QPainterPath path(QPointF(x, y));
            for (auto i = 0; i < num_vertices; ++i) {
                auto g = m_pol.next<qint8>();
                auto h = m_pol.next<qint8>();
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
    Stream m_cmd;
    Stream m_pol;
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
