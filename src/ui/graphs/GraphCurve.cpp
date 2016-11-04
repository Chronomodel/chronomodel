#include "GraphCurve.h"
#include <cmath>


GraphCurve::GraphCurve():
mPen(Qt::black),
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
    
}


void GraphCurve::setPen(QPen pen)
{
    mPen = pen;
}



