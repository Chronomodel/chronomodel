/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "Ruler.h"
#include "Painting.h"
#include "AxisTool.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include <QtWidgets>
#include <iostream>


Ruler::Ruler(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mCurrentMin(0.),
mCurrentMax(1000.),
mMin(0.),
mMax(1000.),
mZoomProp(1.),
mStepMinWidth(3.),//define when minor scale can appear
mStepWidth(100),
mMarginLeft(0),
mMarginRight(0),
mMarginTop(5),
mMarginBottom(0)
{
    mScrollBar = new QScrollBar(Qt::Horizontal, this);
    mScrollBar->setRange(0, 0);
    mScrollBar->setSingleStep(1);
    mScrollBar->setPageStep(10000);
    //mScrollBar->setTracking(true);

    connect(mScrollBar, static_cast<void (QScrollBar::*)(int)>(&QScrollBar::sliderMoved), this, &Ruler::updateScroll);

    mAxisTool.mIsHorizontal = true;
    mAxisTool.mShowArrow = false;
    mAxisTool.mShowSubSubs = true;
    
    mAxisFont = font();

    setFixedHeight(50);
    setMouseTracking(true);
    updateLayout();
}

Ruler::~Ruler()
{

}

/** Copy assignment operator */
Ruler& Ruler::operator=(const Ruler & origin)
{
    realPosition = origin.realPosition;
    mCurrentMin = origin.mCurrentMin;
    mCurrentMax = origin.mCurrentMax;
    mMin = origin.mMin;
    mMax = origin.mMax;
    mZoomProp = origin.mZoomProp;

    mFormatFuncX = origin.mFormatFuncX;

    //mScrollBar = origin.mScrollBar;

    mAxisFont = origin.mAxisFont;
    mAxisRect = origin.mAxisRect;

    mStepMinWidth = origin.mStepMinWidth;
    mStepWidth = origin.mStepWidth;
    mMarginLeft = origin.mMarginLeft;
    mMarginRight = origin.mMarginRight;
    mMarginTop = origin.mMarginTop;
    mMarginBottom = origin.mMarginBottom;

    mAxisTool = origin.mAxisTool;

    mAreas = origin.mAreas;

    return *this;
}

void Ruler::setFont(const QFont &font)
{
    mMarginTop = 5;
    mAxisFont = font;

    updateLayout();
}


// Areas
void Ruler::clearAreas()
{
    mAreas.clear();
    update();
}

void Ruler::addArea(int start, int end, const QColor& color)
{
    RulerArea area;
    area.mStart = start;
    area.mStop = end;
    area.mColor = color;
    mAreas.append(area);
    update();
}

// Range & Zoom & Scroll & Current

double Ruler::getRealValue()
{
    return realPosition;
}

void Ruler::scrollValueChanged(double value)
{
    emit valueChanged(value);
}

void Ruler::setScaleDivision (const Scale &sc)
{
    mAxisTool.mShowSubSubs = true;
    mAxisTool.setScaleDivision(sc);
    update();
}

void Ruler::setScaleDivision (const double &major, const int &minorCount)
{
    mAxisTool.mShowSubSubs = true;
    mAxisTool.setScaleDivision(major, minorCount);
    update();
}

void Ruler::setRange(const double min, const double max)
{
    if (mMin != min || mMax || max) {
        mMin = min;
        mMax = max;
    }
}

void Ruler::setCurrent(const double min, const double max)
{
    mCurrentMin = min;
    mCurrentMax = max;

    // ---------------------------------------------------
    //  No zoom ! scrollbar range is max => no scrollbar shown
    // ---------------------------------------------------
    if (mCurrentMin == mMin && mCurrentMax == mMax)
        mScrollBar->setRange(0, 0);

    // ---------------------------------------------------
    //  There is a zoom => we pick a scrollSteps
    //  1000 seems fine so that dragging the scrollbar is smooth.
    //  (small value here : only allows few discrete positions)
    // ---------------------------------------------------
    else {
        const double range (1000.);
        double pageStep = range * (mCurrentMax - mCurrentMin) / (mMax - mMin);
        double scrollRange = range - pageStep;

        double curMinAtMaxScroll = mMax - (mMax - mMin) * (pageStep / range);
        double value = scrollRange * (mCurrentMin - mMin) / (curMinAtMaxScroll - mMin);

        mScrollBar->setPageStep(int(pageStep));
        mScrollBar->setRange(0, int(scrollRange));
        mScrollBar->setValue(int(value));
    }

    updateLayout();
    update();
}

void Ruler::currentChanged(const double &min, const double &max)
{
    setCurrent(min, max);

    emit positionChanged(mCurrentMin, mCurrentMax);
    update();

}
double Ruler::getZoom()
{
    return (100.-mZoomProp);
}


void Ruler::setZoom(double &prop)
{
    // Ici, 10 correspond à la différence minimale de valeur (quand le zoom est le plus fort)
    double minProp = 1. / (mMax - mMin);   //10. / (mMax - mMin);

  //  mZoomProp = (100. - prop) / 100.;
    mZoomProp = prop /100.;
    if (mZoomProp < minProp)
        mZoomProp = minProp;

    if (mZoomProp != 1.) {
        // Remember old scroll position
        double posProp = 0.;
        double rangeBefore = double (mScrollBar->maximum());
        if (rangeBefore > 0)
            posProp = double (mScrollBar->value()) / rangeBefore;

        // Update Scroll Range
        int fullScrollSteps = 1000;
        int scrollSteps = int((1. - mZoomProp) * double (fullScrollSteps));
        mScrollBar->setRange(0, scrollSteps);
        mScrollBar->setPageStep(fullScrollSteps);

        // Set scroll to correct position
        double pos (0.);
        double rangeAfter = double (mScrollBar->maximum());
        if (rangeAfter > 0.)
            pos = floor(posProp * rangeAfter);
        mScrollBar->setValue(int(pos));
       /* mScrollBar->setTracking(false);
        mScrollBar->setSliderPosition(pos);
        mScrollBar->setTracking(true);*/
    }
    else
        mScrollBar->setRange(0, 0);

    update();

}

void Ruler::updateScroll()
{
    //qDebug()<<"Ruler::updateScroll() mCurrentMin"<< mCurrentMin<<" mCurrentMax"<<mCurrentMax;
    //if(mZoomProp != 1)
    if ( (mCurrentMax - mCurrentMin) != (mMax - mMin)) {
        double delta = mCurrentMax - mCurrentMin;
        double deltaStart = (mMax - mMin)-delta;

        mCurrentMin = mMin + deltaStart * (double (mScrollBar->value()) / double (mScrollBar->maximum()));
        mCurrentMin = floor( qBound(mMin, mCurrentMin, mMax) );
        mCurrentMax = mCurrentMin + delta;

    }
    else {
        mCurrentMin = mMin;
        mCurrentMax = mMax;
    }

    mAxisTool.mShowSubSubs = true; // updateValues can set mShowSubSubs to false;
    mAxisTool.updateValues(int(mAxisRect.width()), int(mStepMinWidth), mCurrentMin, mCurrentMax);

    emit positionChanged(mCurrentMin, mCurrentMax);

    update();

 }

// Layout & Paint

/**
 * @brief Set value formatting functions
 */
void Ruler::setFormatFunctX(DateConversion f)
{
    mFormatFuncX = f;
}

void Ruler::updateLayout()
{
    int scrollBarHeight = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    mAxisRect = QRectF(mMarginLeft + 1, mMarginTop + scrollBarHeight, width() - mMarginLeft - mMarginRight, mMarginBottom);// + font().pointSizeF());

    mScrollBar->setGeometry(mMarginLeft , 0, mAxisRect.width(), scrollBarHeight);

    mAxisTool.mShowSubSubs = true;
    mAxisTool.updateValues( int (mAxisRect.width()), int(mStepMinWidth), mCurrentMin, mCurrentMax);
    
    update();
}

void Ruler::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void Ruler::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    const double w = mAxisRect.width();

    QPainter painter(this);
    painter.setFont(mAxisFont);
    painter.setRenderHint(QPainter::Antialiasing);

    /* ----------------------------------------------
     *  Areas (used to display green, orange, and red areas)
     * ----------------------------------------------
     */
    for ( auto && area : mAreas) {
        if (area.mStart < mCurrentMax && area.mStop > mCurrentMin) {
            double x1 = w * (area.mStart - mCurrentMin) / (mCurrentMax - mCurrentMin);
            x1 = (x1 < 0.) ? 0. : x1;
            double x2 = w;
            if (area.mStop < mCurrentMax)
                x2 = w * (area.mStop - mCurrentMin) / (mCurrentMax - mCurrentMin);

            painter.setPen(area.mColor);
            painter.setBrush(area.mColor);
            painter.drawRect(mAxisRect.x() + x1, mAxisRect.y(), x2 - x1, mAxisRect.height());
        }
    }

    painter.setPen(qApp->palette().text().color());

    /* ----------------------------------------------
     *    Axis, the values inside the ruler are set in layout
     *  and the size of mRulerRect are calucate in layout too.
     * ----------------------------------------------
     */

    mAxisTool.paint(painter, mAxisRect , -1, mFormatFuncX);

}
