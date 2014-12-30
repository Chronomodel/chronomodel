#include "GraphViewAbstract.h"
#include <cmath>


#pragma mark Constructor / Destructor

GraphViewAbstract::GraphViewAbstract():
mGraphWidth(50), mGraphHeight(50),
mMarginLeft(50), mMarginRight(10), mMarginTop(5), mMarginBottom(15),
mMinX(0), mMaxX(10),
mMinY(0), mMaxY(10),
mCurrentMinX(0),mCurrentMaxX(1000)
{

}

GraphViewAbstract::~GraphViewAbstract(){}

#pragma mark Getters

double GraphViewAbstract::rangeX() const {return mMaxX - mMinX;}
double GraphViewAbstract::rangeY() const {return mMaxY - mMinY;}

double GraphViewAbstract::minimumX() const {return mMinX;}
double GraphViewAbstract::maximumX() const {return mMaxX;}
double GraphViewAbstract::minimumY() const {return mMinY;}
double GraphViewAbstract::maximumY() const {return mMaxY;}

int GraphViewAbstract::marginLeft() const {return mMarginLeft;}
int GraphViewAbstract::marginRight() const {return mMarginRight;}
int GraphViewAbstract::marginTop() const {return mMarginTop;}
int GraphViewAbstract::marginBottom() const {return mMarginBottom;}


#pragma mark Setters

void GraphViewAbstract::setRangeX(const double aMinX, const double aMaxX)
{
    mMinX = aMinX;
    mMaxX = aMaxX;
    mCurrentMinX = aMinX;
    mCurrentMaxX = aMaxX;
    repaintGraph(true);
}

void GraphViewAbstract::setRangeY(const double aMinY, const double aMaxY)
{
    mMinY = aMinY;
    mMaxY = aMaxY;
    repaintGraph(true);
}

void GraphViewAbstract::setMinimumX(const double aMinX)				{mMinX = aMinX; repaintGraph(true);}
void GraphViewAbstract::setMaximumX(const double aMaxX)				{mMaxX = aMaxX; repaintGraph(true);}
void GraphViewAbstract::setMinimumY(const double aMinY)				{mMinY = aMinY; repaintGraph(true);}
void GraphViewAbstract::setMaximumY(const double aMaxY)				{mMaxY = aMaxY; repaintGraph(true);}

void GraphViewAbstract::setMarginLeft(const int aMarginLeft)		{mMarginLeft = aMarginLeft; repaintGraph(true);}
void GraphViewAbstract::setMarginRight(const int aMarginRight)		{mMarginRight = aMarginRight; repaintGraph(true);}
void GraphViewAbstract::setMarginTop(const int aMarginTop)			{mMarginTop = aMarginTop; repaintGraph(true);}
void GraphViewAbstract::setMarginBottom(const int aMarginBottom)	{mMarginBottom = aMarginBottom; repaintGraph(true);}
void GraphViewAbstract::setMargins(const int aMarginLeft, const int aMarginRight, const int aMarginTop, const int aMarginBottom)
{
	mMarginLeft = aMarginLeft;
	mMarginRight = aMarginRight;
	mMarginTop = aMarginTop;
	mMarginBottom = aMarginBottom;
	repaintGraph(true);
}

#pragma mark Values utilities

double GraphViewAbstract::getXForValue(const double aValue, const bool aConstainResult)
{
	return mMarginLeft + valueForProportion(aValue, mCurrentMinX, mCurrentMaxX, 0.f, (double)mGraphWidth, aConstainResult);
}

double GraphViewAbstract::getValueForX(const double x, const bool aConstainResult)
{
	const double lXFromSide = x - mMarginLeft;
	double lValue = valueForProportion(lXFromSide, 0.f, (double)mGraphWidth, mCurrentMinX, mCurrentMaxX, aConstainResult);
	return lValue;
}
double GraphViewAbstract::getYForValue(const double aValue, const bool aConstainResult)
{
	double lYFromBase = valueForProportion(aValue, mMinY, mMaxY, 0.f, (double)mGraphHeight, aConstainResult);
	const double y = mMarginTop + mGraphHeight - lYFromBase;
	return y;	
}
double GraphViewAbstract::getValueForY(const double y, const bool aConstainResult)
{
	const double lYFromBase = mMarginTop + mGraphHeight - y;
	double lValue = valueForProportion(lYFromBase, 0.f, (double)mGraphHeight, mMinY, mMaxY, aConstainResult);
	return lValue;
}

double GraphViewAbstract::valueForProportion(const double v1, const double v1min, const double v1max, const double v2min, const double v2max, const bool resultInBounds)
{
    double v2 = v2min + (v1 - v1min) * (v2max - v2min) / (v1max - v1min);
    return v2;
    
	if(resultInBounds)
	{
		v2 = (v2 > v2max) ? v2max : v2;
		v2 = (v2 < v2min) ? v2min : v2;
	}
	return v2;
}
