#include "ScrollCompressor.h"
#include "Painting.h"
#include <QtWidgets>


ScrollCompressor::ScrollCompressor(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mProp(0.5f),
mIsDragging(false),
mShowText(true),
mIsVertical(true)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

ScrollCompressor::~ScrollCompressor()
{
    
}

QSize ScrollCompressor::sizeHint() const
{
    return mIsVertical ? QSize(20, 100) : QSize(100, 20);
}

void ScrollCompressor::setVertical(bool vertical)
{
    mIsVertical = vertical;
    if(mIsVertical)
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    else
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    update();
}

void ScrollCompressor::setProp(const double& prop, bool sendNotification)
{
    mProp = prop;
    mProp = (mProp < 0) ? 0 : mProp;
    mProp = (mProp > 1) ? 1 : mProp;
    if(sendNotification)
        emit valueChanged(mProp);
    update();
}

double ScrollCompressor::getProp() const
{
    return mProp;
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
    
    QFont font = p.font();
    font.setPointSizeF(pointSize(11));
    p.setFont(font);
    
    if(mIsVertical)
    {
        QLinearGradient grad1(0, 0, width(), 0);
        grad1.setColorAt(0, QColor(60, 60, 60));
        grad1.setColorAt(1, QColor(80, 80, 80));
        
        p.setPen(Qt::NoPen);
        p.setBrush(grad1);
        p.drawRect(r);
        
        QLinearGradient grad2(0, 0, width(), 0);
        grad2.setColorAt(0, Painting::mainColorLight);
        grad2.setColorAt(1, Painting::mainColorDark);
        
        double h = r.height() * mProp;
        QRectF r2 = r.adjusted(0, r.height() - h, 0, 0);
        p.fillRect(r2, grad2);
        
        p.setPen(QColor(50, 50, 50));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);
        
        if(mShowText)
        {
            QString text = mText + "\n" + QString::number(qRound(mProp * 100)) + " %";
            p.setPen(QColor(200, 200, 200));
            p.drawText(r, Qt::AlignCenter, text);
        }
    }
    else
    {
        double w = r.width() * mProp;
        QRectF r2 = r.adjusted(0, 0, -r.width() + w, 0);
        
        /*QLinearGradient grad1(0, 0, 0, height());
        grad1.setColorAt(0, QColor(60, 60, 60));
        grad1.setColorAt(1, QColor(80, 80, 80));
        
        p.setPen(Qt::NoPen);
        p.setBrush(grad1);
        p.drawRect(r);
        
        QLinearGradient grad2(0, 0, 0, height());
        grad2.setColorAt(0, Painting::mainColorLight);
        grad2.setColorAt(1, Painting::mainColorDark);
        p.fillRect(r2, grad2);
         
        p.setPen(QColor(50, 50, 50));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);*/
        
        p.setPen(QColor(150, 150, 150));
        p.setBrush(QColor(150, 150, 150));
        p.drawRect(r);
        
        p.setPen(Painting::mainGreen);
        p.setBrush(Painting::mainGreen);
        p.drawRect(r2);
        
        if(mShowText)
        {
            QString text = mText + " : " + QString::number(qRound(mProp * 100)) + " %";
            p.setPen(Qt::white);
            p.drawText(r, Qt::AlignCenter, text);
        }
    }
}

void ScrollCompressor::mousePressEvent(QMouseEvent* e)
{
    updateProp(e);
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
        updateProp(e);
    }
}

void ScrollCompressor::updateProp(QMouseEvent* e)
{
    QRect r = rect();
    if(mIsVertical)
    {
        int y = r.height() - e->pos().y();
        y = (y < 0) ? 0 : y;
        y = (y > r.height()) ? r.height() : y;
        mProp = (double)y / (double)r.height();
    }
    else
    {
        int x = e->pos().x();
        x = (x < 0) ? 0 : x;
        x = (x > r.width()) ? r.width() : x;
        mProp = (double)x / (double)r.width();
    }
    emit valueChanged(mProp);
    update();
}
