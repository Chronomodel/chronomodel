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
mShowArrow(false),
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
    return qreal(valueForProportion(value, qreal (mStartVal), qreal (mEndVal), qreal (0.), qreal (mTotalPix - rigthBlank), true));
}

qreal AxisTool::getYForValue(const qreal &value)
{
    const qreal blank (10.);
    return qreal(valueForProportion(value, qreal (mStartVal), qreal (mEndVal), qreal (0.), qreal (mTotalPix-blank), true));
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
    setScaleDivision(sc.mark, sc.tip);
}

void AxisTool::setScaleDivision (const double &major, const int &minorCount)
{
    mMajorScale = major;
    mMinorScaleCount = minorCount;
    if ( (mTotalPix * mMajorScale) / ((mEndVal - mStartVal) * mMinorScaleCount) < mMinDeltaPix)
        mShowSubSubs = false;
}


/**
 * @brief Draw axis on a QPainter, if there is no valueFormatFunc, all number is converted in QString with precision 0, it's meanning only integer
 * @param graduationSize size of the main graduation; if -1 indiquate automatique size according to the font; default = -1
 */
QVector<qreal> AxisTool::paint(QPainter &p, const QRectF &r, qreal graduationSize, DateConversion valueFormatFunc)
{
    QPen memoPen(p.pen());
    QBrush memoBrush(p.brush());
    QVector<qreal> linesPos;

    QPen pen(p.pen());//Qt::SolidLine);
    pen.setColor(mAxisColor);
    pen.setCapStyle(Qt::SquareCap);

    p.setPen(pen);
    p.setRenderHints(QPainter::Antialiasing);

    QFontMetrics fm (p.font());

#ifdef Q_OS_MAC
    const int textHeight (int (1.1 * (fm.descent() + fm.ascent()) ));
#else
    const int textHeight = fm.height();
#endif

    if (graduationSize == -1.)
        graduationSize = textHeight /3;

    qreal xo = r.x();
    qreal yo = r.y();
    qreal w = r.width();
    qreal h = r.height();
    if (mIsHorizontal) {
       if (mShowArrow) { // the arrow is over the rectangle of heigthSize
            QPolygonF triangle (std::initializer_list<QPointF>({ QPointF(xo + w + graduationSize*.65, yo),
                                                                 QPointF(xo + w , yo - graduationSize*.65),
                                                                 QPointF(xo + w, yo + graduationSize*.65) }));


            p.setBrush(mAxisColor);
            p.drawPolygon(triangle);
        }

        p.drawLine(QLineF(xo, yo, xo + w, yo)); // QLineF() done a smaller line

        if (mMinMaxOnly) {
            if (mShowText) {
                QRectF tr(xo, yo, w, h);

                if (valueFormatFunc) {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter,stringForGraph(valueFormatFunc(mStartVal)));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter,stringForGraph(valueFormatFunc(mEndVal)));
                } else {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, stringForGraph(mStartVal));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, stringForGraph(mEndVal));
                }
            }
        }
        else {
            if (  mShowSubs && (mEndVal - mStartVal != HUGE_VAL) && (mEndVal > mStartVal) && (mMajorScale > 0)) {

                // look for the text increment
                const QString textMin =(valueFormatFunc ? stringForGraph(valueFormatFunc(mStartVal)) : stringForGraph(mStartVal) );
                const int textMinWidth =  fm.boundingRect(textMin).width() ;

                const QString textMax =(valueFormatFunc ?stringForGraph(valueFormatFunc(mEndVal)) : stringForGraph(mEndVal) );
                const int textMaxWidth =  fm.boundingRect(textMax).width() ;

                const double nbPossibleText = std::abs(getXForValue(mStartVal) -getXForValue(mEndVal)) / (std::max(textMinWidth, textMaxWidth) + 5.);

                const double nbTheoText = std::abs(getXForValue(mStartVal) -getXForValue(mEndVal)) / std::abs(getXForValue(mStartVal) - getXForValue(mStartVal + mMajorScale));

                if (nbTheoText > nbPossibleText)
                    mTextInc = int (std::ceil(nbTheoText/nbPossibleText));
                else
                    mTextInc = 1;

                // draw scale with this text
                const qreal minorStep (mMajorScale/ mMinorScaleCount);

                int textInc = mTextInc - 1;
                qreal xPrev (0.);

                for (qreal v = mStartVal; v <= mEndVal ; v += mMajorScale)  {
                    const qreal x = getXForValue(v) + xo;
                    linesPos.append(x);
                    ++textInc;

                    if ( textInc == mTextInc) {
                        p.drawLine(QLineF(x, yo, x, yo + graduationSize));
                        if (mShowText) {
                            const QString text =(valueFormatFunc ? stringForGraph(valueFormatFunc(v)) : stringForGraph(v) );
                            const int textWidth =  fm.width(text) ;
                            const qreal tx = x - textWidth/2.;
                            const QRectF textRect(tx, yo + h - textHeight, textWidth, textHeight);
                            p.drawText(textRect,Qt::AlignCenter ,text);
                        }

                        textInc = 0;
                        xPrev = x;
                    } else {
                        if (mShowSubSubs)
                            p.drawLine(QLineF(x, yo, x, yo + graduationSize));
                        else if ( (x-xPrev) > mMinDeltaPix) {
                            p.drawLine(QLineF(x, yo, x, yo + graduationSize/2.));
                            xPrev = x;
                       }
                    }



                    if (mShowSubSubs && v<mEndVal) {
                        for (qreal sv = 1.; sv < mMinorScaleCount; sv += 1.) {
                            const qreal vm = getXForValue(sv*minorStep + v) + xo;
                            p.drawLine(QLineF(vm, yo, vm, yo + graduationSize/2.));
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

        p.drawLine(QLineF(xov, yov, xov, yov - h ));

        if (mShowArrow) { // the arrow is over the rectangle of heigthSize

            QPolygonF triangle (std::initializer_list<QPointF>({ QPointF(xov, yov - h),
                                                                 QPointF(xov - graduationSize*.65, yov - h + graduationSize*.65),
                                                                 QPointF(xov + graduationSize*.65, yov - h + graduationSize*.65) }));

            p.setBrush(mAxisColor);
            p.drawPolygon(triangle);

        } else {
            qreal y = yov - getYForValue(mStartVal) ;
            p.drawLine(QLineF(xov, y, xov - graduationSize, y));

             y = yov - getYForValue(mEndVal) ;
             p.drawLine(QLineF(xov, y, xov - graduationSize, y));
        }
  /*      if (!mShowText) {
            // Nothing else to draw !
        }
        else*/
        if (mMinMaxOnly) { // used on posterior densities Maybe change the type of the text exp ou float
            if (mShowText) {

                 const QString textStarVal = (valueFormatFunc ? stringForLocal(valueFormatFunc(mStartVal)) : stringForGraph(mStartVal) );
                 const QString textEndVal = (valueFormatFunc ? stringForLocal(valueFormatFunc(mEndVal)) : stringForGraph(mEndVal) );
                // const int textHeight =  fm.height() ;
                 w = qMax( fm.boundingRect(textStarVal).width(), fm.boundingRect(textEndVal).width() );
                 const int xText = int ((xov - graduationSize -w) *2.  / 3.);
                 qreal y = yov - getYForValue(mStartVal) - textHeight/2. ;
                  p.drawText(xText, y, w, textHeight, Qt::AlignRight | Qt::AlignBottom, textStarVal);

                  y = yov - getYForValue(mEndVal) - textHeight/2. ;
                  p.drawText(xText, y, w, textHeight, Qt::AlignRight | Qt::AlignVCenter, textEndVal);

            }
        } else  {
            if ( mShowSubs && (mEndVal - mStartVal != HUGE_VAL)) {
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

                    p.drawLine(QLineF(xov, y, xov - graduationSize, y));
                    linesPos.append(y);
                    ++textInc;

                    if ( textInc == mTextInc) {
                        const QString text =(valueFormatFunc ?stringForGraph(valueFormatFunc(v)) : stringForGraph(v) );
                        const int textHeight =  fm.height() ;
                        w =  fm.boundingRect(text).width();
                        const int xText = int ((xov - graduationSize -w) *2. / 3.);
                        const int yText = int (y - textHeight/2.) ;
                        p.drawText(xText, yText, w, textHeight, Qt::AlignRight | Qt::AlignBottom, text);

                        textInc = 0;

                    }

                    if (mShowSubSubs && v<mEndVal) {
                        for (qreal sv = 1.; sv < mMinorScaleCount; sv += 1.) {
                            const qreal ym = yov - getYForValue(sv*minorStep + v);

                            p.drawLine(QLineF(xov, ym, xov - graduationSize/2., ym));
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


AxisWidget::AxisWidget(DateConversion funct, QWidget* parent):QWidget(parent),
mMarginLeft(0.),
mMarginRight(0.)
{
    mFormatFunct = funct;
}

void AxisWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setFont(this->font());
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
