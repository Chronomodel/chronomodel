#ifndef AXISTOOL_H
#define AXISTOOL_H

#include <QVector>
#include <QPainter>
#include <QRectF>


class AxisTool
{
public:
    AxisTool();
    void updateValues(double totalPix, double minDeltaPix, double minVal, double maxVal);
    QVector<qreal> paint(QPainter& p, const QRectF& r, qreal heigthSize);
    
public:
    bool mIsHorizontal;
    bool mShowSubs;
    bool mShowSubSubs;
    bool mShowText;
    bool mMinMaxOnly;
    bool mShowArrow;
    bool mShowDate;
    
    double mDeltaVal;
    double mDeltaPix;
    double mStartVal;
    double mEndVal;
    double mStartPix;
    double mPixelsPerUnit;
    QFont    mfont;
    
    QColor mAxisColor;
};

#endif
