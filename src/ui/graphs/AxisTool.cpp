#include "AxisTool.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"

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

void AxisTool::updateValues(const int totalPix, const int minDeltaPix, const qreal minVal, const qreal maxVal)
{
    const qreal rigthBlank (5.); // the same name and the same value as GraphViewAbstract::getXForValue(
    if ((minDeltaPix == 0) || (minVal==maxVal) || (totalPix == 0) || (minVal>= maxVal))
        return;

    mEndVal = maxVal;
    qreal w = mIsHorizontal ? totalPix - rigthBlank : totalPix;
   // qreal w =totalPix;
    w = (w <= 0.) ? minDeltaPix : w;
    qreal numSteps = floor(w / (double)(minDeltaPix));
    numSteps = (numSteps <= 0.) ? 1. : numSteps;
    
    qreal delta = maxVal - minVal;
    qreal unitsPerStep = delta / numSteps;
    
     mPixelsPerUnit = w / delta;
    
    qreal pow10 = 0.;
    if (unitsPerStep < 1)  {
        while (unitsPerStep < 1)  {
            unitsPerStep *= 10.;
            pow10 -= 1;
        }
    } else  {
            while (unitsPerStep >= 10.)  {
                unitsPerStep /= 10.;
                pow10 += 1.;
            }
    }
    
    const qreal factor = pow(10., pow10);
    
    mDeltaVal = 10. * factor;

    mDeltaPix = mDeltaVal * mPixelsPerUnit;
    
    mStartVal = minVal;
    mStartPix = 0;//(mStartVal - minVal) * mPixelsPerUnit;
    mEndVal   = mStartVal+ (double)(totalPix)/ mPixelsPerUnit;
    
    qDebug() << "-----------mIsHorizontal-"<<mIsHorizontal;
    qDebug()<< "totalPix= "<<totalPix << "minDeltaPix= "<<minDeltaPix<< "  minVal "<< minVal<<"maxVal "<< maxVal;
     qDebug() << "w = " << w;
     qDebug() << "pow10= "<<pow10;
     qDebug() << "numSteps = " << numSteps;
     qDebug() << "delta = " << delta;
     qDebug() << "unitsPerStep = " << unitsPerStep ;
     qDebug() << "pixelsPerUnit = " << mPixelsPerUnit<<" pixel per year";
     qDebug() << "factor = " << factor;
     qDebug() << "---";
     qDebug() << "mStartVal = " << mStartVal;
     qDebug() << "mStartPix = " << mStartPix;
     qDebug() << "mEndVal = " << mEndVal;
     qDebug() << "mDeltaVal = " << mDeltaVal;
     qDebug() << "mDeltaPix = " << mDeltaPix<< "pixel between grade";

}
/**
 * @brief Draw axis on a QPainter, if there is no valueFormatFunc, all number is converted in QString with precision 0, it's mean only integer
 *
 */
QVector<qreal> AxisTool::paint(QPainter& p, const QRectF& r, qreal heigthSize, FormatFunc valueFormatFunc)
{
    QPen memoPen(p.pen());
    QBrush memoBrush(p.brush());
    QVector<qreal> linesPos;

    QPen pen(Qt::SolidLine);
    pen.setColor(mAxisColor);
    pen.setWidth(1);
    
    p.setPen(pen);

    QFontMetrics fm (p.font());
    int heightText = fm.height();
    qreal xo = r.x();
    qreal yo = r.y();
    qreal w = r.width();
    qreal h = r.height();
    
    if (mIsHorizontal) {
       if (mShowArrow) { // the arrow is over the rectangle of heigthSize
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
                
                if (valueFormatFunc) {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, valueFormatFunc(mStartVal, false));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, valueFormatFunc(mStartVal + mDeltaVal * (w/mDeltaPix), false));
                } else {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, stringWithAppSettings(mStartVal, false));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, stringWithAppSettings(mStartVal + mDeltaVal * (w/mDeltaPix), false));
                }
            }
        }
        else {
            int i (0);
            for (qreal x = xo + mStartPix - mDeltaPix; x <= xo + w ; x += mDeltaPix) {
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
                            QString text =(valueFormatFunc ? valueFormatFunc((x-xo)/mPixelsPerUnit + mStartVal, false) : stringWithAppSettings(((x-xo)/mPixelsPerUnit + mStartVal), false) );

                            const int textWidth =  fm.width(text) ;
                            const qreal tx = x - textWidth/2;
                            //mAxisToolX.updateValues(mGraphWidth, mStepMinWidth, mCurrentMinX, mCurrentMaxX);
                            //mAxisToolX.paint(p, QRectF(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth , mMarginBottom), (qreal) 7.,stringWithAppSettings);


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
        if (!mShowText) {
            // Nothing else to draw !
        }
        else if (mMinMaxOnly) // used on posterior densities Maybe change the type of the text exp ou float
        {
            if (mShowText){
                const QRectF tr(r.x(), r.y(), w - 8, h);
                const QString textStarVal = (valueFormatFunc ? valueFormatFunc(mStartVal, false) : QString::number(mStartVal,'f', 0) );
                
                p.drawText(tr, Qt::AlignRight | Qt::AlignBottom, textStarVal);
                const QString textEndVal = (valueFormatFunc ? valueFormatFunc(mEndVal, false) : QString::number(mEndVal,'f', 0) );
                p.drawText(tr, Qt::AlignRight | Qt::AlignTop, textEndVal);
            }
        } else  {
            int i(0);
            for (qreal y = yov - (mStartPix - mDeltaPix); y > yov - h; y -= mDeltaPix) {
                if (mShowSubSubs) {
                    for (qreal sy = y + mDeltaPix/10.; sy > std::max(y - mDeltaPix, yov - h); sy -= mDeltaPix/10.) {
                        if (sy <= yov)
                            p.drawLine(QLineF(xov, sy, xov - 3, sy));
                    }
                }
                
                if (y <= yov) {
                    if ( mShowText ) {
                        const int align (Qt::AlignRight | Qt::AlignVCenter);
                        const QString text =(valueFormatFunc ? valueFormatFunc(mEndVal-(y-yo)/mPixelsPerUnit, false) : QString::number((mEndVal-(y-yo)/mPixelsPerUnit ),'f',0) );
                        //const QString text = QString::number((mStartVal-(y-yov)/mPixelsPerUnit ),'f',0);
                        const qreal ty ( y - heightText/2 );
                        const QRectF tr(xov - w, ty, w - 8, heightText);
                        p.drawText(tr, align, text);
                    }
                    
                    if ( mShowSubs ) {
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
mMarginLeft(0.),
mMarginRight(0.)
{
    mFormatFunct = funct;
}

void AxisWidget::paintEvent(QPaintEvent*){
    QPainter p(this);
    paint(p, QRectF( mMarginLeft, 0, width() - mMarginLeft - mMarginRight, height()), 7., mFormatFunct);
}

