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

#include "GraphViewAbstract.h"

#include <cmath>
#include <QDebug>

GraphViewAbstract::GraphViewAbstract():
    mCurrentMinX(-HUGE_VAL), mCurrentMaxX(HUGE_VAL),
    mGraphWidth(150.), mGraphHeight(50), mMarginLeft(50), mMarginRight(10),
    mMarginTop(5), mMarginBottom(15),
    mMinX(0.), mMaxX(10.),
    mMinY(0.),mMaxY(10.)
{

}

GraphViewAbstract::~GraphViewAbstract(){}

# pragma mark Getters

type_data GraphViewAbstract::rangeX() const {return mMaxX - mMinX;}
type_data GraphViewAbstract::rangeY() const {return mMaxY - mMinY;}

type_data GraphViewAbstract::getCurrentMaxX() const {return mCurrentMaxX;}
type_data GraphViewAbstract::getCurrentMinX() const {return mCurrentMinX;}


type_data GraphViewAbstract::minimumX() const {return mMinX;}
type_data GraphViewAbstract::maximumX() const {return mMaxX;}
type_data GraphViewAbstract::minimumY() const {return mMinY;}
type_data GraphViewAbstract::maximumY() const {return mMaxY;}

qreal GraphViewAbstract::marginLeft() const {return mMarginLeft;}
qreal GraphViewAbstract::marginRight() const {return mMarginRight;}
qreal GraphViewAbstract::marginTop() const {return mMarginTop;}
qreal GraphViewAbstract::marginBottom() const {return mMarginBottom;}


# pragma mark Setters -> No Action

void GraphViewAbstract::setRangeX(const type_data &aMinX, const type_data &aMaxX)
{
    mMinX = aMinX;
    mMaxX = aMaxX;
}

void GraphViewAbstract::setCurrentX(const type_data &aMinX, const type_data &aMaxX)
{
    mCurrentMinX = aMinX;
    mCurrentMaxX = aMaxX;
}

void GraphViewAbstract::setRangeY(const type_data &aMinY, const type_data &aMaxY)
{
    if (aMinY != mMinY || aMaxY != mMaxY) {
        if (aMinY == aMaxY) {
            mMinY = aMinY - type_data (1.);
            mMaxY = aMaxY + type_data (1.);
            //qDebug() << "Warning : setting min == max for graph y scale : " << aMinY;
        }
#ifdef DEBUG
        else if (mMinY > mMaxY) {
            qDebug() << " [GraphViewAbstract::setRangeY] ERROR : setting min > max for graph y scale : [" << mMinY << "; " << mMaxY << "]";
        }
#endif
        else {
            mMinY = aMinY;
            mMaxY = aMaxY;
        }
    }
}

void GraphViewAbstract::setMinimumX(const type_data &aMinX)			{ if (mMinX != aMinX) mMinX = aMinX;}
void GraphViewAbstract::setMaximumX(const type_data &aMaxX)			{ if (mMaxX != aMaxX) mMaxX = aMaxX;}
void GraphViewAbstract::setMinimumY(const type_data &aMinY)			{ if (mMinY != aMinY) mMinY = aMinY;}
void GraphViewAbstract::setMaximumY(const type_data &aMaxY)			{ if (mMaxY != aMaxY) mMaxY = aMaxY;}

void GraphViewAbstract::setMarginLeft(const qreal &aMarginLeft)		{ if (mMarginLeft != aMarginLeft) mMarginLeft = aMarginLeft;}
void GraphViewAbstract::setMarginRight(const qreal &aMarginRight)	{ if (mMarginRight != aMarginRight) mMarginRight = aMarginRight;}
void GraphViewAbstract::setMarginTop(const qreal &aMarginTop)		{ if (mMarginTop != aMarginTop) mMarginTop = aMarginTop; }
void GraphViewAbstract::setMarginBottom(const qreal &aMarginBottom)	{ if (mMarginBottom != aMarginBottom) mMarginBottom = aMarginBottom;}
void GraphViewAbstract::setMargins(const qreal &aMarginLeft, const qreal &aMarginRight, const qreal &aMarginTop, const qreal &aMarginBottom)
{
	mMarginLeft = aMarginLeft;
	mMarginRight = aMarginRight;
	mMarginTop = aMarginTop;
	mMarginBottom = aMarginBottom;
}

void GraphViewAbstract::setPrevParameter()
{
   mPrevMarginLeft = mMarginLeft;
   mPrevMarginRight =mMarginRight;
   mPrevMarginTop = mMarginTop;
   mPrevMarginBottom = mMarginBottom;
   mPrevCurrentMinX = mCurrentMinX;
   mPrevCurrentMaxX = mCurrentMaxX;
   mPrevGraphWidth = mGraphWidth;
   mPrevGraphHeight = mGraphHeight;
}

bool GraphViewAbstract::parameterChange() const
{
    bool no = (mMarginLeft == mPrevMarginLeft) && (mMarginRight == mPrevMarginRight);
    no = no && (mMarginTop == mPrevMarginTop) && (mMarginBottom == mPrevMarginBottom);
    no = no && (mCurrentMinX == mPrevCurrentMinX) && (mCurrentMaxX == mPrevCurrentMaxX);
    no = no && (mGraphWidth == mPrevGraphWidth) && (mGraphHeight == mPrevGraphHeight);

    return !no;
}
/**
 * @brief GraphViewAbstract::getXForValue find a position on a graph for a Value in a table
 * @param aValue
 * @param aConstainResult
 * @return
 */
qreal GraphViewAbstract::getXForValue(const type_data value, const bool constainResult) const
{
    return mMarginLeft + valueForProportion(value, mCurrentMinX, mCurrentMaxX, 0., std::max(0.,  mGraphWidth - BLANK_SPACE_ON_RIGHT) , constainResult);
}

type_data GraphViewAbstract::getValueForX(const qreal x, const bool constainResult) const
{
    const qreal xFromSide (x - mMarginLeft);
    return valueForProportion(type_data(xFromSide), 0., std::max(0.,  mGraphWidth - BLANK_SPACE_ON_RIGHT), mCurrentMinX, mCurrentMaxX, constainResult);
}


qreal GraphViewAbstract::getYForValue(const type_data aValue, const bool constainResult) const
{
    const type_data yFromBase = valueForProportion(aValue, mMinY, mMaxY, 0., std::max(0., type_data (mGraphHeight) - BLANK_SPACE_ON_TOP), constainResult);
    return mGraphHeight + mMarginTop - qreal(yFromBase) ;
}

type_data GraphViewAbstract::getValueForY(const qreal y, const bool constainResult) const
{
    const qreal yFromBase = mMarginTop + mGraphHeight - y;
    return valueForProportion( type_data (yFromBase), 0., std::max(0., type_data (mGraphHeight) - BLANK_SPACE_ON_TOP), mMinY, mMaxY, constainResult);
}
