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
    
    double rangeX() const;
    double rangeY() const;
    double getCurrentMaxX() const;
    double getCurrentMinX() const;
    
    double minimumX() const;
    double maximumX() const;
    double minimumY() const;
    double maximumY() const;
    
    int marginLeft() const;
    int marginRight() const;
    int marginTop() const;
    int marginBottom() const;
    
    // Setters
    
    virtual void setRangeX(const double aMinX, const double aMaxX);
    virtual void setCurrentX(const double aMinX, const double aMaxX);
    virtual void setRangeY(const double aMinY, const double aMaxY);
    
	void setMinimumX(const double aMinX);
	void setMaximumX(const double aMaxX);
	void setMinimumY(const double aMinY);
    void setMaximumY(const double aMaxY);
    
    void setMarginLeft(const int aMarginLeft);
	void setMarginRight(const int aMarginRight);
	void setMarginTop(const int aMarginTop);
	void setMarginBottom(const int aMarginBottom);
	void setMargins(const int aMarginLeft, const int aMarginRight, const int aMarginTop, const int aMarginBottom);
	
protected:
	virtual void repaintGraph(const bool aAlsoPaintBackground) = 0;
	
    virtual double getXForValue(const double aValue, const bool aConstainResult = true);
    virtual double getValueForX(const double x, const bool aConstainResult = true);
    virtual double getYForValue(const double aValue, const bool aConstainResult = true);
    virtual double getValueForY(const double y, const bool aConstainResult = true);
    
    double valueForProportion(const double v1, const double valMin, const double valMax, const double Pmin, const double Pmax, const bool resultInBounds = true);
	
protected:
	int		mGraphWidth;
	int		mGraphHeight;
	
    qreal		mMarginLeft;
    qreal		mMarginRight;
    qreal		mMarginTop;
    qreal		mMarginBottom;
    
	double	mMinX;
	double	mMaxX;
	double	mMinY;
	double	mMaxY;
    
    double   mCurrentMinX;
    double   mCurrentMaxX;
};

