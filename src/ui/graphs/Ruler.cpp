#include "Ruler.h"
#include "Painting.h"
#include <QtWidgets>
#include <iostream>


Ruler::Ruler(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mButtonsWidth(72),
mMin(0),
mMax(0),
mCurrentMin(0),
mCurrentMax(1000),
mZoomPropStep(0.1f),
mCurrentProp(1.f),
mStepMinWidth(40),
mStepWidth(100),
mYearsPerStep(100),
mShowScrollBar(true),
mShowControls(true)
{
    mScrollBarHeight = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    setMouseTracking(true);
    
    mScrollBar = new QScrollBar(Qt::Horizontal, this);
    mScrollBar->setRange(0, 0);
    mScrollBar->setSingleStep(1);
    mScrollBar->setPageStep(10000);
    mScrollBar->setTracking(true);
    
    connect(mScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setZoomPosition(int)));
}

Ruler::~Ruler()
{
    
}

void Ruler::showScrollBar(bool show)
{
    mShowScrollBar = show;
    mScrollBar->setParent(show ? this : 0);
    mScrollBar->setVisible(show);
    layout();
}

void Ruler::showControls(bool show)
{
    mShowControls = show;
    layout();
}

void Ruler::clearAreas()
{
    mAreas.clear();
    update();
}

void Ruler::addArea(float start, float end, const QColor& color)
{
    RulerArea area;
    area.mStart = start;
    area.mStop = end;
    area.mColor = color;
    mAreas.append(area);
    update();
}

void Ruler::setRange(const float min, const float max)
{
    if(mMin != min || mMax || max)
    {
        mMin = min;
        mMax = max;
        mCurrentMin = min;
        mCurrentMax = max;
        zoomDefault();
    }
}

void Ruler::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    layout();
}

void Ruler::layout()
{
    int w = width();
    int h = height();
    
    int butW = mShowControls ? mButtonsWidth : 0;
    
    if(mShowScrollBar)
    {
        mScrollBar->setGeometry(0, 0, w - butW, mScrollBarHeight);
        mButtonsRect = QRectF(w - butW, 0, butW, mScrollBarHeight);
        mRulerRect = QRectF(0, mScrollBarHeight, w, h - mScrollBarHeight);
        
        w = mButtonsRect.width()/3;
        mZoomInRect = QRectF(mButtonsRect.x() + 2*w, mButtonsRect.y(), w, mButtonsRect.height());
        mZoomOutRect = QRectF(mButtonsRect.x(), mButtonsRect.y(), w, mButtonsRect.height());
        mZoomDefaultRect = QRectF(mButtonsRect.x() + w, mButtonsRect.y(), w, mButtonsRect.height());
    }
    else
    {
        mRulerRect = QRectF(0, 0, w, h);
    }
    updateGeometry();
}

void Ruler::updateGeometry()
{
    mAxisTool.updateValues(mRulerRect.width(), mStepMinWidth, mCurrentMin, mCurrentMax);
    update();
}

void Ruler::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    
    float w = mRulerRect.width();
    float h = mRulerRect.height();
    float xo = mRulerRect.x();
    float yo = mRulerRect.y();
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QFont font;
    font.setPointSizeF(pointSize(9));
    font.setWeight(QFont::Light);
    painter.setFont(font);
    
    painter.fillRect(mRulerRect, Qt::white);
    
    for(int i=0; i<mAreas.size(); ++i)
    {
        if(mAreas[i].mStart < mCurrentMax && mAreas[i].mStop > mCurrentMin)
        {
            float x1 = w * (mAreas[i].mStart - mCurrentMin) / (mCurrentMax - mCurrentMin);
            float x2 = w;
            if(mAreas[i].mStop < mCurrentMax)
                x2 = w * (mAreas[i].mStop - mCurrentMin) / (mCurrentMax - mCurrentMin);
            
            painter.setPen(mAreas[i].mColor);
            painter.setBrush(mAreas[i].mColor);
            painter.drawRect(mRulerRect.x() + x1, mRulerRect.y(), x2 - x1, mRulerRect.height());
        }
    }

    painter.setPen(Qt::black);
    
    mAxisTool.paint(painter, mRulerRect, mStepMinWidth);
    
    // -----------------------
    
    if(mShowScrollBar)
    {
        QColor colDefault(230, 230, 230);
        QColor colHovered(250, 250, 250);
        
        painter.fillRect(mZoomInRect, mIsZoomInHovered ? colHovered : colDefault);
        painter.fillRect(mZoomOutRect, mIsZoomOutHovered ? colHovered : colDefault);
        painter.fillRect(mZoomDefaultRect, mIsZoomDefaultHovered ? colHovered : colDefault);
        
        painter.setPen(QColor(200, 200, 200));
        painter.drawRect(mZoomInRect);
        painter.drawRect(mZoomOutRect);
        painter.drawRect(mZoomDefaultRect);
        
        painter.setPen(QColor(80, 80, 80));
        painter.drawText(mZoomInRect, Qt::AlignCenter, "+");
        painter.drawText(mZoomOutRect, Qt::AlignCenter, "-");
        painter.drawText(mZoomDefaultRect, Qt::AlignCenter, "1");
    }
}

void Ruler::enterEvent(QEvent* e)
{
    Q_UNUSED(e);
}

void Ruler::leaveEvent(QEvent* e)
{
    Q_UNUSED(e);
    mIsZoomInHovered = false;
    mIsZoomOutHovered = false;
    mIsZoomDefaultHovered = false;
    update(mButtonsRect.toRect());
}

void Ruler::mouseMoveEvent(QMouseEvent* e)
{
    if(mButtonsRect.contains(e->pos()))
    {
        mIsZoomInHovered = mZoomInRect.contains(e->pos());
        mIsZoomOutHovered = mZoomOutRect.contains(e->pos());
        mIsZoomDefaultHovered = mZoomDefaultRect.contains(e->pos());
        update(mButtonsRect.toRect());
        
        setCursor(QCursor(Qt::PointingHandCursor));
    }
    else
    {
        mIsZoomInHovered = false;
        mIsZoomOutHovered = false;
        mIsZoomDefaultHovered = false;
        update(mButtonsRect.toRect());
        
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void Ruler::mousePressEvent(QMouseEvent* e)
{
    if(mZoomInRect.contains(e->pos()))
    {
        zoomIn();
    }
    else if(mZoomOutRect.contains(e->pos()))
    {
        zoomOut();
    }
    else if(mZoomDefaultRect.contains(e->pos()))
    {
        zoomDefault();
    }
}

void Ruler::zoomIn()
{
    //mCurrentProp -= mZoomPropStep;
    //mCurrentProp = (mCurrentProp <= 0.f) ? mZoomPropStep : mCurrentProp;
    
    mCurrentProp = mCurrentProp * 0.9f;
    mCurrentProp = (mCurrentProp <= 0.f) ? mZoomPropStep : mCurrentProp;
    updateZoom();
}

void Ruler::zoomOut()
{
    //mCurrentProp += mZoomPropStep;
    //mCurrentProp = (mCurrentProp > 1.f) ? 1.f : mCurrentProp;
    
    mCurrentProp = mCurrentProp * 1.1f;
    mCurrentProp = (mCurrentProp <= 0.f) ? mZoomPropStep : mCurrentProp;
    updateZoom();
}

void Ruler::zoomDefault()
{
    mCurrentProp = 1.f;
    updateZoom();
}

void Ruler::updateZoom()
{
    float posProp = 0;
    float rangeBefore = (float)mScrollBar->maximum();
    if(rangeBefore > 0)
    {
        posProp = (float)mScrollBar->value() / rangeBefore;
    }
    
    float newDelta = mCurrentProp * (mMax - mMin);
    setCurrentRange(0, newDelta);
    
    float pos = 0;
    float rangeAfter = (float)mScrollBar->maximum();
    if(rangeAfter > 0)
    {
        pos = floorf(posProp * rangeAfter);
    }
    setZoomPosition(pos);
    mScrollBar->setValue(pos);
    
    
    // ??? Should not be needed!!
    updateGeometry();
    
    /*std::cout << "newDelta : " << newDelta << std::endl;
    std::cout << "posProp : " << posProp << std::endl;
    std::cout << "rangeBefore : " << posProp << std::endl;
    std::cout << "rangeAfter : " << posProp << std::endl;
    std::cout << "pos : " << pos << std::endl;*/
}

void Ruler::setCurrentRange(const float min, const float max)
{
    if((mCurrentMin != min || mCurrentMax != max))
    {
        mCurrentMin = min;
        mCurrentMax = max;
        
        if(mCurrentMin < mMin)
        {
            mCurrentMin = mMin;
            mCurrentMax = mMin + (max - min);
            mCurrentMax = (mCurrentMax > mMax) ? mMax : mCurrentMax;
            
            //std::cout << "Corrected : mCurrentMin : " << mCurrentMin << ", mCurrentMax : " << mCurrentMax << std::endl;
        }
        else if(mCurrentMax > mMax)
        {
            mCurrentMax = mMax;
            mCurrentMin = mMax - (max - min);
            mCurrentMin = (mCurrentMin < mMin) ? mMin : mCurrentMin;
            
            //std::cout << "Corrected : mCurrentMin : " << mCurrentMin << ", mCurrentMax : " << mCurrentMax << std::endl;
        }
        updateGeometry();
        
        
        if(mCurrentProp == 1.f)
        {
            mScrollBar->setRange(0, 0);
        }
        else
        {
            int fullScrollSteps = 1000;
            int scrollSteps = (1.f - mCurrentProp) * fullScrollSteps;
            mScrollBar->setRange(0, scrollSteps);
            mScrollBar->setPageStep(mZoomPropStep * fullScrollSteps / 2);
        }
        emit zoomChanged(mCurrentMin, mCurrentMax);
    }
}

void Ruler::setZoomPosition(int pos)
{
    float range = (float)mScrollBar->maximum();
    if(range > 0)
    {
        float prop = (float)pos / range;
        float posMin = (mCurrentMax - mCurrentMin) / 2;
        float posMax = mMax - posMin;
        float newPos = posMin + prop * (posMax - posMin);
        float newMin = newPos - posMin;
        float newMax = newPos + posMin;
        
        setCurrentRange(newMin, newMax);
        
        /*std::cout << "prop : " << prop << std::endl;
        std::cout << "posMin : " << posMin << std::endl;
        std::cout << "posMax : " << posMax << std::endl;
        std::cout << "newPos : " << newPos << std::endl;
        std::cout << "newMin : " << newMin << std::endl;
        std::cout << "newMax : " << newMax << std::endl;*/
    }
}

#pragma mark Axis Tool
AxisTool::AxisTool():
mIsHorizontal(true),
mShowSubs(true),
mDeltaVal(0),
mDeltaPix(0),
mStartVal(0),
mStartPix(0)
{
    
}
void AxisTool::updateValues(float totalPix, float minDeltaPix, float minVal, float maxVal)
{
    float w = totalPix;
    w = (w <= 0.f) ? minDeltaPix : w;
    float numSteps = floor(w / minDeltaPix);
    numSteps = (numSteps <= 0) ? 1 : numSteps;
    
    float delta = maxVal - minVal;
    float unitsPerStep = delta / numSteps;
    
    float pixelsPerUnit = w / delta;
    //float unitsPerPixel = delta / w;
    
    float pow10 = 0;
    if(unitsPerStep < 1)
    {
        while(unitsPerStep < 1)
        {
            unitsPerStep *= 10;
            pow10 -= 1;
        }
    }
    else
    {
        while(unitsPerStep >= 10)
        {
            unitsPerStep /= 10;
            pow10 += 1;
        }
    }
    
    float factor = powf(10.f, pow10);
    
    //mDeltaVal = floorf(unitsPerStep) * factor;
    mDeltaVal = 10.f * factor;
    mDeltaPix = mDeltaVal * pixelsPerUnit;
    
    mStartVal = ceilf(minVal / mDeltaVal) * mDeltaVal;
    mStartPix = (mStartVal - minVal) * pixelsPerUnit;
    
    qDebug() << "------------";
     qDebug() << "w = " << w;
     qDebug() << "numSteps = " << numSteps;
     qDebug() << "delta = " << delta;
     qDebug() << "unitsPerStep = " << unitsPerStep;
     qDebug() << "pixelsPerUnit = " << pixelsPerUnit;
     qDebug() << "factor = " << factor;
     qDebug() << "---";
     qDebug() << "mStartVal = " << mStartVal;
     qDebug() << "mStartPix = " << mStartPix;
     qDebug() << "mDeltaVal = " << mDeltaVal;
     qDebug() << "mDeltaPix = " << mDeltaPix;
}

QVector<float> AxisTool::paint(QPainter& p, const QRectF& r, float textS)
{
    QVector<float> linesPos;
    
    QFont font = p.font();
    font.setPointSizeF(pointSize(9.f));
    p.setFont(font);
    p.setPen(Qt::black);
    
    if(mIsHorizontal)
    {
        float xo = r.x();
        float yo = r.y();
        float w = r.width();
        float h = r.height();
        
        int i = 0;
        for(float x = xo + mStartPix - mDeltaPix; x < xo + w; x += mDeltaPix)
        {
            if(mShowSubs)
            {
                for(float sx = x + mDeltaPix/10; sx < std::min(x + mDeltaPix, xo + w); sx += mDeltaPix/10)
                {
                    if(sx >= xo)
                        p.drawLine(QLineF(sx, yo, sx, yo + h/6));
                }
            }
            
            if(x >= xo)
            {
                p.drawLine(QLineF(x, yo, x, yo + h/3));
                
                int align = Qt::AlignCenter;
                float tx = x - textS/2;
                if(tx < xo)
                {
                    tx = xo + 2;
                    align = (Qt::AlignLeft | Qt::AlignVCenter);
                }
                else if(tx > xo + w - textS)
                {
                    tx = xo + w - textS;
                    align = (Qt::AlignRight | Qt::AlignVCenter);
                }
                QRectF tr(tx, yo + h/3, textS, 2*h/3);
                p.drawText(tr, align, QString::number(mStartVal + i * mDeltaVal));
                
                linesPos.append(x);
                ++i;
            }
        }
    }
    else
    {
        float xo = r.x() + r.width();
        float yo = r.y() + r.height();
        float w = r.width();
        float h = r.height();
        
        int i = 0;
        for(float y = yo - (mStartPix - mDeltaPix); y > yo - h; y -= mDeltaPix)
        {
            if(mShowSubs)
            {
                for(float sy = y + mDeltaPix/10; sy > std::max(y - mDeltaPix, yo - h); sy -= mDeltaPix/10)
                {
                    if(sy <= yo)
                        p.drawLine(QLineF(xo, sy, xo - 3, sy));
                }
            }
            
            if(y <= yo)
            {
                p.drawLine(QLineF(xo, y, xo - 6, y));
                
                int align = (Qt::AlignRight | Qt::AlignVCenter);
                float ty = y - textS/2;
                if(ty + textS > yo)
                {
                    ty = yo - textS;
                    align = (Qt::AlignRight | Qt::AlignBottom);
                }
                else if(ty < yo - h)
                {
                    ty = yo - h;
                    align = (Qt::AlignRight | Qt::AlignTop);
                }
                QRectF tr(xo - w, ty, w - 8, textS);
                p.drawText(tr, align, QString::number(mStartVal + i * mDeltaVal));
                
                linesPos.append(y);
                ++i;
            }
        }
    }
    return linesPos;
}