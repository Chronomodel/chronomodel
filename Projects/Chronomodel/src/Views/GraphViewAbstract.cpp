#include "GraphViewAbstract.h"
#include <cmath>


template <class Type>
Type valueForProportion(const Type aValue1, const Type aMin1, const Type aMax1, const Type aMin2, const Type aMax2, const bool aResultInBounds = true)
{
	const Type lDiff1 = aMax1 - aMin1;
	const Type lDiff2 = aMax2 - aMin2;
	Type lValue2 = aMin2 + (aValue1 - aMin1) * lDiff2 / lDiff1;
	if(aResultInBounds)
	{
		lValue2 = (lValue2 > aMax2) ? aMax2 : lValue2;
		lValue2 = (lValue2 < aMin2) ? aMin2 : lValue2;	
	}
	return lValue2;
}

#pragma mark Constructor / Destructor

GraphViewAbstract::GraphViewAbstract():
mXDisplayMode(eControl_Linear),mYDisplayMode(eControl_Linear),
mXUnit(eControl_Generic),mYUnit(eControl_Generic),
mGraphWidth(50), mGraphHeight(50),
mMarginLeft(50), mMarginRight(30), mMarginTop(30), mMarginBottom(30),
mMinX(0), mMaxX(10),
mMinY(0), mMaxY(10),
mXFactor(1.f),mYFactor(1.f),
mXPrecision(2),mYPrecision(2),
mXNumTicks(3),mXNumSubTicks(4),
mXShowTicks(true),mXShowSubTicks(true),mXShowTicksValues(true),mXShowSubTicksValues(false),
mYNumTicks(3),mYNumSubTicks(4),
mYShowTicks(true),mYShowSubTicks(true),mYShowTicksValues(true),mYShowSubTicksValues(false),
mXValueMargin(4),mYValueMargin(4),
mTitle("")
{

}

GraphViewAbstract::~GraphViewAbstract(){}

#pragma mark Getters

Control_DisplayMode GraphViewAbstract::displayModeX() const	{return mXDisplayMode;}
Control_DisplayMode GraphViewAbstract::displayModeY() const	{return mYDisplayMode;}
Control_Unit GraphViewAbstract::unitX() const					{return mXUnit;}
Control_Unit GraphViewAbstract::unitY() const					{return mYUnit;}
float GraphViewAbstract::rangeX() const						{return mMaxX - mMinX;}
float GraphViewAbstract::rangeY() const						{return mMaxY - mMinY;}
float GraphViewAbstract::minimumX() const						{return mMinX;}
float GraphViewAbstract::maximumX() const						{return mMaxX;}
float GraphViewAbstract::minimumY() const						{return mMinY;}
float GraphViewAbstract::maximumY() const						{return mMaxY;}
int GraphViewAbstract::marginLeft() const						{return mMarginLeft;}
int GraphViewAbstract::marginRight() const						{return mMarginRight;}
int GraphViewAbstract::marginTop() const						{return mMarginTop;}
int GraphViewAbstract::marginBottom() const					{return mMarginBottom;}


#pragma mark Setters

void GraphViewAbstract::setXDisplayMode(const Control_DisplayMode aMode)
{
	mXDisplayMode = aMode; 
	if(mXDisplayMode == eControl_Logarithmic)
	{
		mMinX = (mMinX < 0.00000001f) ? 0.00000001f : mMinX;
		mMaxX = (mMaxX < 0.00000001f) ? 0.00000001f : mMaxX;
	}
	repaintGraph(true);
}
void GraphViewAbstract::setYDisplayMode(const Control_DisplayMode aMode)
{
	mYDisplayMode = aMode;
	if(mYDisplayMode == eControl_Logarithmic)
	{
		mMinY = (mMinY < 0.00000001f) ? 0.00000001f : mMinY;
		mMaxY = (mMaxY < 0.00000001f) ? 0.00000001f : mMaxY;
	}
	repaintGraph(true);
}
void GraphViewAbstract::setRangeX(const float aMinX, const float aMaxX)	{mMinX = aMinX; mMaxX = aMaxX; repaintGraph(true);}
void GraphViewAbstract::setRangeY(const float aMinY, const float aMaxY)	{mMinY = aMinY; mMaxY = aMaxY; repaintGraph(true);}
void GraphViewAbstract::setXUnit(const Control_Unit aUnit)					{mXUnit = aUnit; repaintGraph(true);}
void GraphViewAbstract::setYUnit(const Control_Unit aUnit)					{mYUnit = aUnit; repaintGraph(true);}
void GraphViewAbstract::setMinimumX(const float aMinX)						{mMinX = aMinX; repaintGraph(true);}
void GraphViewAbstract::setMaximumX(const float aMaxX)						{mMaxX = aMaxX; repaintGraph(true);}
void GraphViewAbstract::setMinimumY(const float aMinY)						{mMinY = aMinY; repaintGraph(true);}
void GraphViewAbstract::setMaximumY(const float aMaxY)						{mMaxY = aMaxY; repaintGraph(true);}
void GraphViewAbstract::setXDisplayFactor(const float aFactor)				{mXFactor = aFactor;}
void GraphViewAbstract::setYDisplayFactor(const float aFactor)				{mYFactor = aFactor;}
void GraphViewAbstract::setXPrecision(const int aPrecision)				{mXPrecision = aPrecision;}
void GraphViewAbstract::setYPrecision(const int aPrecision)				{mYPrecision = aPrecision;}
void GraphViewAbstract::setMarginLeft(const int aMarginLeft)				{mMarginLeft = aMarginLeft; repaintGraph(true);}
void GraphViewAbstract::setMarginRight(const int aMarginRight)				{mMarginRight = aMarginRight; repaintGraph(true);}
void GraphViewAbstract::setMarginTop(const int aMarginTop)					{mMarginTop = aMarginTop; repaintGraph(true);}
void GraphViewAbstract::setMarginBottom(const int aMarginBottom)			{mMarginBottom = aMarginBottom; repaintGraph(true);}
void GraphViewAbstract::setMargins(const int aMarginLeft, const int aMarginRight, const int aMarginTop, const int aMarginBottom)
{
	mMarginLeft = aMarginLeft;
	mMarginRight = aMarginRight;
	mMarginTop = aMarginTop;
	mMarginBottom = aMarginBottom;
	repaintGraph(true);
}

void GraphViewAbstract::setXNumTicks(const int aNumTicks)				{mXNumTicks = aNumTicks; repaintGraph(true);}
void GraphViewAbstract::setXNumSubTicks(const int aNumSubTicks)		{mXNumSubTicks = aNumSubTicks; repaintGraph(true);}
void GraphViewAbstract::setXShowTicksValues(const bool aShow)			{mXShowTicksValues = aShow; repaintGraph(true);}
void GraphViewAbstract::setXShowSubTicksValues(const bool aShow)		{mXShowSubTicksValues = aShow; repaintGraph(true);}
void GraphViewAbstract::setXShowTicks(const bool aShow)				{mXShowTicks = aShow; repaintGraph(true);}
void GraphViewAbstract::setXShowSubTicks(const bool aShow)				{mXShowSubTicks = aShow; repaintGraph(true);}

void GraphViewAbstract::setYNumTicks(const int aNumTicks)				{mYNumTicks = aNumTicks; repaintGraph(true);}
void GraphViewAbstract::setYNumSubTicks(const int aNumSubTicks)		{mYNumSubTicks = aNumSubTicks; repaintGraph(true);}
void GraphViewAbstract::setYShowTicksValues(const bool aShow)			{mYShowTicksValues = aShow; repaintGraph(true);}
void GraphViewAbstract::setYShowSubTicksValues(const bool aShow)		{mYShowSubTicksValues = aShow; repaintGraph(true);}
void GraphViewAbstract::setYShowTicks(const bool aShow)				{mYShowTicks = aShow; repaintGraph(true);}
void GraphViewAbstract::setYShowSubTicks(const bool aShow)				{mYShowSubTicks = aShow; repaintGraph(true);}

void GraphViewAbstract::setXValueMargin(const int aMargin)				{mXValueMargin = aMargin; repaintGraph(true);}
void GraphViewAbstract::setYValueMargin(const int aMargin)				{mYValueMargin = aMargin; repaintGraph(true);}

#pragma mark Values utilities

float GraphViewAbstract::getXForValue(const float aValue, const bool aConstainResult)
{
	float lXFromSide = 0.f;
	if(mXDisplayMode == eControl_Linear)
	{
		lXFromSide = valueForProportion(aValue, mMinX, mMaxX, 0.f, (float)mGraphWidth, aConstainResult);
	}
	else if(mXDisplayMode == eControl_Logarithmic)
	{
		// The minimum value of the axis must be > 0 as the log argument must be > 0 !
        //Q_ASSERT(mMinX > 0.f)
        //Q_ASSERT(aValue >= mMinX)
		
		lXFromSide = (float)mGraphWidth * logf(aValue/mMinX) / logf(mMaxX/mMinX);
		if(aConstainResult)
		{
			lXFromSide = (lXFromSide < 0.f) ? 0.f : lXFromSide;
			lXFromSide = (lXFromSide > mGraphWidth) ? mGraphWidth : lXFromSide;
		}
	}
	const float x = mMarginLeft + lXFromSide;
	return x;
}
float GraphViewAbstract::getValueForX(const float x, const bool aConstainResult)
{
	const float lXFromSide = x - mMarginLeft;
	float lValue = mMinX;
	
	if(mXDisplayMode == eControl_Linear)
	{
		lValue = valueForProportion(lXFromSide, 0.f, (float)mGraphWidth, mMinX, mMaxX, aConstainResult);
	}
	else if(mXDisplayMode == eControl_Logarithmic)
	{
		lValue = mMinX * powf(2.f, ((lXFromSide/(float)mGraphWidth) * logf(mMaxX/mMinX)/logf(2.f)));
		if(aConstainResult)
		{
			lValue = (lValue < mMinX) ? mMinX : lValue;
			lValue = (lValue > mMaxX) ? mMaxX : lValue;
		}
	}
	return lValue;
}
float GraphViewAbstract::getYForValue(const float aValue, const bool aConstainResult)
{
	float lYFromBase = 0.f;
	
	if(mYDisplayMode == eControl_Linear)			
	{
		lYFromBase = valueForProportion(aValue, mMinY, mMaxY, 0.f, (float)mGraphHeight, aConstainResult);
	}
	else if(mYDisplayMode == eControl_Logarithmic)
	{
		const float lValue = (aValue < 0.00000001f) ? 0.00000001f : aValue;
		lYFromBase = (float)mGraphHeight * logf(lValue/mMinY) / logf(mMaxY/mMinY);
		if(aConstainResult)
		{
			lYFromBase = (lYFromBase < 0.f) ? 0.f : lYFromBase;
			lYFromBase = (lYFromBase > mGraphHeight) ? mGraphHeight : lYFromBase;
		}
	}
	const float y = mMarginTop + mGraphHeight - lYFromBase;
	return y;	
}
float GraphViewAbstract::getValueForY(const float y, const bool aConstainResult)
{
	const float lYFromBase = mMarginTop + mGraphHeight - y;
	float lValue = mMinY;
	
	if(mYDisplayMode == eControl_Linear)
	{
		lValue = valueForProportion(lYFromBase, 0.f, (float)mGraphHeight, mMinY, mMaxY, aConstainResult);
	}
	else if(mYDisplayMode == eControl_Logarithmic)
	{
		lValue = mMinY * powf(2.f, ((lYFromBase/(float)mGraphHeight) * logf(mMaxY/mMinY)/logf(2.f)));
		if(aConstainResult)
		{
			lValue = (lValue < mMinY) ? mMinY : lValue;
			lValue = (lValue > mMaxY) ? mMaxY : lValue;
		}
	}
	return lValue;
}
