#ifndef Marker_H
#define Marker_H

#include <QWidget>


class Marker: public QWidget
{
    Q_OBJECT
public:
    Marker(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~Marker();
    
    int thickness() const;
    
protected:
    void paintEvent(QPaintEvent* e);
};

#endif
