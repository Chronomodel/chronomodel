#include "Ruler.h"
#include "Painting.h"
#include "AxisTool.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include <QtWidgets>
#include <iostream>


Ruler::Ruler(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mCurrentMin(0.),
mCurrentMax(1000.),
mMin(0.),
mMax(1000.),
mZoomProp(1.),
mMarginLeft(20.),
mMarginRight(20.),
mStepMinWidth(3.),//define when minor scale can appear
mStepWidth(100)
{
    mScrollBarHeight = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    setFont(parentWidget()->font());
    setMouseTracking(true);
    
    mScrollBar = new QScrollBar(Qt::Horizontal, this);
    mScrollBar->setRange(0, 0);
    mScrollBar->setSingleStep(1);
    mScrollBar->setPageStep(10000);
    //mScrollBar->setTracking(true);
    
    connect(mScrollBar, static_cast<void (QScrollBar::*)(int)>(&QScrollBar::sliderMoved), this, &Ruler::updateScroll);

    mAxisTool.mIsHorizontal = true;
    mAxisTool.mShowArrow = false;
    mAxisTool.mShowSubSubs = true;
    
}

Ruler::~Ruler()
{
    
}

// Areas
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

// Range & Zoom & Scroll & Current

double Ruler::getRealValue()
{
    return realPosition;
}

void Ruler::scrollValueChanged(double value)
{
    emit valueChanged(value);
}

void Ruler::setScaleDivision (const Scale &sc)
{
    mAxisTool.mShowSubSubs = true;
    mAxisTool.setScaleDivision(sc);
    update();
}

void Ruler::setScaleDivision (const double &major, const double &minorCount)
{
    mAxisTool.mShowSubSubs = true;
    mAxisTool.setScaleDivision(major, minorCount);
    update();
}

void Ruler::setRange(const double min, const double max)
{
    if (mMin != min || mMax || max) {
        mMin = min;
        mMax = max;
    }
}

void Ruler::setCurrent(const double min, const double max)
{ 
    mCurrentMin = min;
    mCurrentMax = max;
    
    // ---------------------------------------------------
    //  No zoom ! scrollbar range is max => no scrollbar shown
    // ---------------------------------------------------
    if (mCurrentMin == mMin && mCurrentMax == mMax)
        mScrollBar->setRange(0, 0);

    // ---------------------------------------------------
    //  There is a zoom => we pick a scrollSteps
    //  1000 seems fine so that dragging the scrollbar is smooth.
    //  (small value here : only allows few discrete positions)
    // ---------------------------------------------------
    else {
        const double range (1000.);
        double pageStep = range * (mCurrentMax - mCurrentMin) / (mMax - mMin);
        double scrollRange = range - pageStep;
        
        double curMinAtMaxScroll = mMax - (mMax - mMin) * (pageStep / range);
        double value = scrollRange * (mCurrentMin - mMin) / (curMinAtMaxScroll - mMin);
        
        mScrollBar->setPageStep(pageStep);
        mScrollBar->setRange(0, int(scrollRange));
        mScrollBar->setValue(value);
    }
    
    layout();
    update();
}

void Ruler::currentChanged(const double min, const double max)
{    
    setCurrent(min, max);
    
    emit positionChanged(mCurrentMin, mCurrentMax);
    update();
    
}
double Ruler::getZoom()
{
    return (100.-mZoomProp);
}


void Ruler::setZoom(double &prop)
{
    // Ici, 10 correspond à la différence minimale de valeur (quand le zoom est le plus fort)
    double minProp = 1. / (mMax - mMin);   //10. / (mMax - mMin);
    
  //  mZoomProp = (100. - prop) / 100.;
    mZoomProp = prop /100.;
    if (mZoomProp < minProp)
        mZoomProp = minProp;
    
    if (mZoomProp != 1.) {
        // Remember old scroll position
        double posProp = 0.;
        double rangeBefore = (double)mScrollBar->maximum();
        if (rangeBefore > 0)
            posProp = (double)mScrollBar->value() / rangeBefore;
        
        // Update Scroll Range
        int fullScrollSteps = 1000;
        int scrollSteps = (1. - mZoomProp) * (double)fullScrollSteps;
        mScrollBar->setRange(0, scrollSteps);
        mScrollBar->setPageStep(fullScrollSteps);
        
        // Set scroll to correct position
        double pos = 0.;
        double rangeAfter = (double)mScrollBar->maximum();
        if (rangeAfter > 0.)
            pos = floor(posProp * rangeAfter);
        mScrollBar->setValue(pos);
       /* mScrollBar->setTracking(false);
        mScrollBar->setSliderPosition(pos);
        mScrollBar->setTracking(true);*/
    }
    else
        mScrollBar->setRange(0, 0);

    update();
    
}

void Ruler::updateScroll()
{
    //qDebug()<<"Ruler::updateScroll() mCurrentMin"<< mCurrentMin<<" mCurrentMax"<<mCurrentMax;
    //if(mZoomProp != 1)
    if ( (mCurrentMax - mCurrentMin) != (mMax - mMin)) {
        double delta = mCurrentMax - mCurrentMin;
        double deltaStart = (mMax - mMin)-delta;
        
        mCurrentMin = mMin + deltaStart * ((double)mScrollBar->value() / (double)mScrollBar->maximum());
        mCurrentMin = floor( qBound(mMin, mCurrentMin, mMax) );
        mCurrentMax = mCurrentMin + delta;
        
    }
    else {
        mCurrentMin = mMin;
        mCurrentMax = mMax;
    }

    mAxisTool.mShowSubSubs = true; // updateValues can set mShowSubSubs to false;
    mAxisTool.updateValues(mRulerRect.width(), mStepMinWidth, mCurrentMin, mCurrentMax);

    emit positionChanged(mCurrentMin, mCurrentMax);
    
    update();
    
 }

// Layout & Paint
/*
void Ruler::setFont(const QFont &font)
{
    QWidget::setFont(font);
    layout();
}
*/
/**
 * @brief Set value formatting functions
 */
void Ruler::setFormatFunctX(FormatFunc f){
    mFormatFuncX = f;
}

void Ruler::layout()
{
    //QFontMetricsF fmAxe (qApp->font());
    QFontMetricsF fmAxe (font());
    mMarginRight = floor( fmAxe.width(stringWithAppSettings(mMax))/2.);
    qreal penSize = 1.;// same value as pen.width in AxisTool

    mRulerRect = QRectF(mMarginLeft + penSize, mScrollBarHeight, width() - mMarginRight - mMarginLeft, height() - mScrollBarHeight);
    mScrollBar->setGeometry(mMarginLeft , 0., mRulerRect.width()  , mScrollBarHeight);

    mAxisTool.mShowSubSubs = true;
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
      
    /* ----------------------------------------------
     *  Areas (used to display green, orange, and red areas)
     * ----------------------------------------------
     */
    for ( auto && area : mAreas) {
        if (area.mStart < mCurrentMax && area.mStop > mCurrentMin) {
            double x1 = w * (area.mStart - mCurrentMin) / (mCurrentMax - mCurrentMin);
            x1 = (x1 < 0.) ? 0. : x1;
            double x2 = w;
            if (area.mStop < mCurrentMax)
                x2 = w * (area.mStop - mCurrentMin) / (mCurrentMax - mCurrentMin);
            
            painter.setPen(area.mColor);
            painter.setBrush(area.mColor);
            painter.drawRect(mRulerRect.x() + x1, mRulerRect.y(), x2 - x1, mRulerRect.height());
        }
    }

    painter.setPen(Qt::black);
    
    /* ----------------------------------------------
     *    Axis, the values inside the ruler are set in layout
     *  and the size of mRulerRect are calucate in layout too.
     * ----------------------------------------------
     */
    QFont f = font();
    painter.setFont(f);

    if (f.pointSizeF() >20.) {
        f.setPointSizeF(20.);
        painter.setFont(f);
    }

    const qreal heigthSize = 7.; // the same name in AxisTool and the same value as GraphView
    mAxisTool.paint(painter, mRulerRect , heigthSize, mFormatFuncX);

}

