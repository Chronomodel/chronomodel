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

#ifndef AXISTOOL_H
#define AXISTOOL_H

#include "StdUtilities.h"
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

    Scale():min(0), max(1000), mark(100), tip(4){}
    explicit Scale(double n, double x, double m, int t ) : min(n), max(x), mark(m), tip (t) {}

    void findOptimal(const double &a, const double &b, const int &nOptimal);

};

/**
 * @brief The AxisTool class, it's just a function to draw an axis on a painter
 * It is not a widget
 */
class AxisTool
{
public:
    AxisTool();
    void updateValues(const int &totalPix, const int &minDeltaPix, const qreal &minVal, const qreal &maxVal);
    qreal getXForValue(const qreal &value);
    qreal getYForValue(const qreal &value);
    QVector<qreal> paint(QPainter &p, const QRectF &r, qreal graduationSize = -1, DateConversion valueFormatFunc = nullptr);

    double getMajorScale() const {return mMajorScale;}
    int getMinorScaleCount() const {return mMinorScaleCount;} // Tip
    void setMajorScale( const double &major) { mMajorScale= major;} //Mark
    void setMinorScaleCount(const int &minorCount) { mMinorScaleCount = minorCount;}
    void setScaleDivision (const double &major, const int &minorCount);
    void setScaleDivision (const Scale & sc);

public:
    bool mIsHorizontal;
    bool mShowSubs;
    bool mShowSubSubs;
    bool mShowText;
    bool mMinMaxOnly;
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

class AxisWidget: public QWidget, public AxisTool{
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
