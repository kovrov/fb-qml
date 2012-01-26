#ifndef POLYWIDGET_H
#define POLYWIDGET_H

#include <QDeclarativeItem>

class Cutscene;



class CutsceneWidget : public QDeclarativeItem
{
    Q_OBJECT
    typedef QDeclarativeItem super;

public:

    bool isPlaying() { return m_timerId != -1; }
    void setPlaying(bool play);
    Q_SIGNAL void playingChanged();
    Q_PROPERTY (bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)

    CutsceneWidget(QDeclarativeItem *parent=0);
    ~CutsceneWidget();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget=0);

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
