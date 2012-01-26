#include <QTime>
#include <QPainter>
#include <QPaintEvent>
#include <qdebug.h>

#include "cutscenewidget.h"
#include "resource.h"
#include "data.h"



struct SceneNode
{
    QList<Shape> shapes;
    Palete palette;
};


class Cutscene
{
public:
    enum { WIDTH = 240, HEIGHT = 128 };
    Cutscene()
      : m_clear (false),
        m_cmd (Stream::fromBase64(dat_cmd)),
        m_pol (Stream::fromBase64(dat_pol)),
        m_start_new_shot (false)
    {
        m_scene << SceneNode()  // backdrop
                << SceneNode(); // foreground
    }

    const QList<SceneNode> & scene() const { return m_scene; }

    const QPainterPath & path(int i) { return m_pol.path(i); }

    void start()
    {
        m_cmd.seek(2);
    }

    bool doTick(int &wait)
    {
        if (m_start_new_shot) {
            m_start_new_shot = false;
            m_scene[1].shapes.clear(); // foreground
        }

        while (true) {
            const quint8 opcode = m_cmd.next<quint8>();
            if (opcode & (1 << 7))
                break;

            switch (opcode >> 2) {
            case 9: {
                // FIXME: op_handleKeys?
                const quint8 key_mask = m_cmd.next<quint8>();
            }
            case 0:
            case 5:
                m_start_new_shot = true;
                wait = 75;
                return true;  // update needed
            case 1:
                m_clear = bool(m_cmd.next<quint8>());
                if (m_clear)
                    m_scene[0].shapes.clear(); // backdrop
                break;
            case 2: {  // sleep
                const quint8 frame_delay = m_cmd.next<quint8>();
                wait = frame_delay * 60;
            }   return false;
            case 3:{
                const quint16 shape_offset = m_cmd.next<quint16>();
                qint16 x = 0, y = 0;
                if (shape_offset & (1 << 15)) {
                    x = m_cmd.next<qint16>();
                    y = m_cmd.next<qint16>();
                }
                addShape(shape_offset & 0x7FFF, QTransform::fromTranslate(x, y));
            } break;
            case 4: {
                const quint8 palette_index = m_cmd.next<quint8>();
                const quint8 pal_num = m_cmd.next<quint8>();
                setPalette(palette_index, pal_num);
            } break;
            case 6: {
                    const quint16 str_id = m_cmd.next<quint16>();
            } break;
            case 10: {
                const quint16 shape_info = m_cmd.next<quint16>();
                qint16 x = 0, y = 0;
                if (shape_info & (1 << 15)) {
                    x = m_cmd.next<qint16>();
                    y = m_cmd.next<qint16>();
                }
                quint16 zoom = 512;
                zoom += m_cmd.next<quint16>();  // expect to overflow
                const quint8 pivot_x = m_cmd.next<quint8>();
                const quint8 pivot_y = m_cmd.next<quint8>();

                QTransform pivot_origin = QTransform::fromTranslate(-pivot_x, -pivot_y);
                QTransform scaling_matrix = QTransform::fromScale(double(zoom)/512.0, double(zoom)/512.0);
                QTransform restore_origin = QTransform::fromTranslate(pivot_x, pivot_y);
                QTransform pos_matrix = QTransform::fromTranslate(x, y);
                addShape(shape_info & 0x7FFF, pivot_origin * scaling_matrix * restore_origin * pos_matrix);
            } break;
            case 11: {
                const quint16 shape_info = m_cmd.next<quint16>();
                qint16 x = 0, y = 0;
                if (shape_info & (1 << 15)) {
                    x = m_cmd.next<qint16>();
                    y = m_cmd.next<qint16>();
                }
                quint16 zoom = 512;
                if (shape_info & (1 << 14)) {
                    zoom += m_cmd.next<quint16>();  // expect to overflow
                }
                const quint8 pivot_x = m_cmd.next<quint8>();
                const quint8 pivot_y = m_cmd.next<quint8>();
                const quint16 r1 = m_cmd.next<quint16>();
                quint16 r2 = 180;
                if (shape_info & (1 << 13)) {
                    r2 = m_cmd.next<quint16>();
                }
                quint16 r3 = 90;
                if (shape_info & (1 << 12)) {
                    r3 = m_cmd.next<quint16>();
                }
                QTransform pivot_origin = QTransform::fromTranslate(-pivot_x, -pivot_y);
                QTransform scaling_matrix = QTransform::fromScale(double(zoom)/512.0, double(zoom)/512.0);
                QTransform restore_origin = QTransform::fromTranslate(pivot_x, pivot_y);
                QTransform pos_matrix = QTransform::fromTranslate(x, y);
                QTransform rotation_matrix;
                if (r2 != 180) rotation_matrix.rotate(r2-180, Qt::XAxis);
                if (r3 != 90) rotation_matrix.rotate(r3-90, Qt::YAxis);
                rotation_matrix.rotate(-r1, Qt::ZAxis);
                addShape(shape_info & 0xFFF, pivot_origin * rotation_matrix * scaling_matrix * restore_origin * pos_matrix);
            } break;
            case 12:
                wait = 150;
                return false;
            case 13: {
                const quint16 str_id = m_cmd.next<quint16>();
                if (str_id != 0xFFFF) {
                    const qint8 x = m_cmd.next<qint8>() * 8;
                    const qint8 y = m_cmd.next<qint8>() * 8;
                }
            } break;
            default:
                Q_ASSERT_X (false, "opcode_switch", QString::number(opcode >> 2).toAscii());
            }
        }
        start();
        return false;
    }

private:
    void addShape(int offset, const QTransform &transform)
    {
        Shape shape = { transform, m_pol.primitives(offset)};

        if (m_clear) {
            m_scene[0].shapes.append(shape); // backdrop
        }
        else {
            m_scene[1].shapes.append(shape); // foreground
        }
    }

    void setPalette(int palette_index, int pal_num)
    {
        if (pal_num ^ 1) {
            m_scene[1].palette = m_pol.palette(palette_index);
        }
        else {
            m_scene[0].palette = m_pol.palette(palette_index);
        }
    }

    bool m_clear;
    QList<SceneNode> m_scene;
    Stream m_cmd;
    POL m_pol;
    bool m_start_new_shot;
};



CutsceneWidget::CutsceneWidget(QDeclarativeItem *parent)
  : super (parent),
    m_cutscene (new Cutscene),
    m_timerId (-1)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    m_cutscene->start();
}


CutsceneWidget::~CutsceneWidget()
{
    delete m_cutscene;
}


void CutsceneWidget::setPlaying(bool play)
{
    if (play) {
        if (-1 == m_timerId) {
            m_timerId = startTimer(0);
            emit playingChanged();
        }
    }
    else {
        if (-1 != m_timerId) {
            killTimer(m_timerId);
            m_timerId = -1;
            emit playingChanged();
        }
    }
}


void CutsceneWidget::timerEvent(QTimerEvent *ev)
{
    Q_ASSERT (m_timerId == ev->timerId());
    killTimer(ev->timerId());
    int wait = 0;
    if (m_cutscene->doTick(wait))
        update();
    m_timerId = startTimer(wait);
}


void CutsceneWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    painter->fillRect(boundingRect(), QColor(0, 0, 0));
    painter->setClipRect(m_clipRect);
    const auto widget_transform = m_transform * sceneTransform();
    QPen pen;
    pen.setWidth(1);
    pen.setJoinStyle(Qt::MiterJoin);
    foreach (const SceneNode &node, m_cutscene->scene()) {
        foreach (const Shape &shape, node.shapes) {
            const auto shape_transform = shape.transform * widget_transform;
            foreach (const Primitive &primitive, shape.primitives) {
                const QColor &color = node.palette.colors[primitive.color_index];
                pen.setColor(color);
                painter->setPen(pen);
                painter->setBrush(color);
                painter->setTransform(primitive.transform * shape_transform);
                painter->drawPath(m_cutscene->path(primitive.path_index));
            }
        }
    }
    painter->restore();
}


void CutsceneWidget::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    const qreal scale = qMin((qreal)newGeometry.width() / (qreal)Cutscene::WIDTH,
                            (qreal)newGeometry.height() / (qreal)Cutscene::HEIGHT);
    const QSizeF scene_size(scale * Cutscene::WIDTH, scale * Cutscene::HEIGHT);
    const QSizeF diff = newGeometry.size() - scene_size;
    const QPoint translate(diff.width()/2, diff.height()/2);

    m_clipRect = QRectF(translate, scene_size);
    m_transform = QTransform::fromScale(scale, scale) * QTransform::fromTranslate(translate.x(), translate.y());

    super::geometryChanged(newGeometry, oldGeometry);
}
