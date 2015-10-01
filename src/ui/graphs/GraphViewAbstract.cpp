#include "GraphViewAbstract.h"
#include "StdUtilities.h"
#include <cmath>
#include <QDebug>


#pragma mark Constructor / Destructor

GraphViewAbstract::GraphViewAbstract():
mGraphWidth(50), mGraphHeight(50),
mMarginLeft(50), mMarginRight(10), mMarginTop(5), mMarginBottom(15),
mMinX(0), mMaxX(10),
mMinY(0), mMaxY(10),
mCurrentMinX(0),mCurrentMaxX(2000)
{
//qDebug()<<"contructor GraphViewAbstract::GraphViewAbstrac ";
}

GraphViewAbstract::~GraphViewAbstract(){}

#pragma mark Getters

double GraphViewAbstract::rangeX() const {return mMaxX - mMinX;}
double GraphViewAbstract::rangeY() const {return mMaxY - mMinY;}

double GraphViewAbstract::getCurrentMaxX() const {return mCurrentMaxX;}
double GraphViewAbstract::getCurrentMinX() const {return mCurrentMinX;}


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
    //mCurrentMinX = aMinX;
    //mCurrentMaxX = aMaxX;
    //repaintGraph(true);
}

void GraphViewAbstract::setCurrentX(const double aMinX, const double aMaxX)
{
    //mMinX = aMinX;
    //mMaxX = aMaxX;
    mCurrentMinX = aMinX;
    mCurrentMaxX = aMaxX;
    repaintGraph(true);
    
}

void GraphViewAbstract::setRangeY(const double aMinY, const double aMaxY)
{
    if(aMinY != mMinY || aMaxY != mMaxY)
    {
        if(aMinY == aMaxY)
        {
            mMinY = aMinY - 1;
            mMaxY = aMaxY + 1;
            //qDebug() << "Warning : setting min == max for graph y scale : " << aMinY;
        }
        else if(mMinY > mMaxY)
        {
            qDebug() << "ERROR : setting min > max for graph y scale : [" << mMinY << "; " << mMaxY << "]";
        }
        else
        {
            mMinY = aMinY;
            mMaxY = aMaxY;
        }
        repaintGraph(true);
    }
}

void GraphViewAbstract::setMinimumX(const double aMinX)				{if(mMinX != aMinX){mMinX = aMinX; repaintGraph(true);}}
void GraphViewAbstract::setMaximumX(const double aMaxX)				{if(mMaxX != aMaxX){mMaxX = aMaxX; repaintGraph(true);}}
void GraphViewAbstract::setMinimumY(const double aMinY)				{if(mMinY != aMinY){mMinY = aMinY; repaintGraph(true);}}
void GraphViewAbstract::setMaximumY(const double aMaxY)				{if(mMaxY != aMaxY){mMaxY = aMaxY; repaintGraph(true);}}

void GraphViewAbstract::setMarginLeft(const int aMarginLeft)		{if(mMarginLeft != aMarginLeft){mMarginLeft = aMarginLeft; repaintGraph(true);}}
void GraphViewAbstract::setMarginRight(const int aMarginRight)		{if(mMarginRight != aMarginRight){mMarginRight = aMarginRight; repaintGraph(true);}}
void GraphViewAbstract::setMarginTop(const int aMarginTop)			{if(mMarginTop != aMarginTop){mMarginTop = aMarginTop; repaintGraph(true);}}
void GraphViewAbstract::setMarginBottom(const int aMarginBottom)	{if(mMarginBottom != aMarginBottom){mMarginBottom = aMarginBottom; repaintGraph(true);}}
void GraphViewAbstract::setMargins(const int aMarginLeft, const int aMarginRight, const int aMarginTop, const int aMarginBottom)
{
	mMarginLeft = aMarginLeft;
	mMarginRight = aMarginRight;
	mMarginTop = aMarginTop;
	mMarginBottom = aMarginBottom;
	repaintGraph(true);
}

#pragma mark Values utilities

qreal GraphViewAbstract::getXForValue(const double aValue, const bool aConstainResult)
{
	return (qreal)(mMarginLeft + valueForProportion(aValue, mCurrentMinX, mCurrentMaxX, 0.f, (double)mGraphWidth, aConstainResult));
}

qreal GraphViewAbstract::getValueForX(const double x, const bool aConstainResult)
{
	const qreal lXFromSide = x - mMarginLeft;
	const qreal lValue = valueForProportion(lXFromSide, 0.f, (double)mGraphWidth, mCurrentMinX, mCurrentMaxX, aConstainResult);
	return lValue;
}
qreal GraphViewAbstract::getYForValue(const double aValue, const bool aConstainResult)
{
	const qreal lYFromBase = valueForProportion(aValue, mMinY, mMaxY, 0.f, (double)mGraphHeight, aConstainResult);
	const qreal y = mMarginTop + mGraphHeight - lYFromBase;
	return y;	
}
qreal GraphViewAbstract::getValueForY(const double y, const bool aConstainResult)
{
	const qreal lYFromBase = mMarginTop + mGraphHeight - y;
	const qreal lValue = valueForProportion(lYFromBase, 0.f, (double)mGraphHeight, mMinY, mMaxY, aConstainResult);
	return lValue;
}

double GraphViewAbstract::valueForProportion(const double value, const double valMin, const double valMax, const double Pmin, const double Pmax, const bool resultInBounds)
{
    double v2 = Pmin + (value - valMin) * (Pmax - Pmin) / (valMax - valMin);
    //return v2;
    
	if(resultInBounds)
	{
		//v2 = (v2 > v2max) ? v2max : v2;
		//v2 = (v2 < v2min) ? v2min : v2;
        v2 = inRange(Pmin,v2,Pmax);
	}
	return v2;
}
