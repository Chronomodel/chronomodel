#ifndef AXISTOOL_H
#define AXISTOOL_H

#include <QVector>
#include <QPainter>
#include <QRectF>
#include <QWidget>
#include "StdUtilities.h"

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
    QVector<qreal> paint(QPainter &p, const QRectF &r, qreal heigthSize, FormatFunc valueFormatFunc = nullptr);

    double getMajorScale() const {return mMajorScale;}
    int getMinorScaleCount() const {return mMinorScaleCount;}
    void setMajorScale( const double &major) { mMajorScale= major;}
    void setMinorScaleCount(const int &minorCount) { mMinorScaleCount = minorCount;}
    void setScale (const double &major, const double &minorCount);
    
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
