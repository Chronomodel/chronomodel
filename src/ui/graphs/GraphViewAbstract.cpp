#include "GraphViewAbstract.h"
#include "StdUtilities.h"
#include <cmath>
#include <QDebug>


//#pragma mark Constructor / Destructor

GraphViewAbstract::GraphViewAbstract():
mGraphWidth(50.), mGraphHeight(50),
mMarginLeft(50), mMarginRight(10), mMarginTop(5), mMarginBottom(15),
mMinX(0.), mMaxX(10.),
mMinY(0.), mMaxY(10.),
mCurrentMinX(-INFINITY),mCurrentMaxX(INFINITY)
{
//qDebug()<<"contructor GraphViewAbstract::GraphViewAbstrac ";
}

GraphViewAbstract::~GraphViewAbstract(){}

//pragma mark Getters

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


//pragma mark Setters

void GraphViewAbstract::setRangeX(const type_data aMinX, const type_data aMaxX)
{
    mMinX = aMinX;
    mMaxX = aMaxX;
}

void GraphViewAbstract::setCurrentX(const type_data aMinX, const type_data aMaxX)
{
    mCurrentMinX = aMinX;
    mCurrentMaxX = aMaxX;
   // repaintGraph(true); // not necessary
    
}

void GraphViewAbstract::setRangeY(const type_data aMinY, const type_data aMaxY)
{
    if (aMinY != mMinY || aMaxY != mMaxY) {
        if (aMinY == aMaxY) {
            mMinY = aMinY - (type_data)1.;
            mMaxY = aMaxY + (type_data)1.;
            //qDebug() << "Warning : setting min == max for graph y scale : " << aMinY;
        }
        else if (mMinY > mMaxY) {
            qDebug() << "ERROR : setting min > max for graph y scale : [" << mMinY << "; " << mMaxY << "]";
        } else {
            mMinY = aMinY;
            mMaxY = aMaxY;
        }
        repaintGraph(true);
    }
}

void GraphViewAbstract::setMinimumX(const type_data aMinX)			{ if (mMinX != aMinX) {mMinX = aMinX; /*repaintGraph(true);*/}}
void GraphViewAbstract::setMaximumX(const type_data aMaxX)			{ if (mMaxX != aMaxX) {mMaxX = aMaxX; /*repaintGraph(true);*/}}
void GraphViewAbstract::setMinimumY(const type_data aMinY)			{ if (mMinY != aMinY) {mMinY = aMinY; /*repaintGraph(true);*/}}
void GraphViewAbstract::setMaximumY(const type_data aMaxY)			{ if (mMaxY != aMaxY) {mMaxY = aMaxY; /*repaintGraph(true);*/}}

void GraphViewAbstract::setMarginLeft(const qreal aMarginLeft)		{ if (mMarginLeft != aMarginLeft) {mMarginLeft = aMarginLeft; /*repaintGraph(true);*/}}
void GraphViewAbstract::setMarginRight(const qreal aMarginRight)	{ if (mMarginRight != aMarginRight) {mMarginRight = aMarginRight; /*repaintGraph(true);*/}}
void GraphViewAbstract::setMarginTop(const qreal aMarginTop)		{ if (mMarginTop != aMarginTop) {mMarginTop = aMarginTop; /*repaintGraph(true);*/}}
void GraphViewAbstract::setMarginBottom(const qreal aMarginBottom)	{ if (mMarginBottom != aMarginBottom) {mMarginBottom = aMarginBottom; /*repaintGraph(true);*/}}
void GraphViewAbstract::setMargins(const qreal aMarginLeft, const qreal aMarginRight, const qreal aMarginTop, const qreal aMarginBottom)
{
	mMarginLeft = aMarginLeft;
	mMarginRight = aMarginRight;
	mMarginTop = aMarginTop;
	mMarginBottom = aMarginBottom;
}

//#pragma mark Values utilities
/**
 * @brief GraphViewAbstract::getXForValue find a position on a graph for a Value in a table
 * @param aValue
 * @param aConstainResult
 * @return
 */
qreal GraphViewAbstract::getXForValue(const type_data aValue, const bool aConstainResult)
{
    const qreal rigthBlank (5.); // the same name and the same value as AxisTool::updateValues()
    return (qreal)(mMarginLeft + valueForProportion(aValue, mCurrentMinX, mCurrentMaxX, (type_data)0., (type_data)(mGraphWidth - rigthBlank), aConstainResult));
}

type_data GraphViewAbstract::getValueForX(const qreal x, const bool aConstainResult)
{
    const qreal rigthBlank (5.); // the same name and the same value as AxisTool::updateValues() if AxisTool::mIsHorizontal
    const qreal lXFromSide = x - mMarginLeft;
    const type_data lValue = valueForProportion((type_data)lXFromSide, (type_data)0., (type_data) (mGraphWidth - rigthBlank), mCurrentMinX, mCurrentMaxX, aConstainResult);
	return lValue;
}

qreal GraphViewAbstract::getYForValue(const type_data aValue, const bool aConstainResult)
{
    const type_data lYFromBase = valueForProportion(aValue, mMinY, mMaxY, (type_data)(0.), (type_data)(mGraphHeight), aConstainResult);
    const qreal y = mGraphHeight+ mMarginTop - (qreal)lYFromBase;
    return y;
}

type_data GraphViewAbstract::getValueForY(const qreal y, const bool aConstainResult)
{
    const qreal lYFromBase = mMarginTop + mGraphHeight - y;
    const type_data lValue = valueForProportion( (type_data)(lYFromBase), (type_data)0., (type_data)(mGraphHeight), mMinY, mMaxY, aConstainResult);
    return lValue;
}

