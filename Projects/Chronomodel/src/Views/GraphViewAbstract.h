#pragma once
#include "GraphProperties.h"

class GraphViewAbstract
{
public:
    GraphViewAbstract();
    virtual ~GraphViewAbstract();
	
    Control_DisplayMode displayModeX() const;
    Control_DisplayMode displayModeY() const;
    Control_Unit unitX() const;
    Control_Unit unitY() const;
    float rangeX() const;
    float rangeY() const;
    float minimumX() const;
    float maximumX() const;
    float minimumY() const;
    float maximumY() const;
    int marginLeft() const;
    int marginRight() const;
    int marginTop() const;
    int marginBottom() const;
	
	void setXDisplayMode(const Control_DisplayMode aMode);
	void setYDisplayMode(const Control_DisplayMode aMode);
	void setXUnit(const Control_Unit aUnit);
	void setYUnit(const Control_Unit aUnit);
	void setRangeX(const float aMinX, const float aMaxX);
	void setRangeY(const float aMinY, const float aMaxY);
	void setMinimumX(const float aMinX);
	void setMaximumX(const float aMaxX);
	void setMinimumY(const float aMinY);
	void setMaximumY(const float aMaxY);
	void setXDisplayFactor(const float aFactor);
	void setYDisplayFactor(const float aFactor);
	void setXPrecision(const int aPrecision);
	void setYPrecision(const int aPrecision);
	void setMarginLeft(const int aMarginLeft);
	void setMarginRight(const int aMarginRight);
	void setMarginTop(const int aMarginTop);
	void setMarginBottom(const int aMarginBottom);
	void setMargins(const int aMarginLeft, const int aMarginRight, const int aMarginTop, const int aMarginBottom);
	
	void setXNumTicks(const int aNumTicks);
	void setXNumSubTicks(const int aNumSubTicks);
	void setXShowTicksValues(const bool aShow);
	void setXShowSubTicksValues(const bool aShow);
	void setXShowTicks(const bool aShow);
	void setXShowSubTicks(const bool aShow);
	
	void setYNumTicks(const int aNumTicks);
	void setYNumSubTicks(const int aNumSubTicks);
	void setYShowTicksValues(const bool aShow);
	void setYShowSubTicksValues(const bool aShow);
	void setYShowTicks(const bool aShow);
	void setYShowSubTicks(const bool aShow);
	
	void setXValueMargin(const int aMargin);
	void setYValueMargin(const int aMargin);
	
protected:
	virtual void repaintGraph(const bool aAlsoPaintBackground) = 0;
	
    virtual float getXForValue(const float aValue, const bool aConstainResult = true);
    virtual float getValueForX(const float x, const bool aConstainResult = true);
    virtual float getYForValue(const float aValue, const bool aConstainResult = true);
    virtual float getValueForY(const float y, const bool aConstainResult = true);
	
protected:
	Control_DisplayMode	mXDisplayMode;
	Control_DisplayMode	mYDisplayMode;
	Control_Unit		mXUnit;
	Control_Unit		mYUnit;
	
	int		mGraphWidth;
	int		mGraphHeight;
	
	int		mMarginLeft;
	int		mMarginRight;
	int		mMarginTop;
	int		mMarginBottom;
	
	float	mMinX;
	float	mMaxX;
	float	mMinY;
	float	mMaxY;
	
	float	mXFactor;
	float	mYFactor;

	int		mXPrecision;
	int		mYPrecision;
	
	int		mXNumTicks;
	int		mXNumSubTicks;
	bool	mXShowTicks;
	bool	mXShowSubTicks;
	bool	mXShowTicksValues;
	bool	mXShowSubTicksValues;
	
	int		mYNumTicks;
	int		mYNumSubTicks;
	bool	mYShowTicks;
	bool	mYShowSubTicks;
	bool	mYShowTicksValues;
	bool	mYShowSubTicksValues;
	
	int	mXValueMargin;
	int	mYValueMargin;
	
	const char*	mTitle;
};

