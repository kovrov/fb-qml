#ifndef POLYWIDGET_H
#define POLYWIDGET_H

#include <QtQuick/QQuickPaintedItem>
#include <QtGui/QTransform>

class Cutscene;



class CutsceneWidget : public QQuickPaintedItem
{
    Q_OBJECT

public:
    CutsceneWidget(QQuickItem *parent=0);
    ~CutsceneWidget();

    Q_INVOKABLE void play(const QString &name);
    Q_INVOKABLE void stop();
    Q_SIGNAL void finished();

    void paint(QPainter *painter);

protected:
//    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *event);
//    void resizeEvent(QResizeEvent *event);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);

private:
    Cutscene *m_cutscene;
    QTransform m_transform;
    QRectF m_clipRect;
    int m_timerId;
};



#endif // POLYWIDGET_H
