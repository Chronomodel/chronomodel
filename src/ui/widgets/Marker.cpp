#include "Marker.h"
#include <QtWidgets>


Marker::Marker(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    setMouseTracking(false);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Marker::~Marker()
{
    
}

int Marker::thickness() const
{
    return 1;
}

void Marker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setPen(Qt::red);
    p.setBrush(Qt::red);
    p.drawRect(rect());
}
