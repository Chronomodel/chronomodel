/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "AxisTool.h"

#include "QtUtilities.h"
#include "GraphViewAbstract.h"

#include <QtWidgets>

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
    return valueForProportion(value, qreal (mStartVal), qreal (mEndVal), 0., std::max(0., mTotalPix - BLANK_SPACE_ON_RIGHT), true);
}


qreal AxisTool::getYForValue(const qreal &value)
{
    return valueForProportion(value, mStartVal, mEndVal, 0., std::max(0., qreal(mTotalPix - BLANK_SPACE_ON_TOP)), true);
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
QList<qreal> AxisTool::paint(QPainter &p, const QRectF &r, qreal graduationSize, DateConversion valueFormatFunc)
{
    QPen memoPen(p.pen());
    QBrush memoBrush(p.brush());
    QList<qreal> linesPos;

    QPen pen(p.pen());//Qt::SolidLine);
    mAxisColor = p.pen().color();
    pen.setColor(mAxisColor);
    pen.setCapStyle(Qt::SquareCap);

    p.setPen(pen);
    p.setRenderHints(QPainter::Antialiasing);
    
    QFontMetrics fm (p.font());

#ifdef Q_OS_MAC
    const int textHeight = int (1.1 * (fm.descent() + fm.ascent()) );
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
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, stringForGraph(valueFormatFunc(mStartVal)));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, stringForGraph(valueFormatFunc(mEndVal)));

                } else {
                    p.drawText(tr, Qt::AlignLeft  | Qt::AlignVCenter, stringForGraph(mStartVal));
                    p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, stringForGraph(mEndVal));
                }
            }
        }
        else {
            if ( mShowSubs && (mEndVal - mStartVal != HUGE_VAL) && (mEndVal > mStartVal) && (mMajorScale > 0)) {

                // look for the text increment
                const double valStart = valueFormatFunc ? valueFormatFunc(mStartVal) : mStartVal;
                const double valEnd = valueFormatFunc ? valueFormatFunc(mEndVal) : mEndVal;

                const double displayStartVal = ceil(valStart/ mMajorScale)*mMajorScale;
                const double displayEndVal = floor(valEnd/ mMajorScale)*mMajorScale;

                const QString textMin = stringForGraph(displayStartVal);
                const int textMinWidth =  fm.horizontalAdvance(textMin) ;

                const QString textMax = stringForGraph(displayEndVal);
                const int textMaxWidth =  fm.horizontalAdvance(textMax) ;

                const double nbPossibleText = std::abs(getXForValue(displayStartVal) - getXForValue(displayEndVal)) / (std::max(textMinWidth, textMaxWidth) + 5.);

                const double nbTheoText = std::abs(getXForValue(displayStartVal) - getXForValue(displayEndVal)) / std::abs(getXForValue(displayStartVal) - getXForValue(displayStartVal + mMajorScale));

                if (nbTheoText > nbPossibleText)
                    mTextInc = int (std::ceil(nbTheoText/nbPossibleText));
                else
                    mTextInc = 1;

                // draw scale with this text
                const double minorStep (mMajorScale/ mMinorScaleCount);

                int textInc = mTextInc - 1;
                qreal xPrev = 0.;

                const int maxCount = int(floor((displayEndVal - displayStartVal)/mMajorScale));

                // count starts at -1 to allow the drawing of minorScales
                for (int count = -1; count <= maxCount; ++count) {
                     const qreal v = displayStartVal + (count * mMajorScale);

                    if (v >= displayStartVal) {
                        const qreal x = getXForValue(v) + xo;
                        linesPos.append(x);
                        ++textInc;

                        if ( textInc == mTextInc) {
                            p.drawLine(QLineF(x, yo, x, yo + graduationSize));
                            if (mShowText) {
                                const QString text = (valueFormatFunc ? stringForGraph(valueFormatFunc(v)) : stringForGraph(v) );
                                const int textWidth =  fm.horizontalAdvance(text) ;
                                const qreal tx = x - textWidth/2.;
                                const QRectF textRect(tx, yo + h - textHeight, textWidth, textHeight);
                                p.drawText(textRect, Qt::AlignCenter ,text);
                            }

                            textInc = 0;
                            xPrev = x;

                        } else if (mShowSubSubs) {
                            p.drawLine(QLineF(x, yo, x, yo + graduationSize));

                        } else if ( (x-xPrev) > mMinDeltaPix) {
                            p.drawLine(QLineF(x, yo, x, yo + graduationSize/2.));
                            xPrev = x;
                        }

                    }
                    if (mShowSubSubs)  {
                        for (int sv = 1; sv < mMinorScaleCount; ++sv) {
                            const double vm = sv * minorStep + v;

                            if (valStart<=vm && vm<= valEnd) {
                                const qreal xm = getXForValue(vm) + xo;
                                p.drawLine(QLineF(xm, yo, xm, yo + graduationSize/2.));
                            }
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

            const QPolygonF triangle (std::initializer_list<QPointF>({ QPointF(xov, yov - h),
                                                                 QPointF(xov - graduationSize*.65, yov - h + graduationSize*.65),
                                                                 QPointF(xov + graduationSize*.65, yov - h + graduationSize*.65) }));

            p.setBrush(mAxisColor);
            p.drawPolygon(triangle);

        } else {
            qreal y = yov;
            p.drawLine(QLineF(xov, y, xov - graduationSize, y));

            y = yov - r.height() + BLANK_SPACE_ON_TOP;
            p.drawLine(QLineF(xov, y, xov - graduationSize, y));
        }

        if (mMinMaxOnly) { // used on posterior densities Maybe change the type of the text exp ou float
            if (mShowText) {

                 const QString textStarVal = (valueFormatFunc ? stringForLocal(valueFormatFunc(mStartVal)) : stringForGraph(mStartVal) );
                 const QString textEndVal = (valueFormatFunc ? stringForLocal(valueFormatFunc(mEndVal)) : stringForGraph(mEndVal) );
                 const qreal wText = qMax( fm.horizontalAdvance(textStarVal), fm.horizontalAdvance(textEndVal) );
                 const qreal xText = int ((xov - graduationSize - wText) *2.  / 3.);
                 qreal y = yov - getYForValue(mStartVal) - textHeight/2. ;
                  p.drawText(xText, y, wText, textHeight, Qt::AlignRight | Qt::AlignBottom, textStarVal);

                  y = yov - getYForValue(mEndVal) - textHeight/2. ;
                  p.drawText(xText, y, wText, textHeight, Qt::AlignRight | Qt::AlignVCenter, textEndVal);

            }
        } else  {
            if ( mShowSubs && ((mEndVal-mStartVal)/mMajorScale) != HUGE_VAL) {
                mTextInc = 1;
                // look for the text increment
                int textInc = 1;
                qreal prevTextHeight = 0.;

                const size_t maxCount = (size_t) std::min(10000., floor((mEndVal-mStartVal)/mMajorScale));

                for (size_t count = 1; count <= maxCount; ++count) {
                    const qreal v = mStartVal + (count * mMajorScale);
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

                for (size_t count = 0; count <= maxCount; ++count) {
                    const qreal v = mStartVal + (count * mMajorScale);
                    const qreal y = yov - getYForValue(v) ;

                    p.drawLine(QLineF(xov, y, xov - graduationSize, y));
                    linesPos.append(y);
                    ++textInc;

                    if ( textInc == mTextInc) {
                        const QString text = (valueFormatFunc ?stringForGraph(valueFormatFunc(v)) : stringForGraph(v) );
                        const int textHeight =  fm.height() ;
                        const int wText =  fm.horizontalAdvance(text);
                        const int xText = int ((xov - graduationSize - wText) *2. / 3.);
                        const int yText = int (y - textHeight/2.) ;
                        p.drawText(xText, yText, wText, textHeight, Qt::AlignRight | Qt::AlignBottom, text);

                        textInc = 0;

                    }

                    if (mShowSubSubs && v<mEndVal) {                  
                        for (size_t sv = 1; sv <= size_t(mMinorScaleCount); ++sv) {
                            const qreal ym = yov - getYForValue(qreal(sv)*minorStep + v);
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


AxisWidget::AxisWidget(DateConversion funct, QWidget* parent):
    QWidget(parent),
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
void Scale::findOptimal(const double a, const double b, const int nOptimal)
{
    min = std::min(a, b);
    max = std::max(a, b);

    if ( min == max) {
        min = min - 1;
        max = max + 1;
        mark = .2;
        return;
    }
    double coef = 1.;
    if (log10(abs(min))<0 && log10(abs(max))<0) {
        coef = pow(10., -std::floor(std::max(log10(abs(min)), log10(abs(max)))) +1.);
        min *= coef;
        max *= coef;
    }

    // Mark
    const double e = (max - min);
    double u = floor(log10(e));
    u = std::pow(10., u);

    const double fract[4] = { 1. , 2. , 5. , 10. };

    double diff (std::max(u, e));
    mark = diff;

   for (const double f : fract) {
        const double stp = u/f;

        if (std::abs(nOptimal - e/stp) < diff) {
            diff = nOptimal - e/stp;
            mark = stp;
        }
    }

    // Tip

    u = floor(log10(mark));
    u = std::pow(10., u);

    diff = std::max(u, mark);
    tip = fract[0];

    for (const double f : fract) {
        const double stp = u/f;

        if (std::abs(nOptimal - mark/stp) < diff) {
            diff = nOptimal - mark/stp;
            tip = f;
        }
    }


    if (min == 0.)
        min = 0.;
    else if ( std::floor(min/ mark) * mark > min)
        min = std::floor(min/ mark) * mark - mark;
    else
        min = std::floor(min/ mark) * mark;

    if (max == 0.)
        max = 0.;
    else if (min + std::ceil( (max - min) / mark ) * mark < max)
        max = min + std::ceil( (max - min) / mark ) * mark + mark;
    else
        max = min + std::ceil( (max - min) / mark ) * mark;

    if (coef != 1.) {
        min /= coef;
        max /= coef;
        mark /= coef;
        tip /= coef;
    }
}

void Scale::findOptimalMark(const double a, const double b, const int nOptimal)
{
    min = std::min(a, b);
    max = std::max(a, b);

    if ( a == b) {
        mark = .2;
        return;
    }

    const double e (max - min);
    double u = floor(log10(e));
    u = std::pow(10., u);

    const double fract[4] = { 1. , 2. , 5. , 10. };

    double diff (std::max(u, e));
    mark = diff;

    for (const double f : fract) {
        const double stp = u/f;

        if (std::abs(nOptimal - e/stp) < diff) {
            diff = nOptimal - e/stp;
            mark = stp;
        }
    }


}
