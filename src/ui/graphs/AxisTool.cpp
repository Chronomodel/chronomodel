#include "AxisTool.h"
#include "Painting.h"
#include <QtWidgets>
#include <iostream>


AxisTool::AxisTool():
mIsHorizontal(true),
mShowSubs(true),
mShowSubSubs(true),
mShowText(true),
mMinMaxOnly(false),
mDeltaVal(0),
mDeltaPix(20),
mStartVal(0),
mStartPix(0),
mAxisColor(0, 0, 0)
{
    
}

void AxisTool::updateValues(double totalPix, double minDeltaPix, double minVal, double maxVal)
{
    if((minDeltaPix == 0) || (minVal==maxVal) || (totalPix == 0) || (minVal>= maxVal))
        return;
    mEndVal = maxVal;
    double w = totalPix;
    w = (w <= 0.f) ? minDeltaPix : w;
    double numSteps = floor(w / minDeltaPix);
    numSteps = (numSteps <= 0) ? 1 : numSteps;
    
    double delta = maxVal - minVal;
    double unitsPerStep = delta / numSteps;
    
     mPixelsPerUnit = w / delta;
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
        if(unitsPerStep < 100000){
            while(unitsPerStep >= 10)  {
                unitsPerStep /= 10;
                pow10 += 1;
            }
        }
    }
    
    double factor = pow(10.f, pow10);
    
    //mDeltaVal = floor(unitsPerStep) * factor;
    mDeltaVal = 10.f * factor;
    mDeltaPix = mDeltaVal * mPixelsPerUnit;
    
    mStartVal = (minVal / mDeltaVal) * mDeltaVal;
    mStartPix = (mStartVal - minVal) * mPixelsPerUnit;
    mEndVal   = mStartVal+ totalPix/ mPixelsPerUnit;
    
/*    qDebug() << "------------";
     qDebug() << "w = " << w;
     qDebug() << "numSteps = " << numSteps;
     qDebug() << "delta = " << delta;
     qDebug() << "unitsPerStep = " << unitsPerStep;
     qDebug() << "pixelsPerUnit = " << mPixelsPerUnit;
     qDebug() << "factor = " << factor;
     qDebug() << "---";
     qDebug() << "mStartVal = " << mStartVal;
     qDebug() << "mStartPix = " << mStartPix;
     qDebug() << "mDeltaVal = " << mDeltaVal;
     qDebug() << "mDeltaPix = " << mDeltaPix;*/
}

QVector<qreal> AxisTool::paint(QPainter& p, const QRectF& r, qreal heigthSize)
{
    QPen memoPen(p.pen());
    QBrush memoBrush(p.brush());
    QVector<qreal> linesPos;
    QPen pen(mAxisColor, 1, Qt::SolidLine);
    
    p.setPen(pen);
    
    QFontMetrics fm(p.font());
    int heightText= fm.height();
    qreal xo = r.x();
    qreal yo = r.y();
    qreal w = r.width();
    qreal h = r.height();
    if (heightText<h/3) heightText=trunc(h/3);
    
    if(mIsHorizontal)
    {
       // qreal yoText = (mShowText ? yo : yo - heigthSize );
        
        if (mShowArrow) { // the arrow is over the rectangle of heigthSize
            QPainterPath arrowRight;
            
            QPolygonF triangle;
            triangle << QPointF(xo + w + heigthSize*.65, yo) << QPointF(xo + w , yo - heigthSize*.65) << QPointF(xo + w, yo + heigthSize*.65);
          
            p.setBrush(mAxisColor);
            p.drawPolygon(triangle);
            
        }
        
        p.drawLine(xo, yo, xo + w, yo);
       
        
        if(mMinMaxOnly) {
            QRectF tr(xo, yo, w, h);
            p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, QString::number(mStartVal, 'G', 5));
            p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, QString::number(mStartVal + mDeltaVal * (w/mDeltaPix), 'G', 5));
        }
        else {
            int i = 0;
            for(double x = xo + mStartPix - mDeltaPix; x < xo + w; x += mDeltaPix)
            {
                if((x >= xo)) {
                    if(mShowSubSubs){
                        for(double sx = x + mDeltaPix/10; sx < std::min(x + mDeltaPix, xo + w); sx += mDeltaPix/10)
                        {
                            p.drawLine(QLineF(sx, yo, sx, yo + heigthSize/2));
                        }
                    }
                    if( mShowSubs ) {
                       p.drawLine(QLineF(x, yo, x, yo + heigthSize));
                       linesPos.append(x);
                    }
                    
                    if (mShowText) {
                        QString text;
                        if (fabs((x-xo)/mPixelsPerUnit + mStartVal)<1E-6) {
                            text = "0";
                        }
                        else text = QString::number((x-xo)/mPixelsPerUnit + mStartVal, 'G', 5);
                        
                        int textWidth =  fm.width(text) ;
                        qreal tx = x - textWidth/2;
                    
                        QRectF textRect(tx, yo + h - heightText, textWidth, heightText);
                        p.drawText(textRect,Qt::AlignCenter ,text);
                    }
                }
                
                
                if (mShowText || mShowSubs) {
                    ++i;
                }

            }
                         //   qDebug()<<"in AxisTool::paint mStartVal"<<mStartVal<<"max"<<QString::number(mStartVal + i * mDeltaVal, 'G', 5);
        }
    }
    else // ______________________vertical axe______________________________________________________
    {
        double xov = r.x() + r.width()- p.pen().width();
        double yov = r.y() + r.height();
       /* double w = r.width();
        double h = r.height();
        
        QVector<qreal> linesYPos = mAxisToolY.paint(p, QRectF(0, mMarginTop+ mGraphHeight, mMarginLeft, mGraphHeight- mMarginTop - mMarginBottom), 5);
      */
        p.drawLine(xov, yov, xov, yov - h );
       // tr(xo - w, ty, w - 8, heightText);
        
        if (mShowArrow) { // the arrow is over the rectangle of heigthSize
            
            
            QPolygonF triangle;
            
            triangle << QPointF(xov, yov - h) << QPointF(xov - heigthSize*.65, yov - h + heigthSize*.65) << QPointF(xov + heigthSize*.65, yov- h + heigthSize*.65);
            
            p.setBrush(mAxisColor);
            p.drawPolygon(triangle);
            
        }
        if(mMinMaxOnly) // used on posterior densities Maybe change the type of the text exp ou float
        {
            QRectF tr(r.x(), r.y(), w - 8, h);
            QString textStarVal=QString::number(mStartVal, 'G', 2);
            if (mStartVal==0) {
                textStarVal ="0";
            }
            p.drawText(tr, Qt::AlignRight | Qt::AlignBottom, textStarVal);
            //p.drawText(tr, Qt::AlignRight | Qt::AlignTop, QString::number(mStartVal + mDeltaVal * (h/mDeltaPix), 'G', 2));
            p.drawText(tr, Qt::AlignRight | Qt::AlignTop, QString::number(mEndVal, 'G', 2));
        }
        else
        {
            int i = 0;
            for(double y = yov - (mStartPix - mDeltaPix); y > yov - h; y -= mDeltaPix)
            {
                if(mShowSubs)
                {
                    for(double sy = y + mDeltaPix/10; sy > std::max(y - mDeltaPix, yov - h); sy -= mDeltaPix/10)
                    {
                        if(sy <= yov)
                            p.drawLine(QLineF(xov, sy, xov - 3, sy));
                    }
                }
                
                if(y <= yov)
                {
                    p.drawLine(QLineF(xov, y, xov - 6, y));
                    
                    int align = (Qt::AlignRight | Qt::AlignVCenter);
                    QString text = QString::number(mStartVal + i * mDeltaVal, 'g', 5);
                    //qDebug()<<"Axistool::paint mSartVal"<<mStartVal<<" endvalue"<<mEndVal;
                    //QString text = QString::number((-y+yov)/mPixelsPerUnit + mStartVal, 'g', 5);
                    
                    
                   // int textWidth = fm.width(text);
                   // double ty = y - textS/2;
                    qreal ty = y - heightText/2;
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
                    QRectF tr(xov - w, ty, w - 8, heightText);
                    p.drawText(tr, align, text);
                    
                    linesPos.append(y);
                    ++i;
                }
            }
        }
    }
    
    p.setPen(memoPen);
    p.setBrush(memoBrush);
    return linesPos;
}