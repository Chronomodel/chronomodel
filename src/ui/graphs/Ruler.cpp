#include "Ruler.h"
#include "Painting.h"
#include "AxisTool.h"
#include <QtWidgets>
#include <iostream>


Ruler::Ruler(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMin(0),
mMax(0),
mCurrentMin(0),
mCurrentMax(1000),
mStepMinWidth(40),
mStepWidth(100)
{
    mScrollBarHeight = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    setMouseTracking(true);
    
    mScrollBar = new QScrollBar(Qt::Horizontal, this);
    mScrollBar->setRange(0, 0);
    mScrollBar->setSingleStep(1);
    mScrollBar->setPageStep(10000);
    mScrollBar->setTracking(true);
    
    connect(mScrollBar, SIGNAL(valueChanged(int)), this, SLOT(updateScroll()));
}

Ruler::~Ruler()
{
    
}

#pragma mark Areas
void Ruler::clearAreas()
{
    mAreas.clear();
    update();
}

void Ruler::addArea(double start, double end, const QColor& color)
{
    RulerArea area;
    area.mStart = start;
    area.mStop = end;
    area.mColor = color;
    mAreas.append(area);
    update();
}

#pragma mark Range & Zoom & Scroll
void Ruler::setRange(const double min, const double max)
{
    if(mMin != min || mMax || max)
    {
        mMin = min;
        mMax = max;
        mCurrentMin = min;
        mCurrentMax = max;
        updateScroll();
    }
}
void Ruler::setZoom(int prop)
{
    // Ici, 10 correspond à la différence minimale de valeur (quand le zoom est le plus fort)
    double minProp = 10 / (mMax - mMin);
    
    mZoomProp = (100. - prop) / 100.;
    if(mZoomProp < minProp) mZoomProp = minProp;
    
    if(mZoomProp != 1)
    {
        // Remember old scroll position
        double posProp = 0;
        double rangeBefore = (double)mScrollBar->maximum();
        if(rangeBefore > 0)
            posProp = (double)mScrollBar->value() / rangeBefore;
        
        // Update Scroll Range
        int fullScrollSteps = 1000;
        int scrollSteps = (1.f - mZoomProp) * fullScrollSteps;
        mScrollBar->setRange(0, scrollSteps);
        mScrollBar->setPageStep(fullScrollSteps);
        
        // Set scroll to correct position
        double pos = 0;
        double rangeAfter = (double)mScrollBar->maximum();
        if(rangeAfter > 0)
            pos = floorf(posProp * rangeAfter);
        mScrollBar->setValue(pos);
    }
    else
    {
        mScrollBar->setRange(0, 0);
    }
    updateScroll();
}

void Ruler::updateScroll()
{
    if(mZoomProp != 1)
    {
        double delta = mZoomProp * (mMax - mMin);
        double deltaStart = (mMax - mMin) - delta;
        
        mCurrentMin = mMin + deltaStart * ((double)mScrollBar->value() / (double)mScrollBar->maximum());
        mCurrentMax = mCurrentMin + delta;
    }
    mAxisTool.updateValues(mRulerRect.width(), mStepMinWidth, mCurrentMin, mCurrentMax);
    
    emit positionChanged(mCurrentMin, mCurrentMax);
    
    update();
}

#pragma mark Layout & Paint
void Ruler::layout()
{
    int w = width();
    int h = height();
    
    mScrollBar->setGeometry(0, 0, w, mScrollBarHeight);
    mRulerRect = QRectF(0, mScrollBarHeight, w, h - mScrollBarHeight);
    
    mAxisTool.updateValues(mRulerRect.width(), mStepMinWidth, mCurrentMin, mCurrentMax);
    update();
}

void Ruler::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    layout();
}

void Ruler::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    
    double w = mRulerRect.width();
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.fillRect(mRulerRect, Qt::white);
    
    // ----------------------------------------------
    //  Areas
    // ----------------------------------------------
    
    for(int i=0; i<mAreas.size(); ++i)
    {
        if(mAreas[i].mStart < mCurrentMax && mAreas[i].mStop > mCurrentMin)
        {
            double x1 = w * (mAreas[i].mStart - mCurrentMin) / (mCurrentMax - mCurrentMin);
            double x2 = w;
            if(mAreas[i].mStop < mCurrentMax)
                x2 = w * (mAreas[i].mStop - mCurrentMin) / (mCurrentMax - mCurrentMin);
            
            painter.setPen(mAreas[i].mColor);
            painter.setBrush(mAreas[i].mColor);
            painter.drawRect(mRulerRect.x() + x1, mRulerRect.y(), x2 - x1, mRulerRect.height());
        }
    }

    painter.setPen(Qt::black);
    
    // ----------------------------------------------
    //  Axis
    // ----------------------------------------------
    
    mAxisTool.paint(painter, mRulerRect, mStepMinWidth);
}

