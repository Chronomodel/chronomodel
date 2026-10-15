/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "Ruler.h"
#include "AxisTool.h"
#include "DateUtils.h"
#include <QtWidgets>


int Ruler::sHeight = 50;

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
    mMarginBottom(3)
{
    mScrollBar = new QScrollBar(Qt::Horizontal, this);
    mScrollBar->setRange(0, 0);
    mScrollBar->setSingleStep(1);
    mScrollBar->setPageStep(10000);

    connect(mScrollBar, static_cast<void (QScrollBar::*)(int)>(&QScrollBar::sliderMoved), this, &Ruler::updateScroll);
    connect(mScrollBar, static_cast<void (QScrollBar::*)(int)>(&QScrollBar::valueChanged), this, &Ruler::updateScroll);

    mAxisTool.mIsHorizontal = true;
    mAxisTool.mShowArrow = false;
    mAxisTool.mShowSubSubs = true;
    
    mAxisFont = font();

    setFixedHeight(sHeight);
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

/*
void Ruler::scrollValueChanged(double value)
{
    emit valueChanged(value);
}
*/

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


        {
            const QSignalBlocker blocker(mScrollBar);
            mScrollBar->setPageStep(int(pageStep));
            mScrollBar->setRange(0, int(scrollRange));

            mScrollBar->setValue(int(value));
        }

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
    return (100.0 - mZoomProp);
}


void Ruler::setZoom(double &prop)
{
    double minProp = 1.0 / (mMax - mMin);   //10. / (mMax - mMin);

    mZoomProp = std::max(minProp, prop /100.);

    if (mZoomProp != 1.0) {
        // Remember old scroll position
        double posProp = 0.;
        double rangeBefore = double (mScrollBar->maximum());
        if (rangeBefore > 0)
            posProp = double (mScrollBar->value()) / rangeBefore;

        // Update Scroll Range
        const int fullScrollSteps = 1000;
        int scrollSteps = int((1. - mZoomProp) * fullScrollSteps);
        mScrollBar->setRange(0, scrollSteps);
        mScrollBar->setPageStep(fullScrollSteps);

        // Set scroll to correct position
        double pos (0.);
        double rangeAfter = double (mScrollBar->maximum());
        if (rangeAfter > 0.)
            pos = floor(posProp * rangeAfter);
        mScrollBar->setValue(int(pos));


    } else
        mScrollBar->setRange(0, 0);

    update();

}

void Ruler::updateScroll()
{
    if ( (mCurrentMax - mCurrentMin) != (mMax - mMin)) {
        const double delta = mCurrentMax - mCurrentMin;
        const double deltaStart = (mMax - mMin) - delta;

        mCurrentMin = mMin + deltaStart * (double (mScrollBar->value()) / double (mScrollBar->maximum()));
        mCurrentMin = std::clamp(mCurrentMin, mMin, mMax);
        mCurrentMax = mCurrentMin + delta;

    } else {
        mCurrentMin = mMin;
        mCurrentMax = mMax;
    }

    mAxisTool.mShowSubSubs = true; // updateValues can set mShowSubSubs to false;
    mAxisTool.updateValues(int(mAxisRect.width()), int(mStepMinWidth), mCurrentMin, mCurrentMax);

    emit positionChanged(mCurrentMin, mCurrentMax);

    update();

 }

 bool Ruler::event(QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        mScrollBar->event(event);
    }
    return QWidget::event(event);
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
    
    mAxisRect = QRectF(mMarginLeft + 1, mMarginTop + scrollBarHeight, width() - mMarginLeft - mMarginRight, sHeight - (mMarginTop + scrollBarHeight));

    mScrollBar->setGeometry(mMarginLeft , 0, mAxisRect.width(), scrollBarHeight);

    mAxisTool.mShowSubSubs = true;
    mAxisTool.updateValues( int (mAxisRect.width()), int(mStepMinWidth), mCurrentMin, mCurrentMax);
    
    update();
}

void Ruler::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void Ruler::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    const double w = mAxisRect.width();

    QPainter painter(this);
    painter.setFont(mAxisFont);
    painter.setRenderHint(QPainter::Antialiasing);

    // ----------------------------------------------
    //  Areas (to display green, orange, and red areas)
    // ----------------------------------------------
    for(auto && area : mAreas) {
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

    mAxisTool.paint(painter, mAxisRect.adjusted(0, 0, 0, -mMarginBottom), -1, mFormatFuncX);
}
