#ifndef POLYWIDGET_H
#define POLYWIDGET_H

#include <QDeclarativeItem>

class Cutscene;



class CutsceneWidget : public QDeclarativeItem
{
    Q_OBJECT
    typedef QDeclarativeItem super;

public:
    CutsceneWidget(QDeclarativeItem *parent=0);
    ~CutsceneWidget();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget=0);

    Q_INVOKABLE void play();

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
