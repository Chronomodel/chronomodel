#pragma once


class GraphViewAbstract
{
public:
    GraphViewAbstract();
    virtual ~GraphViewAbstract();
	
    // Getters
    
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
    
    // Setters
    
    virtual void setRangeX(const float aMinX, const float aMaxX);
    virtual void setRangeY(const float aMinY, const float aMaxY);
    
	void setMinimumX(const float aMinX);
	void setMaximumX(const float aMaxX);
	void setMinimumY(const float aMinY);
    void setMaximumY(const float aMaxY);
    
    void setMarginLeft(const int aMarginLeft);
	void setMarginRight(const int aMarginRight);
	void setMarginTop(const int aMarginTop);
	void setMarginBottom(const int aMarginBottom);
	void setMargins(const int aMarginLeft, const int aMarginRight, const int aMarginTop, const int aMarginBottom);
	
protected:
	virtual void repaintGraph(const bool aAlsoPaintBackground, const bool aAlsoPaintGraphs = true) = 0;
	
    virtual float getXForValue(const float aValue, const bool aConstainResult = true);
    virtual float getValueForX(const float x, const bool aConstainResult = true);
    virtual float getYForValue(const float aValue, const bool aConstainResult = true);
    virtual float getValueForY(const float y, const bool aConstainResult = true);
    
    float valueForProportion(const float v1, const float v1min, const float v1max, const float v2min, const float m2max, const bool resultInBounds = true);
	
protected:
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
    
    float   mCurrentMinX;
    float   mCurrentMaxX;
};

