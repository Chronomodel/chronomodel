#include "ScrollCompressor.h"
#include <QtWidgets>


ScrollCompressor::ScrollCompressor(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mProp(0.5f),
mIsDragging(false),
mShowText(false)
{
    
}
ScrollCompressor::~ScrollCompressor()
{
    
}

void ScrollCompressor::setProp(const float& prop, bool sendNotification)
{
    mProp = prop;
    mProp = (mProp < 0) ? 0 : mProp;
    mProp = (mProp > 1) ? 1 : mProp;
    if(sendNotification)
        emit valueChanged(mProp);
    update();
}

void ScrollCompressor::showText(const QString& text, bool show)
{
    mShowText = show;
    mText = text;
    update();
}

void ScrollCompressor::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    QRectF r = rect();
    p.setPen(QColor(50, 50, 50));
    p.setBrush(QColor(110, 110, 110));
    p.drawRect(r);
    
    float h = (r.height()-2) * mProp;
    r = QRectF(r.x()+1.f, r.y()+r.height() - 1.f - h, r.width()-2.f, h);
    p.fillRect(r, QColor(40, 160, 206));
    
    if(mShowText)
    {
        QString text = mText + "\n" + QString::number(mProp) + " %";
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, mText);
    }
}

void ScrollCompressor::mousePressEvent(QMouseEvent* e)
{
    updateProp(e->pos().y());
    mIsDragging = true;
}

void ScrollCompressor::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    mIsDragging = false;
}

void ScrollCompressor::mouseMoveEvent(QMouseEvent* e)
{
    if(mIsDragging)
    {
        updateProp(e->pos().y());
    }
}

void ScrollCompressor::updateProp(int y)
{
    QRect r = rect();
    y = r.height() - y;
    y = (y < 0) ? 0 : y;
    y = (y > r.height()) ? r.height() : y;
    mProp = (float)y / (float)r.height();
    emit valueChanged(mProp);
    update();
}
