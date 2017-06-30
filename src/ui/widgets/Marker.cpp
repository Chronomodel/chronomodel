#include "Marker.h"
#include <QtWidgets>


Marker::Marker(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
   mPen(Qt::red),
   mBrush(Qt::red)
{
    mPen.setStyle(Qt::SolidLine);
    mBrush.setStyle(Qt::SolidPattern);

    setMouseTracking(false);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Marker::~Marker()
{
    
}
void Marker::hideMarker()
{
    QWidget::setVisible(false);
    update();
}
void Marker::showMarker()
{
    QWidget::setVisible(true);
    update();
}

int Marker::thickness() const
{
    return 1;
}

void Marker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    if (QWidget::isVisible()) {
        QPainter p(this);
        p.setPen(mPen);
        p.setBrush(mBrush);
        p.drawRect(rect());
    }
}
