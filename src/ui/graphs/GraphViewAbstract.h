/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#ifndef GRAPHVIEWABSTRACT_H
#define GRAPHVIEWABSTRACT_H

#include <QWidget>
#include <qglobal.h>
#include <QPainterPath>

#define BLANK_SPACE_ON_TOP 5. //Space used to draw the credibility bar
#define BLANK_SPACE_ON_RIGHT 5.

typedef double type_data;

class GraphViewAbstract:public QWidget
{
    Q_OBJECT

public:
    GraphViewAbstract();
    virtual ~GraphViewAbstract();

#pragma mark Getters
    bool parameterChange() const;
    type_data rangeX() const;
    type_data rangeY() const;
    type_data getCurrentMaxX() const;
    type_data getCurrentMinX() const;

    type_data minimumX() const;
    type_data maximumX() const;
    type_data minimumY() const;
    type_data maximumY() const;

    qreal marginLeft() const;
    qreal marginRight() const;
    qreal marginTop() const;
    qreal marginBottom() const;

 #pragma mark Setters
    void setParent(QWidget *parent) {this->QWidget::setParent(parent);}

    void setPrevParameter();

    virtual void setRangeX(const type_data &aMinX, const type_data &aMaxX);
    virtual void setCurrentX(const type_data &aMinX, const type_data &aMaxX);
    virtual void setRangeY(const type_data &aMinY, const type_data &aMaxY);

    void setMinimumX(const type_data &aMinX);
    void setMaximumX(const type_data &aMaxX);
    void setMinimumY(const type_data &aMinY);
    void setMaximumY(const type_data &aMaxY);

    void setGraphHeight(const qreal h) {mGraphHeight = h;};

    void setMarginLeft(const qreal &aMarginLeft);
    void setMarginRight(const qreal &aMarginRight);
    void setMarginTop(const qreal &aMarginTop);
    void setMarginBottom(const qreal &aMarginBottom);
    void setMargins(const qreal &aMarginLeft, const qreal &aMarginRight, const qreal &aMarginTop, const qreal &aMarginBottom);

protected:
	virtual void repaintGraph(const bool aAlsoPaintBackground) = 0;

    virtual qreal getXForValue(const type_data aValue, const bool aConstainResult = true) const;
    virtual type_data getValueForX(const qreal x, const bool aConstainResult = true) const;
    virtual qreal getYForValue(const type_data aValue, const bool aConstainResult = true) const;
    virtual type_data getValueForY(const qreal y, const bool aConstainResult = true) const;

protected:
    QPainterPath mPainterPath;

    type_data   mCurrentMinX;
    type_data   mCurrentMaxX;

    qreal		mGraphWidth;
    qreal		mGraphHeight;

    qreal		mMarginLeft;
    qreal		mMarginRight;
    qreal		mMarginTop;
    qreal		mMarginBottom;

    type_data	mMinX;
    type_data	mMaxX;
    type_data	mMinY;
    type_data	mMaxY;


    // previous parameter
    qreal		mPrevGraphWidth;
    qreal		mPrevGraphHeight;

    qreal		mPrevMarginLeft;
    qreal		mPrevMarginRight;
    qreal		mPrevMarginTop;
    qreal		mPrevMarginBottom;

    type_data   mPrevCurrentMinX;
    type_data   mPrevCurrentMaxX;

};

template <typename T>
inline T valueForProportion(const T &value, const T &valMin, const T &valMax, const T &Pmin, const T &Pmax, const bool &resultInBounds)
{
    T v2 = Pmin + (value - valMin) * (Pmax - Pmin) / (valMax - valMin);

    if (resultInBounds)
        return std::clamp(Pmin, v2, Pmax);

    return v2;
}


#endif
