#include "GraphView.h"
#include "Ruler.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>
#include <algorithm>
#include <QtSvg>

class ProjectSettings;

#pragma mark Constructor / Destructor

GraphView::GraphView(QWidget *parent):QWidget(parent),
mStepMinWidth(50), // define secondary scale on axis
mRendering(eSD),
mShowAxisArrows(true),
mShowAxisLines(true),
mShowVertGrid(true),
mShowHorizGrid(true),
mXAxisMode(eAllTicks),
mYAxisMode(eAllTicks),
mAutoAdjustYScale(false),
mFormatFuncX(0),
mFormatFuncY(0),
mShowInfos(false),
mBackgroundColor(Qt::white),
mThickness(1),
mTipX(0.),
mTipY(0.),
mTipWidth(100.),
mTipHeight(40.),
mTipVisible(false),
mUseTip(true)
{
    mAxisToolX.mIsHorizontal = true;
    mAxisToolX.mShowArrow = true;
    mAxisToolY.mIsHorizontal = false;
    mAxisToolX.mShowArrow = true;
    mAxisToolY.mShowSubs = false;
    
    mTipRect.setTop(0);
    mTipRect.setLeft(0);
    mTipRect.setWidth(mTipWidth);
    mTipRect.setHeight(mTipHeight);
    
    setMouseTracking(true);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    
    setRangeY(0.f, 1.f);
    this->mAxisToolX.updateValues(width(), mStepMinWidth, mCurrentMinX, mCurrentMaxX);
    resetNothingMessage();
  
}

GraphView::~GraphView(){}

// ------------------------------------------------------
//  Zoom X
// ------------------------------------------------------
#pragma mark Zoom X
void GraphView::zoomX(const double min, const double max)
{
    if(mCurrentMinX != min || mCurrentMaxX || max)
    {
        mCurrentMinX = min;
        mCurrentMaxX = max;
        
        this->mAxisToolX.updateValues(width(), 10, min, max);
        if(mAutoAdjustYScale)
        {
            double yMax = -100000000;
            double yMin =  100000000;
            for(int curveIndex=0; curveIndex<mCurves.size(); ++curveIndex)
            {
                const GraphCurve& curve = mCurves[curveIndex];
                if(curve.mIsHorizontalLine)
                {
                    yMax = qMax(yMax, curve.mHorizontalValue);
                    yMin = qMin(yMin, curve.mHorizontalValue);
                }
                else if(curve.mUseVectorData)
                {
                    QVector<double> subData = curve.getVectorDataInRange(mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);
                    yMax = qMax(yMax, vector_max_value(subData));
                    yMin = qMin(yMin, vector_min_value(subData));
                }
                else if(!curve.mIsVertical && !curve.mIsVerticalLine && !curve.mIsHorizontalSections)
                {
                    QMap<double, double> subData = curve.getMapDataInRange(mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);
                    yMax = qMax(yMax, map_max_value(subData));
                    yMin = qMin(yMin, map_min_value(subData));
                }
            }
            if(yMax > yMin)
                setRangeY(yMin, yMax);
        }
        repaintGraph(true);
    }
      // ici ca marche  qDebug()<<"apres GraphView::zoomX"<<mCurrentMaxX<<" max"<<max;
     //if (mCurrentMaxX!= 1000) qDebug()<<"-----------in GraphView::zoomX mCurrentMaxX"<<mCurrentMaxX;
    update();
    
}

// ------------------------------------------------------
//  Options
// ------------------------------------------------------
#pragma mark Options

void GraphView::setBackgroundColor(const QColor& aColor)
{
    mBackgroundColor = aColor;
    repaintGraph(true);
}

QColor GraphView::getBackgroundColor() const
{
    return mBackgroundColor;
}

void GraphView::addInfo(const QString& info)
{
    mInfos << info;
    if(mShowInfos)
        repaintGraph(false);
}

void GraphView::clearInfos()
{
    mInfos.clear();
    if(mShowInfos)
        repaintGraph(false);
}

void GraphView::showInfos(bool show)
{
    mShowInfos = show;
    repaintGraph(true);
}

void GraphView::setNothingMessage(const QString& message)
{
    mNothingMessage = message;
    repaintGraph(true);
}

void GraphView::resetNothingMessage()
{
    mNothingMessage = tr("Nothing to display");
    repaintGraph(true);
}

void GraphView::setRendering(GraphView::Rendering render)
{
    mRendering = render;
    repaintGraph(true);
}

GraphView::Rendering GraphView::getRendering()
{
    return mRendering;
}


void GraphView::showAxisArrows(bool show)
{
    if(mShowAxisArrows != show)
    {
        mShowAxisArrows = show;
        repaintGraph(true);
    }
}

void GraphView::showAxisLines(bool show)
{
    if(mShowAxisLines != show)
    {
        mShowAxisLines = show;
        repaintGraph(true);
    }
}

void GraphView::showVertGrid(bool show)
{
    if(mShowVertGrid != show)
    {
        mShowVertGrid = show;
        repaintGraph(true);
    }
}

void GraphView::showHorizGrid(bool show)
{
    if(mShowHorizGrid != show)
    {
        mShowHorizGrid = show;
        repaintGraph(true);
    }
}

void GraphView::setXAxisMode(AxisMode mode)
{
    if(mXAxisMode != mode)
    {
        mXAxisMode = mode;
        mAxisToolX.mMinMaxOnly = (mXAxisMode == eMinMax);
        repaintGraph(true);
    }
}

void GraphView::setYAxisMode(AxisMode mode)
{
    if(mYAxisMode != mode)
    {
        mYAxisMode = mode;
        mAxisToolY.mMinMaxOnly = (mYAxisMode == eMinMax);
        repaintGraph(true);
    }
}

void GraphView::autoAdjustYScale(bool active)
{
    mAutoAdjustYScale = active;
    repaintGraph(true);
}

void GraphView::adjustYToMaxValue()
{
    double yMax = 0;
    for(int i=0; i<mCurves.size(); ++i){
        if(mCurves[i].mVisible)
        {
            if(!mCurves[i].mUseVectorData &&
               !mCurves[i].mIsHorizontalLine &&
               !mCurves[i].mIsHorizontalSections &&
               !mCurves[i].mIsVerticalLine &&
               !mCurves[i].mIsVertical){
                yMax = qMax(yMax, map_max_value(mCurves[i].mData));
            }else if(mCurves[i].mUseVectorData){
                yMax = qMax(yMax, vector_max_value(mCurves[i].mDataVector));
            }
        }
    }
    setRangeY(0, yMax);
}

void GraphView::setGraphFont(const QFont& font)
{
    setFont(font);
    repaintGraph(true);
}

void GraphView::setCurvesThickness(int value)
{
    mThickness = value;
    repaintGraph(true);
}

void GraphView::setFormatFunctX(FormatFunc f){
    mFormatFuncX = f;
}

void GraphView::setFormatFunctY(FormatFunc f){
    mFormatFuncY = f;
}

/* ------------------------------------------------------
 Curves & Zones
 ------------------------------------------------------ */
#pragma mark Curves & Zones

void GraphView::addCurve(const GraphCurve& curve)
{
    mCurves.append(curve);
    repaintGraph(false);
}

void GraphView::removeCurve(const QString& name)
{
    for(int i=0; i<mCurves.size(); ++i)
    {
        if(mCurves[i].mName == name)
        {
            mCurves.removeAt(i);
            break;
        }
    }
    repaintGraph(false);
}

void GraphView::removeAllCurves()
{
    mCurves.clear();
    repaintGraph(false);
}

void GraphView::setCurveVisible(const QString& name, const bool visible)
{
    bool modified = false;
    for(int i=0; i<mCurves.size(); ++i)
    {
        if(mCurves[i].mName == name && mCurves[i].mVisible != visible){
            mCurves[i].mVisible = visible;
            modified = true;
            break;
        }
    }
    if(modified){
        repaintGraph(false);
    }
}

GraphCurve* GraphView::getCurve(const QString& name)
{
    for(int i=0; i<mCurves.size(); ++i)
    {
        if(mCurves[i].mName == name)
            return &mCurves[i];
    }
    return 0;
}

const QList<GraphCurve>& GraphView::getCurves() const{
    return mCurves;
}

int GraphView::numCurves() const
{
    return mCurves.size();
}

void GraphView::addZone(const GraphZone& zone)
{
    mZones.append(zone);
    repaintGraph(false);
}

void GraphView::removeAllZones()
{
    mZones.clear();
    repaintGraph(false);
}

#pragma mark Mouse events & Tool Tip
void GraphView::enterEvent(QEvent* e)
{
    Q_UNUSED(e);
    mTipVisible = mUseTip;
    update();
}

void GraphView::leaveEvent(QEvent* e)
{
    Q_UNUSED(e);
    mTipVisible = false;
    update();
}

void GraphView::mouseMoveEvent(QMouseEvent* e)
{
    double x = e->pos().x();
    double y = e->pos().y();
    //std::cout << y << std::endl;
    
    if(mUseTip && x >= mMarginLeft && x <= (mMarginLeft + mGraphWidth) && y >= mMarginTop && y <= (mMarginTop + mGraphHeight))
    {
        mTipVisible = true;
        QRectF old_rect = mTipRect;
        
        int cursorW = 15;
        int cursorH = 15;
        
        if(mMarginLeft + mGraphWidth - x <= (mTipWidth + cursorW))
            x -= mTipWidth;
        else
            x += cursorW;
        
        if(mMarginTop + mGraphHeight - y <= (mTipHeight + cursorH))
            y -= mTipHeight;
        else
            y += cursorH;
        
        mTipRect.setLeft(x - 0.5);
        mTipRect.setTop(y - 0.5);
        
        mTipRect.setWidth(mTipWidth);
        mTipRect.setHeight(mTipHeight);
        
        mTipX = getValueForX(e->x()-0.5);
        if(mFormatFuncX)
            mTipX = mFormatFuncX(mTipX).toDouble();
        
        mTipY = getValueForY(e->y()+0.5);
        if(mFormatFuncY)
            mTipY = mFormatFuncY(mTipY).toDouble();
        
        update(old_rect.adjusted(-30, -30, 30, 30).toRect());
    }
    else
    {
        mTipVisible = false;
    }
    update(mTipRect.adjusted(-30, -30, 30, 30).toRect());
    
    // forward to parent to move a marker for example
    e->ignore();
}

#pragma mark Resize & Paint
/* void GraphView::repaintGraph(const bool aAlsoPaintBackground)
{
    if(aAlsoPaintBackground)
    {
        mBufferBack = QPixmap();
    }
        if (mCurrentMaxX!= 1000) qDebug()<<"in GraphView::repaintGraph mCurrentMaxX"<<mCurrentMaxX;
    //update();
   
} */

void GraphView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    repaintGraph(true);
}

void GraphView::updateGraphSize(int w, int h)
{
    mGraphWidth = w - mMarginLeft - mMarginRight;
    mGraphHeight = h - mMarginTop - mMarginBottom;
    mAxisToolX.updateValues(mGraphWidth, mStepMinWidth, mCurrentMinX, mCurrentMaxX);
    mAxisToolY.updateValues(mGraphHeight, 12, mMinY, mMaxY);
}

void GraphView::repaintGraph(const bool aAlsoPaintBackground)
{
    if(aAlsoPaintBackground)
    {
        mBufferBack = QPixmap();
    }
    update();
}
    
void GraphView::paintEvent(QPaintEvent* )
{
 //   Q_UNUSED(event);
    
    
    updateGraphSize(width(), height());
    
    // ----------------------------------------------------
    //  Nothing to draw !
    // ----------------------------------------------------
    if(mCurves.size() == 0)
    {
        QPainter p(this);
        p.setFont(font());
        p.fillRect(0, 0, width(), height(), QColor(200, 200, 200));
        p.setPen(QColor(100, 100, 100));
        p.drawText(0, 0, width(), height(), Qt::AlignCenter, mNothingMessage);
        
        return;
    }
    
    // ----------------------------------------------------
    //  SD : draw on a buffer only if it has been reset
    // ----------------------------------------------------
    if(mBufferBack.isNull() && mRendering == eSD)
    {
        mBufferBack = QPixmap(width(), height());
        paintToDevice(&mBufferBack);
        if (mBufferBack.isNull() ) {
            qDebug()<< "mBufferBack.isNull()";

        }
    }
    // ----------------------------------------------------
    //  HD : draw directly on widget
    // ----------------------------------------------------
    else if(mRendering == eHD)
    {
        paintToDevice(this);
    }
    // ----------------------------------------------------
    //  SD rendering : draw buffer on widget !
    // ----------------------------------------------------
    if(mRendering == eSD)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing);
        p.drawPixmap(mBufferBack.rect(), mBufferBack, rect());
        
    }
    
    // ----------------------------------------------------
    //  Tool Tip (above all) Draw horizontal and vertical red line
    // ----------------------------------------------------
    if(mTipVisible)
    {
        QPainterPath tipPath;
        if (mTipRect.width()<2) {
            mTipRect.setLeft(20);
            mTipRect.setTop(20);
            
            mTipRect.setWidth(20);
            mTipRect.setHeight(20);
        }
      
        
        tipPath.addRoundedRect(mTipRect, 5, 5);
        
        
        QFont font;
        font.setPointSizeF(pointSize(9));
        
        QPainter p(this);
        p.setFont(font);
        
        p.fillPath(tipPath, QColor(0, 0, 0, 180));
        p.setPen(Qt::black);
        p.drawPath(tipPath);
        p.setPen(Qt::white);
        p.drawText(mTipRect.adjusted(0, 0, 0, (int)(-mTipRect.height()/2) ), Qt::AlignCenter, QString("x : ") + QString::number(mTipX,'f',3) );
        p.drawText(mTipRect.adjusted(0, (int)(mTipRect.height()/2), 0, 0), Qt::AlignCenter, QString("y : ") + QString::number(mTipY));
       
    }
}






/**
 * @brief draw graphics
 */
void GraphView::paintToDevice(QPaintDevice* device)
{
    
    QPainter p(device);    
    p.setFont(font());

    p.setRenderHints(QPainter::Antialiasing);
    
    // ----------------------------------------------------
    //  Background
    // ----------------------------------------------------
    p.fillRect(rect(), mBackgroundColor);
    
       
    QFont font = p.font();
    font.setPointSizeF(font.pointSizeF() - 2.);
    p.setFont(font);
   
    // ----------------------------------------------------
    //  Curves
    // ----------------------------------------------------
    drawCurves(p);
    
    // ----------------------------------------------------
    //  Vertical Grid
    // ----------------------------------------------------
    if(mXAxisMode != eHidden)
    {
        if(!mLegendX.isEmpty()) {
            //QRectF tr(0, mGraphHeight, mMarginLeft, mMarginBottom);
            
            QRectF tr(mMarginLeft, mGraphHeight- mMarginBottom, mGraphWidth, mMarginBottom);
            p.setPen(Qt::black);
            p.drawText(tr, Qt::AlignRight | Qt::AlignTop, mLegendX);
        }
        
        mAxisToolX.mShowText    = true;
        mAxisToolX.mShowSubs    = true;
        mAxisToolX.mShowSubSubs = true;
        mAxisToolX.mShowArrow   = true;
        
        mAxisToolX.updateValues(mGraphWidth, mStepMinWidth, mCurrentMinX, mCurrentMaxX);
        QVector<qreal> linesXPos = mAxisToolX.paint(p, QRectF(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth , mMarginBottom), 7, mFormatFuncX);
        
        if(mShowVertGrid)
        {
            p.setPen(QColor(0, 0, 0, 20));
            for(int i=0; i<linesXPos.size(); ++i)
            {
                qreal x = linesXPos[i];
                p.drawLine(x, mMarginTop, x, mMarginTop + mGraphHeight);
            }
        }
    }
    else {
        mAxisToolX.mShowText = false;
        mAxisToolX.mShowSubs = true;
        mAxisToolX.mShowSubSubs = false;
        mAxisToolX.mShowArrow = true;
        mAxisToolX.paint(p, QRectF(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth , mMarginBottom), 7, mFormatFuncX);
    }
   
    // ----------------------------------------------------
    //  Horizontal Grid
    // ----------------------------------------------------
    if(mYAxisMode != eHidden)
    {
        mAxisToolY.mShowText = true;
        mAxisToolY.mShowSubs = true;
        mAxisToolY.mShowSubSubs = true;
        mAxisToolY.mShowArrow = true;
        mAxisToolY.updateValues(mGraphHeight, mStepMinWidth, mMinY, mMaxY);
        QVector<qreal> linesYPos = mAxisToolY.paint(p, QRectF(0, mMarginTop, mMarginLeft, mGraphHeight), 5, mFormatFuncY);
        
        if(mShowHorizGrid)
        {
            p.setPen(QColor(0, 0, 0, 20));
            for(int i=0; i<linesYPos.size(); ++i)
            {
                double y = linesYPos[i];
                p.drawLine(mMarginLeft, y, mMarginLeft + mGraphWidth, y);
            }
        }
    }
    
    font.setPointSizeF(font.pointSizeF() + 2.);
    p.setFont(font);
    
    
    
    // ----------------------------------------------------
    //  Graph specific infos at the top right
    // ----------------------------------------------------
    if(mShowInfos)
    {
        p.setPen(QColor(50, 50, 50));
        int y = 0;
        int lineH = 16;
        for(int i=0; i<mInfos.size(); ++i){
            p.drawText(mMarginLeft + 5, mMarginTop + 5 + y, mGraphWidth - 10, lineH, Qt::AlignRight | Qt::AlignTop, mInfos[i]);
            y += lineH;
        }
    }

}

void GraphView::drawCurves(QPainter& painter)
{
     //if (mCurrentMaxX < 900) qDebug()<<" in GraphView::drawCurves mCurrentMinX"<<mCurrentMinX<<"mCurrentMaxX"<<mCurrentMaxX;
    // ---------------------------------------------------------------
    //  Curves
    // ---------------------------------------------------------------
    for(int curveIndex=0; curveIndex<mCurves.size(); ++curveIndex)
    {
        const GraphCurve& curve = mCurves[curveIndex];
        if(curve.mVisible)
        {
            QPainterPath path;
            
            QPen pen = curve.mPen;
            pen.setWidth(pen.width() * mThickness);
            painter.setPen(pen);
            
            painter.setBrush(curve.mBrush);
            
            if(curve.mIsHorizontalLine)
            {
                qreal y = getYForValue(curve.mHorizontalValue);
                path.moveTo(mMarginLeft, y);
                path.lineTo(mMarginLeft + mGraphWidth, y);
                painter.strokePath(path, curve.mPen);
            }
            else if(curve.mIsVerticalLine)
            {
                qreal x = getXForValue(curve.mVerticalValue);
                path.moveTo(x, mMarginTop + mGraphHeight);
                path.lineTo(x, mMarginTop);
                painter.strokePath(path, curve.mPen);
            }
            else if(curve.mIsHorizontalSections)
            {
                qreal y = mMarginTop;
                path.moveTo(mMarginLeft, y);
                for(int i=0; i<curve.mSections.size(); ++i)
                {
                    qreal x1 = getXForValue(curve.mSections[i].first);
                    x1 = qMax(x1, mMarginLeft);
                    x1 = qMin(x1,mMarginLeft + mGraphWidth);
                    
                    qreal x2 = getXForValue(curve.mSections[i].second);
                    x2 = qMax(x2,mMarginLeft);
                    x2 = qMin(x2, mMarginLeft + mGraphWidth);
                    
                    path.moveTo(x1, y);
                    path.lineTo(x2, y);
                }
                path.moveTo(mMarginLeft + mGraphWidth, y);
                painter.strokePath(path, curve.mPen);
            }
            else if(curve.mIsVertical)
            {
                path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);
                
                int index = 0;
                //double last_x = 0;
                qreal last_y = 0;
                
                QMapIterator<double, double> iter(curve.mData);
                while(iter.hasNext())
                {
                    iter.next();
                    
                    qreal valueX = iter.value();
                    qreal valueY = iter.key();
                    
                    // vertical curves must be normalized (values from 0 to 1)
                    // They are drawn using a 100px width
                    qreal x = mMarginLeft + valueX * 100;
                    qreal y = getYForValue(valueY, false);
                    y = qMin(y, mMarginTop + mGraphHeight);
                    y = qMax(y, mMarginTop);
                    
                    //qDebug() << valueY << ", " << valueX << " => " << y << ", " << x;
                    
                    if(index == 0)
                    {
                        path.moveTo(x, y);
                    }
                    else
                    {
                        if(curve.mIsHisto)
                            path.lineTo(x, last_y);
                        path.lineTo(x, y);
                    }
                    //last_x = x;
                    last_y = y;
                    ++index;
                }
                painter.drawPath(path);
            }
            else
            {
                path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);
                
                int index = 0;
                qreal last_x = 0;
                qreal last_y = 0;
                qreal last_value_y = 0;
                //double last_value_x = 0;
                
                if(curve.mUseVectorData)
                {
                    /*for(int x=0; x<curve.mDataVector.size(); ++x)
                     {
                     double valueX = x;
                     double valueY = curve.mDataVector[x];
                     
                     if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                     {
                     double x = getXForValue(valueX, false);
                     double y = getYForValue(valueY, false);
                     
                     if(index == 0)
                     {
                     path.moveTo(x, y);
                     }
                     else
                     {
                     if(curve.mIsHisto)
                     path.lineTo(x, last_y);
                     path.lineTo(x, y);
                     }
                     last_x = x;
                     last_y = y;
                     last_value_x = valueX;
                     last_value_y = valueY;
                     ++index;
                     }
                     }*/
                    
                    // Down sample vector
                    
                    QVector<double> subData = curve.getVectorDataInRange(mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);
                    
                    QVector<double> lightData;
                    double dataStep = (double)subData.size() / (double)(2.*mGraphWidth);
                    if(dataStep > 1)
                    {
                        for(int i=0; i<2*mGraphWidth; ++i)
                        {
                            
                            /* double dataMean=0;
                             for(int j=i*dataStep; (j<(i+1)*dataStep) and (j<subData.size());j++)
                             {
                             dataMean+=subData[j];
                             }
                             dataMean=dataMean/dataStep;
                             lightData.append(dataMean);*/
                            int idx = (int)round(i * dataStep);
                            lightData.append(subData[idx]);
                        }
                    }
                    else
                    {
                        lightData = subData;
                    }
                    /*qDebug() << "-----------------------";
                     qDebug() << "dataStep : " << dataStep;
                     qDebug() << "data : " << curve.mDataVector.size();
                     qDebug() << "subData : " << subData.size();
                     qDebug() << "lightData : " << lightData.size();*/
                    
                    
                    for(int i=0; i<lightData.size(); ++i)
                    {
                        // Use "dataStep" only if lightData is different of subData !
                        double valueX = mCurrentMinX + ((dataStep > 1) ? i * dataStep : i);
                        double valueY = lightData[i];
                        
                        if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                        {
                            qreal x = getXForValue(valueX, false);
                            qreal y = getYForValue(valueY, false);
                            
                            if(index == 0)
                            {
                                path.moveTo(x, y);
                            }
                            else
                            {
                                if(curve.mIsHisto)
                                    path.lineTo(x, last_y);
                                path.lineTo(x, y);
                            }
                            last_x = x;
                            last_y = y;
                            //last_value_x = valueX;
                            last_value_y = valueY;
                            ++index;
                        }
                        
                    }
                }
                else
                {
                    /*QMapIterator<double, double> iter(curve.mData);
                     while(iter.hasNext())
                     {
                     iter.next();
                     double valueX = iter.key();
                     double valueY = iter.value();
                     
                     if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                     {
                     double x = getXForValue(valueX, false);
                     double y = getYForValue(valueY, false);
                     
                     if(index == 0)
                     {
                     path.moveTo(x, y);
                     }
                     else
                     {
                     if(curve.mIsHisto)
                     path.lineTo(x, last_y);
                     path.lineTo(x, y);
                     }
                     last_x = x;
                     last_y = y;
                     ++index;
                     }
                     }*/
                    
                    // Down sample curve for better performances
                    
                    QMap<double, double> subData = curve.getMapDataInRange(mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);
                    
                    QMap<double, double> lightMap;
                    if(subData.size() > 2*mGraphWidth)
                    {
                        int valuesPerPixel = subData.size() / (2*mGraphWidth);
                        //qDebug() << "Graph drawing : step = " << valuesPerPixel << ", data size = " << subData.size() << ", org data size = " << curve.mData.size();
                        QMapIterator<double, double> iter(subData);
                        int index = 0;
                        while(iter.hasNext())
                        {
                            iter.next();
                            if(index % valuesPerPixel == 0)
                                lightMap[iter.key()] = iter.value();
                            ++index;
                        }
                    }
                    
                    else
                    {
                        lightMap = subData;
                    }
                    
                    // Draw
                    
                    QMapIterator<double, double> iter(lightMap);
                    iter.toFront();
                    if (!iter.hasNext()) {
                        continue;
                    }
                    
                    iter.next();
                    double valueX = iter.key();
                    double valueY = iter.value();
                    
                    qreal x = getXForValue(mCurrentMinX, false);
                    qreal y = getYForValue(0, false);
                    
                    path.moveTo(x, y);
                    iter.toFront();
                    while(iter.hasNext())
                    {
                        iter.next();
                        valueX = iter.key();
                        valueY = iter.value();
                        
                        /* if(curve.mName == "G")
                         {
                         //qDebug() << valueX << " : " << valueY;
                         }*/
                        if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                        {
                            x = getXForValue(valueX, false);
                            y = getYForValue(valueY, false);
                            
                            if(index == 0)
                            {
                                //path.moveTo(x, y);
                                path.lineTo(x, y);
                            }
                            else
                            {
                                if(curve.mIsHisto) // a revoir
                                {
                                    // histo bars must be centered around x value :
                                    qreal dx2 = (x - last_x)/2.f;
                                    path.lineTo(x - dx2, last_y);
                                    path.lineTo(x - dx2, y);
                                    //qDebug() << "y = " << valueY << ", last_y = " << last_value_y;
                                }
                                else if(curve.mIsRectFromZero && last_value_y == 0.f && valueY != 0.f)
                                {
                                    path.lineTo(x, last_y);
                                    path.lineTo(x, y);
                                }
                                else if(curve.mIsRectFromZero && last_value_y != 0.f && valueY == 0.f)
                                {
                                    path.lineTo(last_x, y);
                                    path.lineTo(x, y);
                                }
                                else
                                {
                                    path.lineTo(x, y);
                                }
                            }
                            last_x = x;
                            last_y = y;
                            //last_value_x = valueX;
                            last_value_y = valueY;
                            ++index;
                            /*if(iter.hasNext()) iter.next();
                             else break; */
                        }
                    }
                    if(curve.mIsRectFromZero && valueY != 0.f)
                    {
                        //iter.toBack();
                        x = getXForValue(mCurrentMaxX, false);
                        y = getYForValue(0, false);
                        path.lineTo(x, y);
                        
                    }
                    
                }
                painter.drawPath(path);
            }
            
            if(curve.mBrush != Qt::NoBrush)
            {
                // Close the path
                if(curve.mIsVertical)
                    path.lineTo(mMarginLeft, mMarginTop);
                else
                    path.lineTo(mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
                path.lineTo(mMarginLeft, mMarginTop + mGraphHeight);
                
                painter.setPen(curve.mPen);
                painter.fillPath(path, curve.mBrush);
            }
        }
    }
}

#pragma mark Save & Export

void GraphView::exportCurrentCurves(const QString& defaultPath, const QString& csvSep, bool writeInRows, int offset) const
{
    Q_UNUSED(writeInRows);
    
    QString filter = tr("CSV (*.csv)");
    QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph data as..."),
                                                    defaultPath,
                                                    filter);
    QFile file(filename);
    if(file.open(QFile::WriteOnly | QFile::Truncate))
    {
        bool abscissesWritten = false;
        QList<QStringList> rows;
        rows.append(QStringList(""));
        
        QMap<double, QVector<double> > rowsData;
        
        int colInx = 0;
        
        for(int i=0; i<mCurves.size(); ++i)
        {
            colInx++;
            
            if(!mCurves[i].mIsHorizontalLine &&
               !mCurves[i].mIsVerticalLine &&
               !mCurves[i].mIsVertical &&
               !mCurves[i].mIsHorizontalSections &&
               !mCurves[i].mUseVectorData)
            {
                const QMap<double, double>& data = mCurves[i].mData;
                
                QMapIterator<double, double> iter(data);
                rows[0] << mCurves[i].mName;
                
                int sht = 0;
                while(iter.hasNext() && sht<offset)
                {
                    ++sht;
                    iter.next();
                }
                
                while(iter.hasNext())
                {
                    iter.next();
                    
                    // Nouvelle abscisse trouvée
                    if(rowsData.find(iter.key()) == rowsData.end())
                    {
                        QVector<double> rowData;
                        for(int c=0; c<colInx-1; ++c)
                            rowData.append(0);
                        rowData.append(iter.value());
                        rowsData.insert(iter.key(), rowData);
                    }
                    else
                    {
                        rowsData[iter.key()].append(iter.value());
                    }
                }
            }
            else if(mCurves[i].mUseVectorData)
            {
                const QVector<double>& data = mCurves[i].mDataVector;
                if(!abscissesWritten)
                {
                    abscissesWritten = true;
                    
                    //rows.append(QStringList(""));
                    for(int i=offset; i<data.size(); ++i)
                    {
                        rows.append(QStringList(QString::number(i-offset+1)));
                    }
                }
                if(abscissesWritten)
                {
                    rows[0] << mCurves[i].mName;
                    for(int i=offset; i<data.size(); ++i)
                    {
                        rows[i-offset+1] << QString::number(data[i]);
                    }
                }
            }
        }
        
        QMapIterator<double, QVector<double> > iter2(rowsData);
        while(iter2.hasNext())
        {
            iter2.next();
            QStringList list;
            list << QString::number(iter2.key());
            for(int i=0; i<iter2.value().size(); ++i)
                list << QString::number(iter2.value()[i]);
            rows.append(list);
        }
        
        QTextStream output(&file);
        for(int i=0; i<rows.size(); ++i)
        {
            output << rows[i].join(csvSep);
            output << "\n";
        }
        file.close();
    }
}

bool GraphView::saveAsSVG(const QString& fileName, const QString graphTitle, const QString svgDescrition, const bool withVersion, int const versionHeight)
{
    if(fileName.isEmpty())
    {
        return false;
    }
    else
    {
        Rendering memoRendering= mRendering;
        QRect rTotal(withVersion ? QRect(0, 0, width(), height()+versionHeight) : QRect(0, 0, width(), height()));
        
        int graphRigthShift = 40;

        QRect rGraph(0, 0, width(), height());
        /* We can not have a svg graph in eSD Rendering Mode */
        setRendering(eHD);
        // Set SVG Generator
        QSvgGenerator svgGen;
        svgGen.setFileName(fileName);
        svgGen.setSize(rTotal.size());
        svgGen.setViewBox(rTotal);
        svgGen.setTitle(graphTitle);
        svgGen.setDescription(svgDescrition);
        
        QPainter painter;
        painter.begin(&svgGen);
        font().wordSpacing();
        render(&painter, QPoint(), QRegion(rGraph, QRegion::Rectangle));
        
        QFontMetrics fm(painter.font());
        painter.drawText( graphRigthShift,0, graphRigthShift+fm.width(graphTitle), versionHeight,
                         Qt::AlignCenter,
                         graphTitle);
        if (withVersion) {
            painter.setPen(Qt::black);
            painter.drawText(0, rGraph.y()+rGraph.height(), rGraph.width(),versionHeight,
                             Qt::AlignCenter,
                             qApp->applicationName() + " " + qApp->applicationVersion());
        }
        painter.end();
        
        setRendering(memoRendering);
        
        return true;
        
    }
    
}
QString GraphView::getInfo()
{
    
    return ( isShow() ? mInfos.join("|") : "");
}

bool GraphView::isShow()
{
    return mShowInfos;
}
bool GraphView::getXAxisMode()
{
    return mXAxisMode;
}
