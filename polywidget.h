#ifndef POLYWIDGET_H
#define POLYWIDGET_H

#include <QtGui/QWidget>

class DPoly;



class PolyWidget : public QWidget
{
    Q_OBJECT

public:
    PolyWidget(QWidget *parent = 0);
    ~PolyWidget();

protected:
     void paintEvent(QPaintEvent *event);
     void timerEvent(QTimerEvent *event);

private:
    DPoly *m_poly;
};



#endif // POLYWIDGET_H
