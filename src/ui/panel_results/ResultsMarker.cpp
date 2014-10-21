#include "ResultsMarker.h"
#include <QtWidgets>


ResultsMarker::ResultsMarker(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    setMouseTracking(true);
}

ResultsMarker::~ResultsMarker()
{
    
}

int ResultsMarker::thickness() const
{
    return 1;
}

void ResultsMarker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setPen(Qt::red);
    p.setBrush(Qt::red);
    p.drawRect(rect());
}
