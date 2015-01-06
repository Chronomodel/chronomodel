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
    QVector<double> paint(QPainter& p, const QRectF& r, double textW);
    
public:
    bool mIsHorizontal;
    bool mShowSubs;
    bool mMinMaxOnly;
    double mDeltaVal;
    double mDeltaPix;
    double mStartVal;
    double mStartPix;
};

#endif
