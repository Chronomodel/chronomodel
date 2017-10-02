#ifndef AXISTOOL_H
#define AXISTOOL_H

#include <QVector>
#include <QPainter>
#include <QRectF>
#include <QWidget>
#include "StdUtilities.h"

struct Scale
{
    double min;
    double max;
    double mark; // Major interval
    int tip ; // Minor Interval count

    Scale():min(0), max(1000), mark(100), tip(4){}
    explicit Scale(double n, double x, double m, int t ) : min(n), max(x), mark(m), tip (t) {}

    void findOptimal(const double &a, const double &b, const int &nOptimal);

};

/**
 * @brief The AxisTool class, it's just a function to draw an axis on a painter
 * It is not a widget
 */
class AxisTool
{
public:
    AxisTool();
    void updateValues(const int &totalPix, const int &minDeltaPix, const qreal &minVal, const qreal &maxVal);
    qreal getXForValue(const qreal &value);
    qreal getYForValue(const qreal &value);
    QVector<qreal> paint(QPainter &p, const QRectF &r, qreal heigthSize, FormatFunc valueFormatFunc = nullptr);

    double getMajorScale() const {return mMajorScale;}
    int getMinorScaleCount() const {return mMinorScaleCount;} // Tip
    void setMajorScale( const double &major) { mMajorScale= major;} //Mark
    void setMinorScaleCount(const int &minorCount) { mMinorScaleCount = minorCount;}
    void setScaleDivision (const double &major, const double &minorCount);
    void setScaleDivision (const Scale & sc);
    
public:
    bool mIsHorizontal;
    bool mShowSubs;
    bool mShowSubSubs;
    bool mShowText;
    bool mMinMaxOnly;
    bool mShowArrow;
    
    int mTotalPix;
    int mMinDeltaPix;

    double mStartVal;
    double mEndVal;

    double mMajorScale;
    int mMinorScaleCount;
    
    QColor mAxisColor;
    QString mLegend;
private:
    int mTextInc; // set the step for the scale's text
};

class AxisWidget: public QWidget, public AxisTool{
public:
    AxisWidget(FormatFunc funct = 0, QWidget* parent = nullptr);
    
protected:
    void paintEvent(QPaintEvent* e);
    
private:
    FormatFunc mFormatFunct;
    
public:
    qreal mMarginLeft;
    qreal mMarginRight;
};


#endif
