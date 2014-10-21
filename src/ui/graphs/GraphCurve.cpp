#include "GraphCurve.h"


GraphCurve::GraphCurve():
mUseVectorData(false),
mPen(Qt::black),
mFillUnder(false),
mIsHisto(true),
mIsHorizontalLine(false),
mHorizontalValue(0),
mIsVerticalLine(false),
mVerticalValue(0),
mIsVertical(false)
{
    
}

GraphCurve::~GraphCurve()
{
    
}
