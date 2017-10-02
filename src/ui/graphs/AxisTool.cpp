#include "AxisTool.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "GraphViewAbstract.h"

#include <QtWidgets>
#include <iostream>


AxisTool::AxisTool():
mIsHorizontal(true),
mShowSubs(true),
mShowSubSubs(true),
mShowText(true),
mMinMaxOnly(false),
mMajorScale (100),
mMinorScaleCount (4),
mAxisColor(0, 0, 0)
{
    
}

/**
 * @brief GraphViewAbstract::getXForValue find a position on a graph for a value
 * @param value
 * @return
 */
qreal AxisTool::getXForValue(const qreal &value)
{
    const qreal rigthBlank (5.); // the same name and the same value as AxisTool::updateValues()
    return (qreal)(valueForProportion(value, (qreal) mStartVal, (qreal) mEndVal, qreal (0.), (qreal) (mTotalPix-rigthBlank), true));
}

qreal AxisTool::getYForValue(const qreal &value)
{
    const qreal blank (10.);
    return (qreal)(valueForProportion(value, (qreal) mStartVal, (qreal) mEndVal, qreal (0.), (qreal) (mTotalPix-blank), true));
}

void AxisTool::updateValues(const int &totalPix, const int &minDeltaPix, const qreal &minVal, const qreal &maxVal)
{
    mStartVal = minVal;
    mEndVal   = maxVal;
    mTotalPix = totalPix;
    mMinDeltaPix = minDeltaPix;

    if ( (mTotalPix*mMajorScale) / ((mEndVal-mStartVal)*mMinorScaleCount) < mMinDeltaPix)
        mShowSubSubs = false;

}

/**
 * @brief AxisTool::setScaleDivision set the mark and the tip
 * @param sc
 */
void AxisTool::setScaleDivision (const Scale & sc)
{
    mMajorScale = sc.mark;
    mMinorScaleCount = sc.tip;
    if ( (mTotalPix*mMajorScale) / ((mEndVal-mStartVal)*mMinorScaleCount) < mMinDeltaPix)
        mShowSubSubs = false;
}

void AxisTool::setScaleDivision (const double &major, const double &minorCount)
{
    mMajorScale = major;
    mMinorScaleCount = minorCount;
    if ( (mTotalPix*mMajorScale) / ((mEndVal-mStartVal)*mMinorScaleCount) < mMinDeltaPix)
        mShowSubSubs = false;
}


/**
 * @brief Draw axis on a QPainter, if there is no valueFormatFunc, all number is converted in QString with precision 0, it's meanning only integer
 *
 */
QVector<qreal> AxisTool::paint(QPainter &p, const QRectF &r, qreal heigthSize, FormatFunc valueFormatFunc)
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
        p.drawLine(xo, yo, xo, yo + heigthSize);

        if (mMinMaxOnly) {
            if (mShowText) {
                QRectF tr(xo, yo, w, h);

                if (valueFormatFunc) {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, valueFormatFunc(mStartVal, false));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, valueFormatFunc(mEndVal, false));
                } else {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, stringWithAppSettings(mStartVal, false));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, stringWithAppSettings(mEndVal, false));
                }
            }
        }
        else {
            if ( mShowSubs && (mEndVal - mStartVal != INFINITY)) {
                mTextInc = 1;
                // look for the text increment
                int textInc (1);

                qreal prevTextWidth (0.);
                for (qreal v = mStartVal; v <= mEndVal ; v += mMajorScale)  {
                    const qreal x = getXForValue(v) + xo;

                    QString text =(valueFormatFunc ? valueFormatFunc(v, false) : stringWithAppSettings(v, false) );
                    const int textWidth =  fm.width(text) ;
                    const qreal tx = x - textWidth/2.;
                    if ( tx > prevTextWidth ) {
                        // memo previous text position
                        prevTextWidth = tx + textWidth + 1.;
                        mTextInc = std::max(mTextInc, textInc);
                        textInc = 1;
                    } else
                        ++textInc;
                }




                // draw scale with this text
                const qreal minorStep (mMajorScale/ mMinorScaleCount);

                textInc = mTextInc - 1;
                qreal xPrev (0.);
                for (qreal v = mStartVal; v <= mEndVal ; v += mMajorScale)  {
                    const qreal x = getXForValue(v) + xo;
                    linesPos.append(x);


                    ++textInc;

                    if ( textInc == mTextInc) {
                        p.drawLine(QLineF(x, yo, x, yo + heigthSize));
                        if (mShowText) {
                            const QString text =(valueFormatFunc ? valueFormatFunc(v, false) : stringWithAppSettings(v, false) );
                            const int textWidth =  fm.width(text) ;
                            const qreal tx = x - textWidth/2.;
                            const QRectF textRect(tx, yo + h - heightText, textWidth, heightText);
                            p.drawText(textRect,Qt::AlignCenter ,text);
                        }

                        textInc = 0;
                        xPrev = x;
                    } else {
                        if (mShowSubSubs)
                            p.drawLine(QLineF(x, yo, x, yo + heigthSize));
                        else if ( (x-xPrev) > mMinDeltaPix) {
                            p.drawLine(QLineF(x, yo, x, yo + heigthSize/2.));
                            xPrev = x;
                       }
                    }




                    if (mShowSubSubs && v<mEndVal) {
                        for (qreal sv = 1.; sv < mMinorScaleCount; sv += 1.) {
                            const qreal vm = getXForValue(sv*minorStep + v) + xo;
                            p.drawLine(QLineF(vm, yo, vm, yo + heigthSize/2.));
                        }
                    }


                }


            }


        }
    }
    else // ______________________vertical axe______________________________________________________
    {
        const qreal xov = r.x() + r.width();
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
            if ( mShowSubs && (mEndVal - mStartVal != INFINITY)) {
                mTextInc = 1;
                // look for the text increment
                int textInc (1);
                qreal prevTextHeight (0.);
                for (qreal v = mStartVal; v <= mEndVal ; v += mMajorScale)  {
                    const qreal y = getYForValue(v) + yov;
                    const int textHeight =  fm.height() ;
                    const qreal ty = y - textHeight/2.;

                    if ( ty > prevTextHeight) {
                        // memo previous text position
                        prevTextHeight = ty + textHeight + 1.;
                        mTextInc = std::max(mTextInc, textInc);
                        textInc = 1;
                    } else
                        ++textInc;

                }



                // draw scale with this text
                const qreal minorStep (mMajorScale/ mMinorScaleCount);

                textInc = mTextInc - 1;
                for (qreal v = mStartVal; v <= mEndVal ; v += mMajorScale)  {
                    const qreal y = yov - getYForValue(v) ;

                    p.drawLine(QLineF(xov, y, xov - heigthSize, y));
                    linesPos.append(y);
                    ++textInc;

                    if ( textInc == mTextInc) {
                        const QString text =(valueFormatFunc ? valueFormatFunc(v, false) : stringWithAppSettings(v, false) );
                        const int textHeight =  fm.height() ;

                        const int align (Qt::AlignRight | Qt::AlignVCenter);
                        const qreal ty ( y - textHeight/2 );
                        const QRectF tr(xov - w, ty, w - 8, textHeight);
                        p.drawText(tr, align, text);
                        textInc = 0;
                    }

                    if (mShowSubSubs && v<mEndVal) {
                        for (qreal sv = 1.; sv < mMinorScaleCount; sv += 1.) {
                            const qreal ym = yov - getYForValue(sv*minorStep + v);

                            p.drawLine(QLineF(xov, ym, xov - heigthSize/2., ym));
                        }
                    }

                }

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

void AxisWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    paint(p, QRectF( mMarginLeft, 0, width() - mMarginLeft - mMarginRight, height()), 7., mFormatFunct);
}

/**
 * @brief scale::findOptimal search the most aesthetic
 * @param a the minimum value which must be visible
 * @param b the maximun value which must be visible
 * @param nOptimal the target of number of step, mark-1
 */
void Scale::findOptimal(const double & a, const double & b, const int & nOptimal)
{
    double e (b - a);
    double u = int(log10(e));
    u = std::pow(10., u);

    double fract[4] = { 1. , 2. , 5. , 10. };

    double diff (std::max(u, e));
    mark =  diff;

    for (int i (0); i<5; ++i) {
        const double stp = u/fract[i];

        if (std::abs(nOptimal - e/stp) < diff) {
            diff = nOptimal - e/stp;
            mark = stp;
        }
    }

    min = std::floor(a/ mark) * mark;
    max = min + std::ceil( (b - min) / mark ) * mark;

}
