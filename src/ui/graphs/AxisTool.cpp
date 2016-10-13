#include "AxisTool.h"
#include "Painting.h"
#include "StdUtilities.h"

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
mPixelsPerUnit(1),
mAxisColor(0, 0, 0)
{
    
}

void AxisTool::updateValues(const double totalPix, const double minDeltaPix, const double minVal, const double maxVal)
{
    if ((minDeltaPix == 0) || (minVal==maxVal) || (totalPix == 0) || (minVal>= maxVal))
        return;

    mEndVal = maxVal;
    double w = totalPix;
    w = (w <= 0.) ? minDeltaPix : w;
    double numSteps = floor(w / minDeltaPix);
    numSteps = (numSteps <= 0.) ? 1. : numSteps;
    
    double delta = maxVal - minVal;
    double unitsPerStep = delta / numSteps;
    
     mPixelsPerUnit = w / delta;
    //double unitsPerPixel = delta / w;
    
    double pow10 = 0.;
    if (unitsPerStep < 1)  {
        while (unitsPerStep < 1)  {
            unitsPerStep *= 10.;
            pow10 -= 1;
        }
    } else  {
       // if (unitsPerStep < 100000) {
            while (unitsPerStep >= 10.)  {
                unitsPerStep /= 10.;
                pow10 += 1.;
            }
       // }
    }
    
    double factor = pow(10.f, pow10);
    
    //mDeltaVal = floor(unitsPerStep) * factor;
    mDeltaVal = 10. * factor;
    mDeltaPix = mDeltaVal * mPixelsPerUnit;
    
    mStartVal = (minVal / mDeltaVal) * mDeltaVal;
    mStartPix = (mStartVal - minVal) * mPixelsPerUnit;
    mEndVal   = mStartVal+ totalPix/ mPixelsPerUnit;
    
   /* qDebug() << "------------";
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
     qDebug() << "mDeltaPix = " << mDeltaPix;
    */
}
/**
 * @brief Draw axis on a QPainter, if there is no valueFormatFunc, all number is converted in QString with precision 0, it's mean only integer
 *
 */
QVector<qreal> AxisTool::paint(QPainter& p, const QRectF& r, qreal heigthSize, QString (*valueFormatFunc)(float))
{
    QPen memoPen(p.pen());
    QBrush memoBrush(p.brush());
    QVector<qreal> linesPos;

    QPen pen(Qt::SolidLine);
    pen.setColor(mAxisColor);
    pen.setWidth(1);
    
    p.setPen(pen);
    
    QFontMetrics fm (p.font());
    int heightText= fm.height();
    qreal xo = r.x();
    qreal yo = r.y();
    qreal w = r.width();
    qreal h = r.height();
    
    
    if (mIsHorizontal) {
       if (mShowArrow) { // the arrow is over the rectangle of heigthSize
            //QPainterPath arrowRight;
            
            //QPolygonF triangle;
            //triangle << QPointF(xo + w + heigthSize*.65, yo) << QPointF(xo + w , yo - heigthSize*.65) << QPointF(xo + w, yo + heigthSize*.65);
          
            QPolygonF triangle (std::initializer_list<QPointF>({ QPointF(xo + w + heigthSize*.65, yo),
                                                                 QPointF(xo + w , yo - heigthSize*.65),
                                                                 QPointF(xo + w, yo + heigthSize*.65) }));


            p.setBrush(mAxisColor);
            p.drawPolygon(triangle);
        }
        
        p.drawLine(xo, yo, xo + w, yo);       

        
        if (mMinMaxOnly) {
            if (mShowText){
                QRectF tr(xo, yo, w, h);
                
                if (valueFormatFunc != 0) {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, valueFormatFunc(mStartVal));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, valueFormatFunc(mStartVal + mDeltaVal * (w/mDeltaPix)));
                } else {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, QString::number(mStartVal, 'f', 0));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, QString::number(mStartVal + mDeltaVal * (w/mDeltaPix), 'f', 0));
                }
            }
        }
        else {
            int i (0);
            for(qreal x = xo + mStartPix - mDeltaPix; x <= xo + w ; x += mDeltaPix) {
                if ((x >= xo)) {
                    if (mShowSubSubs) {
                        for (qreal sx = x + mDeltaPix/10; sx < std::min(x + mDeltaPix, xo + w); sx += mDeltaPix/10)
                            p.drawLine(QLineF(sx, yo, sx, yo + heigthSize/2));

                    }
                    if ( mShowSubs ) {
                       p.drawLine(QLineF(x, yo, x, yo + heigthSize));
                       linesPos.append(x);
                    }

                     if (mShowText && mPixelsPerUnit>0) {
                            QString text =(valueFormatFunc ? valueFormatFunc((x-xo)/mPixelsPerUnit + mStartVal) : QString::number(((x-xo)/mPixelsPerUnit + mStartVal),'f',0) );

                            const int textWidth =  fm.width(text) ;
                            const qreal tx = x - textWidth/2;

                            QRectF textRect(tx, yo + h - heightText, textWidth, heightText);
                            p.drawText(textRect,Qt::AlignCenter ,text);

                   }

                }
                if (mShowText || mShowSubs)
                    ++i;

            }
        }
    }
    else // ______________________vertical axe______________________________________________________
    {
        const qreal xov = r.x() + r.width()- p.pen().width();
        const qreal yov = r.y() + r.height();
       
        p.drawLine(xov, yov, xov, yov - h );
        
        if (mShowArrow) { // the arrow is over the rectangle of heigthSize
            
            QPolygonF triangle (std::initializer_list<QPointF>({ QPointF(xov, yov - h),
                                                                 QPointF(xov - heigthSize*.65, yov - h + heigthSize*.65),
                                                                 QPointF(xov + heigthSize*.65, yov - h + heigthSize*.65) }));

            p.setBrush(mAxisColor);
            p.drawPolygon(triangle);
            
        }
        if (!mShowText){
            // Nothing else to draw !
        }
        else if (mMinMaxOnly) // used on posterior densities Maybe change the type of the text exp ou float
        {
            if (mShowText){
                const QRectF tr(r.x(), r.y(), w - 8, h);
                const QString textStarVal = (valueFormatFunc ? valueFormatFunc(mStartVal) : QString::number(mStartVal,'f', 0) );
                
                p.drawText(tr, Qt::AlignRight | Qt::AlignBottom, textStarVal);
                const QString textEndVal = (valueFormatFunc ? valueFormatFunc(mEndVal) : QString::number(mEndVal,'f',0) );
                p.drawText(tr, Qt::AlignRight | Qt::AlignTop, textEndVal);
            }
        } else  {
            int i(0);
            for (qreal y = yov - (mStartPix - mDeltaPix); y > yov - h; y -= mDeltaPix) {
                if (mShowSubSubs) {
                    for (qreal sy = y + mDeltaPix/10; sy > std::max(y - mDeltaPix, yov - h); sy -= mDeltaPix/10) {
                        if (sy <= yov)
                            p.drawLine(QLineF(xov, sy, xov - 3, sy));
                    }
                }
                
                if (y <= yov) {
                    if ( mShowText ) {
                        const int align (Qt::AlignRight | Qt::AlignVCenter);
                        const QString text =(valueFormatFunc ? valueFormatFunc(mEndVal-(y-yo)/mPixelsPerUnit) : QString::number((mEndVal-(y-yo)/mPixelsPerUnit ),'f',0) );
                        
                        const qreal ty ( y - heightText/2 );
                        const QRectF tr(xov - w, ty, w - 8, heightText);
                        p.drawText(tr, align, text);
                    }
                    
                    if( mShowSubs ) {
                        p.drawLine(QLineF(xov, y, xov - 6, y));
                        linesPos.append(y);
                    }
                    
                }
                ++i;
                
            }
        }
    }
    
    p.setPen(memoPen);
    p.setBrush(memoBrush);
    return linesPos;
}




AxisWidget::AxisWidget(FormatFunc funct, QWidget* parent):QWidget(parent),
mMarginLeft(0),
mMarginRight(0)
{
    mFormatFunct = funct;
}

void AxisWidget::paintEvent(QPaintEvent*){
    QPainter p(this);
    //updateValues(width() - mMarginLeft - mMarginRight, 50, mStartVal, mEndVal);
    paint(p, QRect( mMarginLeft, 0, width() - mMarginLeft - mMarginRight, height()), 7, mFormatFunct);
}

