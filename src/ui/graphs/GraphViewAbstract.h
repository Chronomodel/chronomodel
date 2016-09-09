#pragma once

#include <qglobal.h>
#include <QPainterPath>

class GraphViewAbstract
{
public:
    GraphViewAbstract();
    virtual ~GraphViewAbstract();
	
    QPainterPath mPainterPath;
    // Getters
    
    float rangeX() const;
    float rangeY() const;
    float getCurrentMaxX() const;
    float getCurrentMinX() const;
    
    float minimumX() const;
    float maximumX() const;
    float minimumY() const;
    float maximumY() const;
    
    qreal marginLeft() const;
    qreal marginRight() const;
    qreal marginTop() const;
    qreal marginBottom() const;
    
    // Setters
    
    virtual void setRangeX(const float aMinX, const float aMaxX);
    virtual void setCurrentX(const float aMinX, const float aMaxX);
    virtual void setRangeY(const float aMinY, const float aMaxY);
    
    void setMinimumX(const float aMinX);
    void setMaximumX(const float aMaxX);
    void setMinimumY(const float aMinY);
    void setMaximumY(const float aMaxY);
    
    void setMarginLeft(const qreal aMarginLeft);
    void setMarginRight(const qreal aMarginRight);
    void setMarginTop(const qreal aMarginTop);
    void setMarginBottom(const qreal aMarginBottom);
    void setMargins(const qreal aMarginLeft, const qreal aMarginRight, const qreal aMarginTop, const qreal aMarginBottom);
	
protected:
	virtual void repaintGraph(const bool aAlsoPaintBackground) = 0;
	
    virtual qreal getXForValue(const float aValue, const bool aConstainResult = true);
    virtual float getValueForX(const qreal x, const bool aConstainResult = true);
    virtual qreal getYForValue(const float aValue, const bool aConstainResult = true);
    virtual float getValueForY(const qreal y, const bool aConstainResult = true);
    
    float valueForProportion(const float v1, const float valMin, const float valMax, const float Pmin, const float Pmax, const bool resultInBounds = true);
    qreal valueForProportion(const qreal v1, const qreal valMin, const qreal valMax, const qreal Pmin, const qreal Pmax, const bool resultInBounds = true);
	
protected:
    qreal		mGraphWidth;
    qreal		mGraphHeight;
	
    qreal		mMarginLeft;
    qreal		mMarginRight;
    qreal		mMarginTop;
    qreal		mMarginBottom;
    
    float	mMinX;
    float	mMaxX;
    float	mMinY;
    float	mMaxY;
    
    float   mCurrentMinX;
    float   mCurrentMaxX;
};

