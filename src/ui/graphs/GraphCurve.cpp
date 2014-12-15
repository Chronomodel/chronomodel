#include "GraphCurve.h"


GraphCurve::GraphCurve():
mPen(Qt::black),
mFillUnder(false),
mIsHisto(true),
mIsRectFromZero(false),
mUseVectorData(false),
mIsHorizontalLine(false),
mHorizontalValue(0),
mIsVerticalLine(false),
mVerticalValue(0),
mIsHorizontalSections(false),
mIsVertical(false)
{
    
}

GraphCurve::~GraphCurve()
{
    
}
