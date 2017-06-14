
#include <qglobal.h>
#include <QPainterPath>

typedef double type_data;

class GraphViewAbstract
{
public:
    GraphViewAbstract();
    virtual ~GraphViewAbstract();
	
    QPainterPath mPainterPath;
    // Getters
    
    type_data rangeX() const;
    type_data rangeY() const;
    type_data getCurrentMaxX() const;
    type_data getCurrentMinX() const;
    
    type_data minimumX() const;
    type_data maximumX() const;
    type_data minimumY() const;
    type_data maximumY() const;
    
    qreal marginLeft() const;
    qreal marginRight() const;
    qreal marginTop() const;
    qreal marginBottom() const;
    
    // Setters
    
    virtual void setRangeX(const type_data &aMinX, const type_data &aMaxX);
    virtual void setCurrentX(const type_data &aMinX, const type_data &aMaxX);
    virtual void setRangeY(const type_data &aMinY, const type_data &aMaxY);
    
    void setMinimumX(const type_data &aMinX);
    void setMaximumX(const type_data &aMaxX);
    void setMinimumY(const type_data &aMinY);
    void setMaximumY(const type_data &aMaxY);
    
    void setMarginLeft(const qreal &aMarginLeft);
    void setMarginRight(const qreal &aMarginRight);
    void setMarginTop(const qreal &aMarginTop);
    void setMarginBottom(const qreal &aMarginBottom);
    void setMargins(const qreal &aMarginLeft, const qreal &aMarginRight, const qreal &aMarginTop, const qreal &aMarginBottom);
	
protected:
	virtual void repaintGraph(const bool aAlsoPaintBackground) = 0;
	
    virtual qreal getXForValue(const type_data &aValue, const bool &aConstainResult = true);
    virtual type_data getValueForX(const qreal &x, const bool &aConstainResult = true);
    virtual qreal getYForValue(const type_data &aValue, const bool &aConstainResult = true);
    virtual type_data getValueForY(const qreal &y, const bool &aConstainResult = true);
    
protected:
    qreal		mGraphWidth;
    qreal		mGraphHeight;
	
    qreal		mMarginLeft;
    qreal		mMarginRight;
    qreal		mMarginTop;
    qreal		mMarginBottom;
    
    type_data	mMinX;
    type_data	mMaxX;
    type_data	mMinY;
    type_data	mMaxY;
    
    type_data   mCurrentMinX;
    type_data   mCurrentMaxX;
};

template <typename T>
T valueForProportion(const T &value, const T &valMin, const T &valMax, const T &Pmin, const T &Pmax, const bool &resultInBounds)
{
    T v2 = Pmin + (value - valMin) * (Pmax - Pmin) / (valMax - valMin);

    if (resultInBounds)
        v2 = qBound(Pmin,v2,Pmax);

    return v2;
}
