#include "GraphView.h"
#include "Ruler.h"
#include "StdUtilities.h"
#include "DateUtils.h"
#include "Painting.h"
#include <QtWidgets>
#include <algorithm>
#include <QtSvg>
#include <QLocale>

class ProjectSettings;

#pragma mark Constructor / Destructor

#if GRAPH_OPENGL
GraphView::GraphView(QWidget *parent):QOpenGLWidget(parent),
#else
GraphView::GraphView(QWidget *parent):QWidget(parent),
#endif
mStepMinWidth(100), // define secondary scale on axis
mXAxisLine(true),
mXAxisArrow(true),
mXAxisTicks(true),
mXAxisSubTicks(true),
mXAxisValues(true),
mYAxisLine(true),
mYAxisArrow(true),
mYAxisTicks(true),
mYAxisSubTicks(true),
mYAxisValues(true),
mXAxisMode(eAllTicks),
mYAxisMode(eAllTicks),
mRendering(eSD),
mAutoAdjustYScale(false),
mFormatFuncX(0),
mFormatFuncY(0),
mShowInfos(false),
mBackgroundColor(Qt::white),
mThickness(1),
mTipX(0.),
mTipY(0.),
mTipWidth(110.),
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

void GraphView::showXAxisLine(bool show)     {if(mXAxisLine != show){mXAxisLine = show; repaintGraph(true);} }
void GraphView::showXAxisArrow(bool show)    {if(mXAxisArrow != show){mXAxisArrow = show; repaintGraph(true);} }
void GraphView::showXAxisTicks(bool show)    {if(mXAxisTicks != show){mXAxisTicks = show; repaintGraph(true);} }
void GraphView::showXAxisSubTicks(bool show) {if(mXAxisSubTicks != show){mXAxisSubTicks = show; repaintGraph(true);} }
void GraphView::showXAxisValues(bool show)   {if(mXAxisValues != show){mXAxisValues = show; repaintGraph(true);} }

void GraphView::showYAxisLine(bool show)     {if(mYAxisLine != show){mYAxisLine = show; repaintGraph(true);} }
void GraphView::showYAxisArrow(bool show)    {if(mYAxisArrow != show){mYAxisArrow = show; repaintGraph(true);} }
void GraphView::showYAxisTicks(bool show)    {if(mYAxisTicks != show){mYAxisTicks = show; repaintGraph(true);} }
void GraphView::showYAxisSubTicks(bool show) {if(mYAxisSubTicks != show){mYAxisSubTicks = show; repaintGraph(true);} }
void GraphView::showYAxisValues(bool show)   {if(mYAxisValues != show){mYAxisValues = show; repaintGraph(true);} }


void GraphView::setXAxisMode(AxisMode mode)
{
    if(mXAxisMode != mode)
    {
        mXAxisMode = mode;
        mAxisToolX.mShowText = (mXAxisMode!=eHidden);
        repaintGraph(true);
    }
}

void GraphView::setYAxisMode(AxisMode mode)
{
    if(mYAxisMode != mode)
    {
        mYAxisMode = mode;
        showYAxisValues(true);
        showYAxisTicks(true);
        showYAxisSubTicks(true);
        
        if(mYAxisMode==eMinMax){
            showYAxisValues(true);
            showYAxisTicks(false);
            showYAxisSubTicks(false);
            
        }

        mAxisToolY.mMinMaxOnly = (mYAxisMode == eMinMax);
        
        if(mYAxisMode==eHidden){
            showYAxisValues(false);
            showYAxisTicks(false);
            showYAxisSubTicks(false);
            /*mAxisToolY.mShowText = false;
            mAxisToolY.mShowSubs=false;
            mAxisToolY.mShowSubSubs=false;*/
            
        }
        repaintGraph(true);
    }
}

/**
 * @brief If active is true, the current view automaticaly adjust Y axis to the curent view.
 * @brief it's a dynamic adjustment
 */
void GraphView::autoAdjustYScale(bool active)
{
    mAutoAdjustYScale = active;
    repaintGraph(true);
}
/**
 * @brief Adjust the Y axis with 0 for the minimun and find the Maximum value in the visible curve
 */
void GraphView::adjustYToMaxValue(const double& marginProp)
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
    setRangeY(0, yMax * (1. + marginProp));
}
void GraphView::adjustYToMinMaxValue()
{
    double yMin = 0;
    double yMax = 0;

    QList<GraphCurve>::const_iterator iter = mCurves.cbegin();
    bool firstFound = false;
    while(iter != mCurves.cend()){
        if(iter->mVisible){
            if(iter->mUseVectorData){
                yMin = firstFound ? qMin(yMin, vector_min_value(iter->mDataVector)) : vector_min_value(iter->mDataVector);
                yMax = firstFound ? qMax(yMax, vector_max_value(iter->mDataVector)) : vector_max_value(iter->mDataVector);
            }
            else if(!iter->mUseVectorData &&
                    !iter->mIsHorizontalLine &&
                    !iter->mIsHorizontalSections &&
                    !iter->mIsVerticalLine &&
                    !iter->mIsVertical){
                yMin = firstFound ? qMin(yMin, map_min_value(iter->mData)) : map_min_value(iter->mData);
                yMax = firstFound ? qMax(yMax, map_max_value(iter->mData)) : map_max_value(iter->mData);
            }
            firstFound = true;
        }
        ++iter;
    }
    setRangeY(yMin, yMax);

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
        QLocale locale;
        
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
            mTipX = locale.toDouble(mFormatFuncX(mTipX));
        
        mTipY = getValueForY(e->y()+0.5);
        if(mFormatFuncY)
            mTipY = locale.toDouble(mFormatFuncY(mTipY));
        
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

void GraphView::setTipXLab(const QString& lab)
{
    mTipXLab = lab =="" ? "":  lab + " = ";
}
void GraphView::setTipYLab(const QString& lab)
{
    mTipYLab = lab =="" ? "":  lab + " = ";
}

#pragma mark Resize & Paint

#if GRAPH_OPENGL
void GraphView::initializeGL()
{
    //QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    //f->glEnable(GL_MULTISAMPLE_ARB);
}
void GraphView::resizeGL(int w, int h)
{
    repaintGraph(true);
}
#else
void GraphView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    repaintGraph(true);
}
#endif

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
    if(mTipVisible && (!mTipXLab.isEmpty() || !mTipYLab.isEmpty()))
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
        font.setPointSizeF(pointSize(10));
        
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing);
        p.setFont(font);
        
        p.fillPath(tipPath, QColor(0, 0, 0, 180));
        p.setPen(Qt::black);
        p.drawPath(tipPath);
        p.setPen(Qt::white);
        if(!mTipXLab.isEmpty() && !mTipYLab.isEmpty()){
            p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2), Qt::AlignCenter, mTipXLab + DateUtils::dateToString(mTipX));
            p.drawText(mTipRect.adjusted(0, (int)(mTipRect.height()/2), 0, 0), Qt::AlignCenter, mTipYLab + DateUtils::dateToString(mTipY));
        }
        else if(!mTipXLab.isEmpty()){
            p.drawText(mTipRect, Qt::AlignCenter, mTipXLab + DateUtils::dateToString(mTipX));
        }
        else if(!mTipYLab.isEmpty()){
            p.drawText(mTipRect, Qt::AlignCenter, mTipYLab + DateUtils::dateToString(mTipY));
        }
       
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
    if(!mLegendX.isEmpty()) {
        QRectF tr(mMarginLeft, mGraphHeight- mMarginBottom, mGraphWidth, mMarginBottom);
        p.setPen(Qt::black);
        p.drawText(tr, Qt::AlignRight | Qt::AlignTop, mLegendX);
    }
    
    mAxisToolX.mShowArrow = mXAxisArrow;
    mAxisToolX.mShowSubs = mXAxisTicks;
    mAxisToolX.mShowSubSubs = mXAxisSubTicks;
    mAxisToolX.mShowText = mXAxisValues;
    
    mAxisToolX.updateValues(mGraphWidth, mStepMinWidth, mCurrentMinX, mCurrentMaxX);
    QVector<qreal> linesXPos = mAxisToolX.paint(p, QRectF(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth , mMarginBottom), 7, mFormatFuncX);
    
    // ----------------------------------------------------
    //  Horizontal Grid
    // ----------------------------------------------------
    mAxisToolY.mShowArrow = mYAxisArrow;
    mAxisToolY.mShowSubs = mYAxisTicks;
    mAxisToolY.mShowSubSubs = mYAxisSubTicks;
    mAxisToolY.mShowText = mYAxisValues;

    mAxisToolY.updateValues(mGraphHeight, mStepMinWidth, mMinY, mMaxY);
    QVector<qreal> linesYPos = mAxisToolY.paint(p, QRectF(0, mMarginTop, mMarginLeft, mGraphHeight), 5, mFormatFuncY);
    
    
    // ----------------------------------------------------
    //  Graph specific infos at the top right
    // ----------------------------------------------------
    if(mShowInfos)
    {
        font.setPointSizeF(font.pointSizeF() + 2.);
        p.setFont(font);
        p.setPen(QColor(50, 50, 50));
        int y = 0;
        int lineH = 16;
        for(int i=0; i<mInfos.size(); ++i){
            p.drawText(mMarginLeft + 5, mMarginTop + 5 + y, mGraphWidth - 10, lineH, Qt::AlignRight | Qt::AlignTop, mInfos[i]);
            y += lineH;
        }
    }
    p.end();
}

void GraphView::drawCurves(QPainter& painter)
{
    // ---------------------------------------------------------------
    //  Curves
    // ---------------------------------------------------------------
    painter.save();
    // Draw curves inside axis only (not in margins!)
    painter.setClipRect(mMarginLeft, mMarginTop, mGraphWidth, mGraphHeight);
    
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
                qreal x = getXForValue(curve.mVerticalValue, false);
                path.moveTo(x, mMarginTop + mGraphHeight);
                path.lineTo(x, mMarginTop);
                
                painter.strokePath(path, curve.mPen);
            }
            else if(curve.mIsHorizontalSections)
            {
                qreal y1 = mMarginTop;
                qreal y0 = mMarginTop + mGraphHeight;
                path.moveTo(mMarginLeft, y0);
                for(int i=0; i<curve.mSections.size(); ++i)
                {
                    qreal x1 = getXForValue(curve.mSections[i].first, false);
                    //x1 = qMax(x1, mMarginLeft);
                    //x1 = qMin(x1, mMarginLeft + mGraphWidth);

                    qreal x2 = getXForValue(curve.mSections[i].second, false);
                    //x2 = qMax(x2, mMarginLeft);
                    //x2 = qMin(x2, mMarginLeft + mGraphWidth);
                    
                    path.lineTo(x1, y0);
                    path.lineTo(x1, y1);
                    path.lineTo(x2, y1);
                    path.lineTo(x2, y0);
                }
                path.lineTo(mMarginLeft + mGraphWidth, y0);
                
                painter.setClipRect(mMarginLeft, mMarginTop, mGraphWidth, mGraphHeight);
                painter.fillPath(path, curve.mBrush);
                painter.strokePath(path, curve.mPen);
            }
            else if(curve.mIsTopLineSections)
            {
                qreal y1 = mMarginTop + curve.mPen.width();
                painter.setPen(curve.mPen);
                for(int i=0; i<curve.mSections.size(); ++i)
                {
                    qreal x1 = getXForValue(curve.mSections[i].first, false);
                    qreal x2 = getXForValue(curve.mSections[i].second, false);
                    
                    painter.drawLine(QPointF(x1, y1),QPointF(x2, y1));
                }
            }
            else if(curve.mIsVertical)
            {
                path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);
                
                int index = 0;
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
                    
                    if(index == 0)
                    {
                        path.lineTo(x, y);
                    }
                    else
                    {
                        if(curve.mIsHisto)
                            path.lineTo(x, last_y);
                        path.lineTo(x, y);
                    }
                    last_y = y;
                    ++index;
                }
                path.lineTo(mMarginLeft, mMarginTop);
                painter.drawPath(path);
            }
            else // it's horizontal curve
            {
                path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);
                
                //int index = 0;
                qreal last_x = 0;
                qreal last_y = 0;
                qreal last_value_y = 0;
                
                if(curve.mUseVectorData)
                {
                    // Down sample vector
                    
                    QVector<double> subData = curve.getVectorDataInRange(mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);
                    
                    QVector<double> lightData;
                    double dataStep = (double)subData.size() / (double)(2.*mGraphWidth);
                    if(dataStep > 1)
                    {
                        for(int i=0; i<2*mGraphWidth; ++i)
                        {
                            int idx = (int)round(i * dataStep);
                            lightData.append(subData[idx]);
                        }
                    }
                    else
                    {
                        lightData = subData;
                    }
                    bool isFirst=true;
                    
                    //path.moveTo(getXForValue(mCurrentMinX, false), getYForValue(0, false));
                    for(int i=0; i<lightData.size(); ++i)
                    {
                        // Use "dataStep" only if lightData is different of subData !
                        double valueX = mCurrentMinX + ((dataStep > 1) ? i * dataStep : i);
                        double valueY = lightData[i];
                        
                        if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                        {
                            qreal x = getXForValue(valueX, false);
                            qreal y = getYForValue(valueY, false);
                            
                             if(isFirst)
                            {
                                path.moveTo(x, y);
                                //path.lineTo(x, y);
                                isFirst=false;
                            }
                            else
                            {
                                if(curve.mIsHisto)
                                    path.lineTo(x, last_y);
                                path.lineTo(x, y);
                            }
                            last_x = x;
                            last_y = y;
                            last_value_y = valueY;
                            //++index;
                        }
                        
                    }
                   // path.lineTo(last_x, last_value_y);
                }
                else
                {
                    // Down sample curve for better performances
                    
                    QMap<double, double> subData = curve.getMapDataInRange(mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);
                    
                    QMap<double, double> lightMap;
                    if(subData.size() > 2*mGraphWidth)
                    {
                        int valuesPerPixel = subData.size() / (2*mGraphWidth);
                        
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
                    
                    iter.toFront();
                    bool isFirst=true;
                    
                    if(curve.mBrush != Qt::NoBrush) {
                        isFirst=false;
                        last_x= getXForValue(valueX, false);
                        last_y = y;
                    }
                    while(iter.hasNext())
                    {
                        iter.next();
                        valueX = iter.key();
                        valueY = iter.value();
                        
                        if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                        {
                            x = getXForValue(valueX, false);
                            y = getYForValue(valueY, false);
                            
                            if(isFirst)
                            {
                                path.moveTo(x, y);
                                isFirst=false;
                            }
                            else
                            {
                                if(curve.mIsHisto)
                                {
                                    // histo bars must be centered around x value :
                                    qreal dx2 = (x - last_x)/2.f;
                                    path.lineTo(x - dx2, last_y);
                                    path.lineTo(x - dx2, y);
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

                            last_value_y = valueY;
                        }
                    }
                    
                    
                    if(curve.mIsRectFromZero && valueY != 0.f)
                    {
                        x = last_x;
                        y = getYForValue(0, false);
                        path.lineTo(x, y);
                        
                    }
                    
                }
                painter.drawPath(path);
                if(curve.mBrush != Qt::NoBrush)
                {
                    // Close the path
                   /* if(curve.mIsVertical)
                     path.lineTo(mMarginLeft, mMarginTop);
                     else
                     path.lineTo(mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
                     path.lineTo(mMarginLeft, mMarginTop + mGraphHeight);
                    */
                    path.lineTo(last_x, getYForValue(0, false));
                    
                    painter.setPen(curve.mPen);
                    painter.fillPath(path, curve.mBrush);
                }
            }
        }
    }
    painter.restore();
}

#pragma mark Save & Export

/**
 * @brief Export a density with locale setting and separator and specific step
 * @todo Maybe we can use QString QLocale::createSeparatedList(const QStringList & list) const
 */
void GraphView::exportCurrentDensityCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, double step) const
{
    if (step<=0) step=1;
    QString filter = tr("CSV (*.csv)");
    QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph data as..."),
                                                    defaultPath,
                                                    filter);
    QFile file(filename);
    if(file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qDebug()<<"GraphView::exportCurrentCurves"<<" nbCurve to export"<<mCurves.size();
        
        QList<QStringList> rows;
        
        
        QStringList list;
        
        list <<"X Axis";
        double xMin = 0;
        double xMax = 0;
        
        for(auto iter= mCurves.begin(); iter != mCurves.end(); ++iter) {
            if (!iter->mData.empty() &&
                !iter->mIsHorizontalLine &&
                !iter->mIsVerticalLine &&
                !iter->mIsVertical &&
                !iter->mIsHorizontalSections &&
                !iter->mUseVectorData &&
                iter->mVisible) {
                
                // 1 -Create the header
                list << iter->mName;
                // 2 - Find x Min and x Max period, on all curve, we suppose Qmap is order
                xMin = qMin(xMin, iter->mData.firstKey());
                xMax = qMax(xMax, iter->mData.lastKey());
                
            }
            else continue;
        }
        rows<<list;
        rows.reserve(ceil( (xMax-xMin)/step) );
        
        // 3 - Create Row, with each curve
        //  Create data in row
        for(double x= xMin; x <= xMax; x += step) {
            list.clear();
            if(mFormatFuncX) {
                list << mFormatFuncX(x);
            }
            else list << QString::number(x);
            for(auto iter= mCurves.begin(); iter != mCurves.end(); ++iter) {
                
                if (!iter->mData.empty() &&
                    !iter->mIsHorizontalLine &&
                    !iter->mIsVerticalLine &&
                    !iter->mIsVertical &&
                    !iter->mIsHorizontalSections &&
                    !iter->mUseVectorData &&
                    iter->mVisible) {
                    
                    double xi = interpolateValueInQMap(x, iter->mData);
                    list<<locale.toString(xi);
                }
                else continue;
                
            }
            rows<<list;
        }
        
        // 4 - Save Qlist
        QTextStream output(&file);
        for(int i=0; i<rows.size(); ++i)
        {
            output << rows[i].join(csvSep);
            output << "\n";
        }
        file.close();
    }
    
}


void GraphView::exportCurrentVectorCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, bool writeInRows, int offset) const
{
    Q_UNUSED(writeInRows);
    qDebug()<<"GraphView::exportCurrentVectorCurves";
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

        rows.append(QStringList("X Axis"));
        QMap<double, QVector<double> > rowsData;
        
        int rowsCount = rows.count();
        QStringList emptyColumn;
        
        qDebug()<<"GraphView::exportCurrentVectorCurves"<<" nbCurve to export"<<mCurves.size();
        for(int idCurve=0; idCurve<mCurves.size(); ++idCurve)
        {
            if ( !mCurves[idCurve].mVisible || mCurves[idCurve].mDataVector.empty() ) continue;
            
            const QVector<double>& data = mCurves[idCurve].mDataVector;
            // the new DataVector is longer than the last, we need to expand the size of rows
            if (data.size()>rowsCount-2) {
                abscissesWritten=false;
                rowsCount = rows.count();
                qDebug()<<"rowscount="<<rowsCount<<data.size();
            }
            
            if (!abscissesWritten) {
                for(int i=offset+rowsCount; i<data.size()+1; ++i) {
                    rows.append(QStringList(locale.toString(i-rowsCount))+emptyColumn);
                   
                }
                abscissesWritten = true;
                
            }
            //prepare adding row
            emptyColumn<<"";
            
            if(abscissesWritten) {
                    rows[0] << mCurves[idCurve].mName;
                    for(int i=offset; i<data.size(); ++i) {
                        rows[i-offset+1]<< locale.toString(data[i]);
                    }
            }
            
        }
        
        QMapIterator<double, QVector<double> > iter2(rowsData);
        while(iter2.hasNext())
        {
            iter2.next();
            QStringList list;

            if(mFormatFuncX) {
                list << mFormatFuncX(iter2.key());
            }
            else list << locale.toString(iter2.key());
            
            for(int i=0; i<iter2.value().size(); ++i)
                if(mFormatFuncX) {
                    list << mFormatFuncY(iter2.value()[i]);
                }
                else list << locale.toString(iter2.value()[i]);
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

bool GraphView::saveAsSVG(const QString& fileName, const QString& graphTitle, const QString& svgDescrition, const bool withVersion, const int versionHeight)
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

