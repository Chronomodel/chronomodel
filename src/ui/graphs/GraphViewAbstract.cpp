#include "GraphViewAbstract.h"
#include <cmath>


#pragma mark Constructor / Destructor

GraphViewAbstract::GraphViewAbstract():
mGraphWidth(50), mGraphHeight(50),
mMarginLeft(0), mMarginRight(0), mMarginTop(0), mMarginBottom(0),
mMinX(0), mMaxX(10),
mMinY(0), mMaxY(10),
mCurrentMinX(0),mCurrentMaxX(1000)
{

}

GraphViewAbstract::~GraphViewAbstract(){}

#pragma mark Getters

float GraphViewAbstract::rangeX() const {return mMaxX - mMinX;}
float GraphViewAbstract::rangeY() const {return mMaxY - mMinY;}

float GraphViewAbstract::minimumX() const {return mMinX;}
float GraphViewAbstract::maximumX() const {return mMaxX;}
float GraphViewAbstract::minimumY() const {return mMinY;}
float GraphViewAbstract::maximumY() const {return mMaxY;}

int GraphViewAbstract::marginLeft() const {return mMarginLeft;}
int GraphViewAbstract::marginRight() const {return mMarginRight;}
int GraphViewAbstract::marginTop() const {return mMarginTop;}
int GraphViewAbstract::marginBottom() const {return mMarginBottom;}


#pragma mark Setters

void GraphViewAbstract::setRangeX(const float aMinX, const float aMaxX)
{
    mMinX = aMinX;
    mMaxX = aMaxX;
    mCurrentMinX = aMinX;
    mCurrentMaxX = aMaxX;
    repaintGraph(true);
}

void GraphViewAbstract::setRangeY(const float aMinY, const float aMaxY)
{
    mMinY = aMinY;
    mMaxY = aMaxY;
    repaintGraph(true);
}

void GraphViewAbstract::setMinimumX(const float aMinX)				{mMinX = aMinX; repaintGraph(true);}
void GraphViewAbstract::setMaximumX(const float aMaxX)				{mMaxX = aMaxX; repaintGraph(true);}
void GraphViewAbstract::setMinimumY(const float aMinY)				{mMinY = aMinY; repaintGraph(true);}
void GraphViewAbstract::setMaximumY(const float aMaxY)				{mMaxY = aMaxY; repaintGraph(true);}

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

float GraphViewAbstract::getXForValue(const float aValue, const bool aConstainResult)
{
	return mMarginLeft + valueForProportion(aValue, mCurrentMinX, mCurrentMaxX, 0.f, (float)mGraphWidth, aConstainResult);
}

float GraphViewAbstract::getValueForX(const float x, const bool aConstainResult)
{
	const float lXFromSide = x - mMarginLeft;
	float lValue = valueForProportion(lXFromSide, 0.f, (float)mGraphWidth, mCurrentMinX, mCurrentMaxX, aConstainResult);
	return lValue;
}
float GraphViewAbstract::getYForValue(const float aValue, const bool aConstainResult)
{
	float lYFromBase = valueForProportion(aValue, mMinY, mMaxY, 0.f, (float)mGraphHeight, aConstainResult);
	const float y = mMarginTop + mGraphHeight - lYFromBase;
	return y;	
}
float GraphViewAbstract::getValueForY(const float y, const bool aConstainResult)
{
	const float lYFromBase = mMarginTop + mGraphHeight - y;
	float lValue = valueForProportion(lYFromBase, 0.f, (float)mGraphHeight, mMinY, mMaxY, aConstainResult);
	return lValue;
}

float GraphViewAbstract::valueForProportion(const float v1, const float v1min, const float v1max, const float v2min, const float v2max, const bool resultInBounds)
{
    float v2 = v2min + (v1 - v1min) * (v2max - v2min) / (v1max - v1min);
    return v2;
    
	if(resultInBounds)
	{
		v2 = (v2 > v2max) ? v2max : v2;
		v2 = (v2 < v2min) ? v2min : v2;
	}
	return v2;
}
