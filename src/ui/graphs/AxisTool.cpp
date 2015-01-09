#include "AxisTool.h"
#include "Painting.h"
#include <QtWidgets>
#include <iostream>


AxisTool::AxisTool():
mIsHorizontal(true),
mShowSubs(true),
mMinMaxOnly(false),
mDeltaVal(0),
mDeltaPix(0),
mStartVal(0),
mStartPix(0)
{
    
}
void AxisTool::updateValues(double totalPix, double minDeltaPix, double minVal, double maxVal)
{
    double w = totalPix;
    w = (w <= 0.f) ? minDeltaPix : w;
    double numSteps = floor(w / minDeltaPix);
    numSteps = (numSteps <= 0) ? 1 : numSteps;
    
    double delta = maxVal - minVal;
    double unitsPerStep = delta / numSteps;
    
    double pixelsPerUnit = w / delta;
    //double unitsPerPixel = delta / w;
    
    double pow10 = 0;
    if(unitsPerStep < 1)
    {
        while(unitsPerStep < 1)
        {
            unitsPerStep *= 10;
            pow10 -= 1;
        }
    }
    else
    {
        while(unitsPerStep >= 10)
        {
            unitsPerStep /= 10;
            pow10 += 1;
        }
    }
    
    double factor = pow(10.f, pow10);
    
    //mDeltaVal = floor(unitsPerStep) * factor;
    mDeltaVal = 10.f * factor;
    mDeltaPix = mDeltaVal * pixelsPerUnit;
    
    mStartVal = ceil(minVal / mDeltaVal) * mDeltaVal;
    mStartPix = (mStartVal - minVal) * pixelsPerUnit;
    
    /*qDebug() << "------------";
     qDebug() << "w = " << w;
     qDebug() << "numSteps = " << numSteps;
     qDebug() << "delta = " << delta;
     qDebug() << "unitsPerStep = " << unitsPerStep;
     qDebug() << "pixelsPerUnit = " << pixelsPerUnit;
     qDebug() << "factor = " << factor;
     qDebug() << "---";
     qDebug() << "mStartVal = " << mStartVal;
     qDebug() << "mStartPix = " << mStartPix;
     qDebug() << "mDeltaVal = " << mDeltaVal;
     qDebug() << "mDeltaPix = " << mDeltaPix;*/
}

QVector<double> AxisTool::paint(QPainter& p, const QRectF& r, double textS)
{
    QVector<double> linesPos;
    
    QFont font = p.font();
    font.setPointSizeF(pointSize(9.f));
    p.setFont(font);
    p.setPen(Qt::black);
    
    if(mIsHorizontal)
    {
        double xo = r.x();
        double yo = r.y();
        double w = r.width();
        double h = r.height();
        
        if(mMinMaxOnly)
        {
            QRectF tr(xo, yo, w, h);
            p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, QString::number(mStartVal, 'G', 5));
            p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, QString::number(mStartVal + mDeltaVal * (w/mDeltaPix), 'G', 5));
        }
        else
        {
            int i = 0;
            for(double x = xo + mStartPix - mDeltaPix; x < xo + w; x += mDeltaPix)
            {
                if(mShowSubs)
                {
                    for(double sx = x + mDeltaPix/10; sx < std::min(x + mDeltaPix, xo + w); sx += mDeltaPix/10)
                    {
                        if(sx >= xo)
                            p.drawLine(QLineF(sx, yo, sx, yo + h/6));
                    }
                }
                
                if(x >= xo)
                {
                    p.drawLine(QLineF(x, yo, x, yo + h/3));
                    
                    int align = Qt::AlignCenter;
                    double tx = x - textS/2;
                    /*if(tx < xo)
                     {
                     tx = xo + 2;
                     align = (Qt::AlignLeft | Qt::AlignVCenter);
                     }
                     else if(tx > xo + w - textS)
                     {
                     tx = xo + w - textS;
                     align = (Qt::AlignRight | Qt::AlignVCenter);
                     }*/
                    QRectF tr(tx, yo + h/3, textS, 2*h/3);
                    p.drawText(tr, align, QString::number(mStartVal + i * mDeltaVal, 'G', 5));
                    
                    linesPos.append(x);
                    ++i;
                }
            }
        }
    }
    else
    {
        double xo = r.x() + r.width();
        double yo = r.y() + r.height();
        double w = r.width();
        double h = r.height();
        
        if(mMinMaxOnly)
        {
            QRectF tr(r.x(), r.y(), w - 8, h);
            p.drawText(tr, Qt::AlignRight | Qt::AlignBottom, QString::number(mStartVal, 'g', 2));
            p.drawText(tr, Qt::AlignRight | Qt::AlignTop, QString::number(mStartVal + mDeltaVal * (h/mDeltaPix), 'g', 2));
        }
        else
        {
            int i = 0;
            for(double y = yo - (mStartPix - mDeltaPix); y > yo - h; y -= mDeltaPix)
            {
                if(mShowSubs)
                {
                    for(double sy = y + mDeltaPix/10; sy > std::max(y - mDeltaPix, yo - h); sy -= mDeltaPix/10)
                    {
                        if(sy <= yo)
                            p.drawLine(QLineF(xo, sy, xo - 3, sy));
                    }
                }
                
                if(y <= yo)
                {
                    p.drawLine(QLineF(xo, y, xo - 6, y));
                    
                    int align = (Qt::AlignRight | Qt::AlignVCenter);
                    double ty = y - textS/2;
                    /*if(ty + textS > yo)
                     {
                     ty = yo - textS;
                     align = (Qt::AlignRight | Qt::AlignBottom);
                     }
                     else if(ty < yo - h)
                     {
                     ty = yo - h;
                     align = (Qt::AlignRight | Qt::AlignTop);
                     }*/
                    QRectF tr(xo - w, ty, w - 8, textS);
                    p.drawText(tr, align, QString::number(mStartVal + i * mDeltaVal, 'g', 5));
                    
                    linesPos.append(y);
                    ++i;
                }
            }
        }
    }
    return linesPos;
}