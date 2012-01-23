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
    QPainterPath path;
};



struct Shape
{
    QTransform transform;
    QList<Primitive> primitives;
};

struct Palete { QColor colors[16]; };



class Stream
{
public:
    static Stream fromBase64(const QByteArray &base64);
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
    Stream(const QByteArray &data);
    static QByteArray decode(const QByteArray &array);

    const QByteArray _data;
    int _pos;
};



class POL
{
public:
    POL(Stream stream);

private:
    void parse_poly(Stream &stream);

public:  /// FIXME: accessors!
    QVector<Palete> m_palettes;
    QList< QList<Primitive> > m_primitive_groups;
};



#endif // RESOURCE_H
