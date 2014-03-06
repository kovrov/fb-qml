#ifndef RESOURCE_H
#define RESOURCE_H

#include <QPainterPath>
#include <QTransform>
#include <QtEndian>



struct Primitive
{
    bool has_alpha;
    int color_index;
    QTransform transform;
    int path_index;
};



struct Shape
{
    QTransform transform;
    QList<Primitive> primitives;
};

struct Palete { QColor colors[16]; };



class BigEndianStream
{
public:
    static BigEndianStream fromFileInfo(const class QFileInfo &fi);
    void seek(int pos) { _pos = pos; }
    int pos() const { return _pos; }

    template<typename T>
    T next()
    {
        if (sizeof(T) == 1) {
            T res = (T)_data[_pos];
            _pos += 1;
            return res;
        } else if (sizeof(T) == 2) {
            auto data = (const uchar*)_data.constData();
            T res = qFromBigEndian<T>(data + _pos);
            _pos += 2;
            return res;
        }
    }

private:
    BigEndianStream(const QByteArray &data);

    const QByteArray _data;
    int _pos;
};



class POL
{
public:
    POL(BigEndianStream stream);
    const Palete & palette(int i) { return m_palettes.at(i); }
    const QList<Primitive> & primitives(int i) { return m_primitives.at(i); }
    const QPainterPath & path(int i) { return m_paths.at(i); }

private:
    QVector<Palete> m_palettes;
    QList<QList<Primitive> > m_primitives;
    QList<QPainterPath> m_paths;
};



#endif // RESOURCE_H
