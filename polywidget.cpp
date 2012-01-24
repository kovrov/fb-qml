#include "polywidget.h"
#include "resource.h"
#include "data.h"

#include <QTime>
#include <QPainter>
#include <QPaintEvent>
#include <qdebug.h>



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
            m_palette_foreground.colors[i] = m_palette_backdrop.colors[i] = QColor(i, i, i);
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
                const auto shape_info = m_cmd.next<quint16>();
                qint16 x = 0, y = 0;
                if (shape_info & (1 << 15)) {
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
                addShape(shape_info & 0x7FFF, pivot_origin * scaling_matrix * restore_origin * pos_matrix);
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

    void _drawNode(QPainter &painter, const Shape &shape, const Palete &palette)
    {
        auto scale_matrix = QTransform().fromScale(m_scale, m_scale);
        foreach (const auto &primitive, shape.primitives) {
            const QColor &color = palette.colors[primitive.color_index];
            QPen pen(color);
            pen.setWidth(1);
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);
            painter.setBrush(color);
            painter.setTransform(primitive.transform * shape.transform * scale_matrix);
            painter.drawPath(m_pol.path(primitive.path_index));
        }
    }

    void drawScene(QPainter &painter, const QRect &rect)
    {
        painter.fillRect(rect, QColor(0, 0, 0));
        foreach (const auto &primitive, m_backdrop_primitives) {
            _drawNode(painter, primitive, m_palette_backdrop);
        }
        foreach (const auto &primitive, m_foreground_primitives) {
            _drawNode(painter, primitive, m_palette_foreground);
        }
    }

private:
    void addShape(int offset, const QTransform &transform)
    {
        Shape shape = { transform };
        shape.primitives = m_pol.primitives(offset);

        if (m_clear) {
            m_backdrop_primitives.append(shape);
        }
        else {
            m_foreground_primitives.append(shape);
        }
    }

    void setPalette(int palette_index, int pal_num)
    {
        if (pal_num ^ 1) {
            m_palette_foreground = m_pol.palette(palette_index);
        }
        else {
            m_palette_backdrop = m_pol.palette(palette_index);
        }
    }

    bool m_playing;
    bool m_clear;
    Palete m_palette_foreground;
    Palete m_palette_backdrop;
    int m_scale;
    QList<Shape> m_foreground_primitives;
    QList<Shape> m_backdrop_primitives;
    Stream m_cmd;
    POL m_pol;
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
