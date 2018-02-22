#include "GraphCurve.h"
#include <cmath>


GraphCurve::GraphCurve():
mPen(Qt::black, 1),
mBrush(Qt::NoBrush),
mIsHisto(true),
mIsRectFromZero(false),
mUseVectorData(false),
mIsHorizontalLine(false),
mHorizontalValue(type_data(0)),
mIsVerticalLine(false),
mVerticalValue(type_data(0)),
mIsHorizontalSections(false),
mIsTopLineSections(false),
mIsVertical(false),
mVisible(true)
{
    
}

GraphCurve::~GraphCurve()
{
    mData.clear();
    mDataVector.clear();
    mSections.clear();
}


void GraphCurve::setPen(QPen pen)
{
    mPen = pen;
}



