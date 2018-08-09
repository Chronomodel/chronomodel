#ifndef Marker_H
#define Marker_H

#include <QWidget>
#include <QPen>
#include <QBrush>


class Marker: public QWidget
{
    Q_OBJECT

public:
    Marker(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~Marker();
    int thickness() const;
    void hideMarker();
    void showMarker();
    
protected:
    virtual void paintEvent(QPaintEvent* e);
    QPen mPen;
    QBrush mBrush;
};

#endif
