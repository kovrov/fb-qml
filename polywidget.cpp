#include "polywidget.h"
#include "data.h"

#include <QTime>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QtEndian>
#include <qdebug.h>



class Stream
{
public:
    static
    Stream fromBase64(const QByteArray &base64)
    {
        return Stream(decode(QByteArray::fromBase64(base64)));
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
        }
        else if (sizeof(T) == 2) {
            auto data = (const uchar*)_data.constData();
            T res = qFromBigEndian<T>(data + _pos);
            _pos += 2;
            return res;
        }
    }

private:
    Stream(const QByteArray &data) : _data (data), _pos (0) {}

    static
    QByteArray decode(const QByteArray &array)
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

    const QByteArray _data;
    int _pos;
};



struct Primitive
{
    QTransform transform;
    int color;
    QPainterPath path;
};



struct Shape
{
    QTransform transform;
    QList<Primitive> primitives;
};



class DPoly
{
public:
    DPoly()
      : m_playing (false),
        m_clear (false),
        m_scale (3),
        m_cmd (Stream::fromBase64(dat_cmd)),
        m_pol (Stream::fromBase64(dat_pol)),
        m_start_new_shot (false)
    {
    }

    void start()
    {
        m_cmd.seek(2);
        m_playing = true;
        // set default palette
        for (auto i = 0; i < 16; i++)
            m_palette[i] = m_palette[16 + i] = QColor(i, i, i);
    }

    void pause()
    {
        m_playing = !m_playing;
    }

    bool doTick(int &wait)
    {
        if (!m_playing)
            return false;

        if (m_start_new_shot) {
            m_start_new_shot = false;
            m_foreground_primitives.clear();
        }

        while (true) {
            const auto opcode = m_cmd.next<quint8>();
            if (opcode & (1 << 7))
                break;

            switch (opcode >> 2) {
            case 0:
            case 5:
            case 9:
                m_start_new_shot = true;
                wait = 75;
                return true;  // update needed
            case 1:
                m_clear = bool(m_cmd.next<quint8>());
                if (m_clear)
                    m_backdrop_primitives.clear();
                break;
            case 2: {  // sleep
                const auto frame_delay = m_cmd.next<quint8>();
                wait = frame_delay * 60;
            }   return false;
            case 3:{
                const auto shape_offset = m_cmd.next<quint16>();
                qint16 x = 0, y = 0;
                if (shape_offset & (1 << 15)) {
                    x = m_cmd.next<qint16>();
                    y = m_cmd.next<qint16>();
                }
                addShape(shape_offset & 0x7FFF, QTransform().fromTranslate(x, y));
            } break;
            case 4: {
                const auto palette_index = m_cmd.next<quint8>();
                const auto pal_num = m_cmd.next<quint8>();
                setPalette(palette_index, pal_num);
            } break;
            case 10: {
                const auto shape_offset = m_cmd.next<quint16>();
                qint16 x = 0, y = 0;
                if (shape_offset & (1 << 15)) {
                    x = m_cmd.next<qint16>();
                    y = m_cmd.next<qint16>();
                }
                quint16 zoom = 512;
                zoom += m_cmd.next<quint16>();  // expect to overflow
                const auto pivot_x = m_cmd.next<quint8>();
                const auto pivot_y = m_cmd.next<quint8>();

                auto pos_matrix = QTransform().fromTranslate(x, y);
                auto scaling_matrix = QTransform().fromScale(double(zoom)/512.0, double(zoom)/512.0);
                auto pivot_origin = QTransform().fromTranslate(-pivot_x, -pivot_y);
                auto restore_origin = QTransform().fromTranslate(pivot_x, pivot_y);
                addShape(shape_offset & 0x7FFF, pivot_origin * scaling_matrix * restore_origin * pos_matrix);
            } break;
            case 11: {
                const auto shape_offset = m_cmd.next<quint16>();
                qint16 x = 0, y = 0;
                if (shape_offset & (1 << 15)) {
                    y = m_cmd.next<qint16>();
                    x = m_cmd.next<qint16>();
                }
                quint16 zoom = 512;
                if (shape_offset & (1 << 14)) {
                    zoom += m_cmd.next<quint16>();  // expect to overflow
                }
                const auto pivot_x = m_cmd.next<quint8>();
                const auto pivot_y = m_cmd.next<quint8>();
                const auto r1 = m_cmd.next<quint16>();
                quint16 r2 = 90;
                if (shape_offset & (1 << 13)) {
                    r2 = m_cmd.next<quint16>();
                }
                quint16 r3 = 180;
                if (shape_offset & (1 << 12)) {
                    r3 = m_cmd.next<quint16>();
                }
//                addShape(shape_offset & 0xFFF, y, x, double(zoom)/512.0, pivot_x, pivot_y, r1, r2, r3);
            } break;
            case 12:
                wait = 150;
                return false;
            default:
                m_playing = false;
            }
        }
        start();
        return false;
    }

    void _drawNode(QPainter &painter, const Shape &shape)
    {
        auto scale_matrix = QTransform().fromScale(m_scale, m_scale);
        foreach (const auto &primitive, shape.primitives) {
            const QColor &color = m_palette[primitive.color];
            QPen pen(color);
            pen.setWidth(1);
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);
            painter.setBrush(color);
            painter.setTransform(primitive.transform * shape.transform * scale_matrix);
            painter.drawPath(primitive.path);
        }
    }

    void drawScene(QPainter &painter, const QRect &rect)
    {
        painter.fillRect(rect, QColor(0, 0, 0));
        foreach (const auto &primitive, m_backdrop_primitives) {
            _drawNode(painter, primitive);
        }
        foreach (const auto &primitive, m_foreground_primitives) {
            _drawNode(painter, primitive);
        }
    }

private:
    void addShape(int offset, const QTransform &transform)
    {
        Shape shape = { transform };

        m_pol.seek(2);
        const auto shape_offset_index = m_pol.next<quint16>();
        m_pol.seek(shape_offset_index + offset * 2);
        offset = m_pol.next<quint16>();
        m_pol.seek(14);
        const auto shape_data_index = m_pol.next<quint16>();
        m_pol.seek(shape_data_index + offset);
        const auto shape_size = m_pol.next<quint16>();
        for (int i=0; i < shape_size; ++i) {
            const auto primitive_header = m_pol.next<quint16>();
            int x = 0, y = 0;
            if (primitive_header & (1 << 15)) {
                x = m_pol.next<qint16>();
                y = m_pol.next<qint16>();
            }
            bool unused = primitive_header & (1 << 14);
            const auto color_index = m_pol.next<quint8>() + (m_clear ? 0 : 16);
            // queue primitive
            auto pos = m_pol.pos();
            Primitive p = {QTransform().fromTranslate(x, y), color_index, drawPrimitive(primitive_header & 0x3FFF)};
            m_pol.seek(pos);
            shape.primitives.append(p);
        }


        if (m_clear) {
            m_backdrop_primitives.append(shape);
        }
        else {
            m_foreground_primitives.append(shape);
        }
    }

    void setPalette(int palette_index, int pal_num)
    {
        m_pol.seek(6);
        const auto palette_data_offset = m_pol.next<quint16>();
        m_pol.seek(palette_data_offset + palette_index * 32);
        const auto index = (pal_num == 0) ? 16 : 0;
        for (size_t i = 0; i < 16; i++) {
            const auto color = m_pol.next<quint16>();
            const quint8 t = (color == 0) ? 0 : 3;
            const int r = (((color & 0xF00) >> 6) | t) * 3;
            const int g = (((color & 0x0F0) >> 2) | t) * 3;
            const int b = (((color & 0x00F) << 2) | t) * 3;
            m_palette[index+i] = QColor(r, g, b);
        }
    }

    QPainterPath drawPrimitive(const int primitive_num)
    {
        m_pol.seek(10);
        const auto vertices_offset_index = m_pol.next<quint16>();
        m_pol.seek(vertices_offset_index + primitive_num * 2);
        const auto num_off = m_pol.next<quint16>();
        m_pol.seek(18);
        const auto vertices_data_index = m_pol.next<quint16>();
        m_pol.seek(vertices_data_index + num_off);
        const auto num_vertices = m_pol.next<quint8>();
        int x = m_pol.next<qint16>();
        int y = m_pol.next<qint16>();

        QPainterPath path;
        if (num_vertices & (1 << 7)) {
            const auto center_x = m_pol.next<qint16>();
            const auto center_y = m_pol.next<qint16>();
            path.addEllipse(QPointF(x, y), center_x, center_y);
        }
        else if (num_vertices == 0) {
            path.addRect(QRectF(x, y, 1, 1));
        }
        else {
            path.moveTo(x, y);
            for (auto i = 0; i < num_vertices; ++i) {
                const auto g = m_pol.next<qint8>();
                const auto h = m_pol.next<qint8>();
                x += g;
                y += h;
                path.lineTo(x, y);
            }
        }
        return path;
    }

    bool m_playing;
    bool m_clear;
    QColor m_palette[32];
    int m_scale;
    QList<Shape> m_foreground_primitives;
    QList<Shape> m_backdrop_primitives;
    Stream m_cmd;
    Stream m_pol;
    bool m_start_new_shot;
};



PolyWidget::PolyWidget(QWidget *parent)
  : QWidget (parent),
    m_poly (new DPoly)
{
    m_poly->start();
    startTimer(0);
}


PolyWidget::~PolyWidget()
{
    delete m_poly;
}


void PolyWidget::timerEvent(QTimerEvent *ev)
{
    killTimer(ev->timerId());
    int wait = 0;
    if (m_poly->doTick(wait))
        update();
    startTimer(wait);
}


void PolyWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    m_poly->drawScene(painter, event->rect());
}
