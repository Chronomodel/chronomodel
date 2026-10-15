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

#ifndef AXISTOOL_H
#define AXISTOOL_H

#include "DateUtils.h"

#include <QVector>
#include <QPainter>
#include <QRectF>
#include <QWidget>


struct Scale
{
    double min;
    double max;
    double mark; // Major interval
    int tip ; // Minor Interval count

    Scale():min(0), max(1000), mark(100), tip(4) {}
    explicit Scale(const double a, const double b, const int nOptimal = 7) : min(a), max(b) {findOptimal(a, b, nOptimal);}
    explicit Scale(double a, double b, double m, int t = 4 ) : min(a), max(b), mark(m), tip (t) {}

    void findOptimal(double a, double b, const int nOptimal = 7);
    void findOptimalMark(const double a, const double b, const int nOptimal = 7);
};

/**
 * @brief The AxisTool class, it's just a function to draw an axis on a painter
 * It is not a widget
 */
class AxisTool
{
public:
    enum AxisSupport
    {
        eMin_Max = 'N',
        eAllTip = 'A',
        eAllways_Positive = '+',
        eAllways_Negative = '-'
    };
    AxisTool();
    void updateValues(const int totalPix, const int minDeltaPix, const qreal minVal, const qreal maxVal);
    qreal getXForValue(const qreal value);
    qreal getYForValue(const qreal value);
    QList<qreal> paint(QPainter& p, const QRectF& r, qreal graduationSize = -1, DateConversion valueFormatFunc = nullptr);

    double getMajorScale() const {return mMajorScale;}
    int getMinorScaleCount() const {return mMinorScaleCount;} // Tip
    void setMajorScale( const double major) { mMajorScale= major;} //Mark
    void setMinorScaleCount(const int minorCount) { mMinorScaleCount = minorCount;}
    void setScaleDivision (const double major, const int minorCount);
    void setScaleDivision (const Scale& sc);

public:
    bool mIsHorizontal;
    bool mShowSubs;
    bool mShowSubSubs;
    bool mShowText;
    AxisSupport mSupport;//bool mMinMaxOnly;
    bool mShowArrow;

    int mTotalPix;
    int mMinDeltaPix;

    double mStartVal;
    double mEndVal;

    double mMajorScale;
    int mMinorScaleCount;

    QColor mAxisColor;
    QString mLegend;
private:
    int mTextInc; // set the step for the scale's text
};

class AxisWidget: public QWidget, public AxisTool
{
public:
    AxisWidget(DateConversion funct = nullptr, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* e);

private:
    DateConversion mFormatFunct;

public:
    qreal mMarginLeft;
    qreal mMarginRight;
};


#endif
