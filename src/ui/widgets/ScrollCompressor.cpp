#include "ScrollCompressor.h"
#include "Painting.h"
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
    
    QRectF r = rect();//.adjusted(1, 1, -1, -1);
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    QLinearGradient grad1(0, 0, width(), 0);
    grad1.setColorAt(0, QColor(60, 60, 60));
    grad1.setColorAt(1, QColor(80, 80, 80));
    
    p.setPen(Qt::NoPen);
    p.setBrush(grad1);
    p.drawRect(r);
    
    QLinearGradient grad2(0, 0, width(), 0);
    grad2.setColorAt(0, mainColorLight);
    grad2.setColorAt(1, mainColorDark);
    
    float h = r.height() * mProp;
    QRectF r2 = r.adjusted(0, r.height() - h, 0, 0);
    p.fillRect(r2, grad2);
    
    p.setPen(QColor(50, 50, 50));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);
    
    if(mShowText)
    {
        QFont font = p.font();
        font.setPointSizeF(pointSize(11));
        p.setFont(font);
        
        QString text = mText + "\n" + QString::number(qRound(mProp * 100)) + " %";
        p.setPen(QColor(200, 200, 200));
        p.drawText(r, Qt::AlignCenter, text);
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
