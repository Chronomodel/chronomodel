#include "GraphCurve.h"
#include <cmath>


GraphCurve::GraphCurve():
mPen(Qt::black),
mBrush(Qt::black),
mFillUnder(false),
mIsHisto(true),
mIsRectFromZero(false),
mUseVectorData(false),
mIsHorizontalLine(false),
mHorizontalValue(0),
mIsVerticalLine(false),
mVerticalValue(0),
mIsHorizontalSections(false),
mIsVertical(false),
mVisible(true)
{
    
}

GraphCurve::~GraphCurve()
{
    
}


void GraphCurve::setPen(QPen pen)
{
    mPen = pen;
}

QVector<double> GraphCurve::getVectorDataInRange(double subMin, double subMax, double min, double max) const
{
    QVector<double> subData;
    if(subMin != min || subMax != max)
    {
        int idxStart = floor(mDataVector.size() * (subMin - min) / (max - min));
        int idxEnd = floor(mDataVector.size() * (subMax - min) / (max - min));
        for(int i=idxStart; i<idxEnd; ++i)
        {
            if(i >= 0 && i < mDataVector.size())
                subData.append(mDataVector[i]);
        }
        return subData;
    }
    else
    {
        return mDataVector;
    }
}

QMap<double, double> GraphCurve::getMapDataInRange(double subMin, double subMax, double min, double max) const
{
    QMap<double, double> subData;
    if(subMin != min || subMax != max)
    {
        QMapIterator<double, double> iter(mData);
        while(iter.hasNext())
        {
            iter.next();
            double valueX = iter.key();
            if(valueX >= subMin && valueX <= subMax)
                subData[valueX] = iter.value();
        }
        return subData;
    }
    else
    {
        return mData;
    }
}
