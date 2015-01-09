#include "GraphView.h"
#include "Ruler.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>


#pragma mark Constructor / Destructor

GraphView::GraphView(QWidget *parent):QWidget(parent),
mRendering(eSD),
mShowAxisArrows(true),
mShowAxisLines(true),
mShowVertGrid(true),
mShowHorizGrid(true),
mXAxisMode(eAllTicks),
mYAxisMode(eAllTicks),
mAutoAdjustYScale(false),
mShowInfos(false),
mBackgroundColor(Qt::white),
mTipX(0.),
mTipY(0.),
mTipWidth(100.),
mTipHeight(40.),
mTipVisible(false),
mUseTip(true)
{
    mAxisToolX.mIsHorizontal = true;
    mAxisToolY.mIsHorizontal = false;
    mAxisToolY.mShowSubs = false;
    
    mTipRect.setTop(0);
    mTipRect.setLeft(0);
    mTipRect.setWidth(mTipWidth);
    mTipRect.setHeight(mTipHeight);
    
    setMouseTracking(true);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    
    setRangeX(0, 2000);
    setRangeY(0.f, 1.f);
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
        
        if(mAutoAdjustYScale)
        {
            double yMax = -100000000;
            double yMin = 100000000;
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

void GraphView::setRendering(GraphView::Rendering render)
{
    mRendering = render;
    repaintGraph(true);
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

GraphCurve* GraphView::getCurve(const QString& name)
{
    for(int i=0; i<mCurves.size(); ++i)
    {
        if(mCurves[i].mName == name)
            return &mCurves[i];
    }
    return 0;
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
        
        mTipX = getValueForX(e->x());
        mTipY = getValueForY(e->y());
        
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
void GraphView::repaintGraph(const bool aAlsoPaintBackground)
{
    if(aAlsoPaintBackground)
    {
        mBufferBack = QPixmap();
    }
    update();
}

void GraphView::resizeEvent(QResizeEvent*)
{
    repaintGraph(true);
}

void GraphView::updateGraphSize(int w, int h)
{
    mGraphWidth = w - mMarginLeft - mMarginRight;
    mGraphHeight = h - mMarginTop - mMarginBottom;
    
    mAxisToolX.updateValues(mGraphWidth, 40, mCurrentMinX, mCurrentMaxX);
    mAxisToolY.updateValues(mGraphHeight, 12, mMinY, mMaxY);
}

void GraphView::paintEvent(QPaintEvent*)
{
    updateGraphSize(width(), height());
    
    // ----------------------------------------------------
    //  Nothing to draw !
    // ----------------------------------------------------
    if(mCurves.size() == 0)
    {
        QPainter p(this);
        p.fillRect(0, 0, width(), height(), QColor(200, 200, 200));
        p.setPen(QColor(100, 100, 100));
        p.drawText(0, 0, width(), height(), Qt::AlignCenter, tr("Nothing to display"));
        p.end();
        return;
    }
    
    // ----------------------------------------------------
    //  SD : draw on a buffer only if it has been reset
    // ----------------------------------------------------
    if(mBufferBack.isNull() && mRendering == eSD)
    {
        mBufferBack = QPixmap(width(), height());
        paintToDevice(&mBufferBack);
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
        p.end();
    }
    
    // ----------------------------------------------------
    //  Tool Tip (above all)
    // ----------------------------------------------------
    if(mTipVisible)
    {
        QPainterPath tipPath;
        tipPath.addRoundedRect(mTipRect, 5, 5);
     
        QPainter p(this);
        
        QFont font;
        font.setPointSizeF(pointSize(9));
        p.setFont(font);
        
        p.fillPath(tipPath, QColor(0, 0, 0, 180));
        p.setPen(Qt::black);
        p.drawPath(tipPath);
        p.setPen(Qt::white);
        p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2), Qt::AlignCenter, "x : " + QString::number(mTipX));
        p.drawText(mTipRect.adjusted(0, mTipRect.height()/2, 0, 0), Qt::AlignCenter, "y : " + QString::number(mTipY));
        
        p.end();
    }
}

void GraphView::paintToDevice(QPaintDevice* device)
{
    QPainter p;
    p.begin(device);
    p.setRenderHints(QPainter::Antialiasing);
    
    // ----------------------------------------------------
    //  Background
    // ----------------------------------------------------
    p.fillRect(0, 0, width(), height(), mBackgroundColor);
    
    // ----------------------------------------------------
    //  Zones
    // ----------------------------------------------------
    for(int i=0; i<mZones.size(); ++i)
    {
        double x1 = getXForValue(mZones[i].mXStart);
        double x2 = getXForValue(mZones[i].mXEnd);
        QRectF r(x1, mMarginTop, x2 - x1, mGraphHeight);
        p.fillRect(r, mZones[i].mColor);
        p.drawText(r.adjusted(5, 5, -5, -5), Qt::AlignHCenter | Qt::AlignTop, mZones[i].mText);
    }
    
    // ----------------------------------------------------
    //  Vertical Grid
    // ----------------------------------------------------
    if(mXAxisMode != eHidden)
    {
        QVector<double> linesXPos = mAxisToolX.paint(p, QRectF(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth, mMarginBottom), 40);
        
        if(mShowVertGrid)
        {
            p.setPen(QColor(0, 0, 0, 20));
            for(int i=0; i<linesXPos.size(); ++i)
            {
                double x = linesXPos[i];
                p.drawLine(x, mMarginTop, x, mMarginTop + mGraphHeight);
            }
        }
    }
    
    // ----------------------------------------------------
    //  Horizontal Grid
    // ----------------------------------------------------
    if(mYAxisMode != eHidden)
    {
        QVector<double> linesYPos = mAxisToolY.paint(p, QRectF(0, mMarginTop, mMarginLeft, mGraphHeight), 20);
        
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
    
    // ----------------------------------------------------
    //  Curves
    // ----------------------------------------------------
    drawCurves(p);
    
    // ----------------------------------------------------
    //  Infos
    // ----------------------------------------------------
    if(mShowInfos)
    {
        QString infos = mInfos.join(" | ");
        p.setPen(QColor(50, 50, 50));
        QFont font = p.font();
        font.setPointSize(pointSize(11));
        p.setFont(font);
        p.drawText(mMarginLeft + 5, mMarginTop + 5, mGraphWidth - 10, 15, Qt::AlignRight | Qt::AlignTop, infos);
    }
    
    // ----------------------------------------------------
    //  Axis Lines (above curves)
    // ----------------------------------------------------
    if(mShowAxisLines)
    {
        QColor axisCol(120, 120, 120);
        p.setBrush(axisCol);
        p.setPen(axisCol);
        p.drawLine(mMarginLeft, mMarginTop, mMarginLeft, mMarginTop + mGraphHeight);
        p.drawLine(mMarginLeft, mMarginTop + mGraphHeight, mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
        
        QPainterPath arrowTop;
        arrowTop.moveTo(mMarginLeft, mMarginTop);
        arrowTop.lineTo(mMarginLeft - 3, mMarginTop + 5);
        arrowTop.lineTo(mMarginLeft + 3, mMarginTop + 5);
        arrowTop.lineTo(mMarginLeft, mMarginTop);
        p.drawPath(arrowTop);
        
        QPainterPath arrowRight;
        arrowRight.moveTo(mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
        arrowRight.lineTo(mMarginLeft + mGraphWidth - 5, mMarginTop + mGraphHeight - 3);
        arrowRight.lineTo(mMarginLeft + mGraphWidth - 5, mMarginTop + mGraphHeight + 3);
        arrowRight.lineTo(mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
        p.drawPath(arrowRight);
    }
    
    p.end();
}

void GraphView::drawCurves(QPainter& painter)
{
    // ---------------------------------------------------------------
    //  Curves
    // ---------------------------------------------------------------
    for(int curveIndex=0; curveIndex<mCurves.size(); ++curveIndex)
    {
        const GraphCurve& curve = mCurves[curveIndex];
        
        if(!curve.mVisible)
            continue;
        
        QPainterPath path;
        painter.setPen(curve.mPen);
        
        if(curve.mIsHorizontalLine)
        {
            double y = getYForValue(curve.mHorizontalValue);
            path.moveTo(mMarginLeft, y);
            path.lineTo(mMarginLeft + mGraphWidth, y);
            painter.strokePath(path, curve.mPen);
        }
        else if(curve.mIsVerticalLine)
        {
            double x = getXForValue(curve.mVerticalValue);
            path.moveTo(x, mMarginTop + mGraphHeight);
            path.lineTo(x, mMarginTop);
            painter.strokePath(path, curve.mPen);
        }
        else if(curve.mIsHorizontalSections)
        {
            double y = getYForValue(curve.mHorizontalValue);
            path.moveTo(mMarginLeft, y);
            for(int i=0; i<curve.mSections.size(); ++i)
            {
                path.moveTo(getXForValue(curve.mSections[i].first), y);
                path.lineTo(getXForValue(curve.mSections[i].second), y);
            }
            path.moveTo(mMarginLeft + mGraphWidth, y);
            painter.strokePath(path, curve.mPen);
        }
        else if(curve.mIsVertical)
        {
            path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);
            
            int index = 0;
            double last_x = 0;
            double last_y = 0;
            
            QMapIterator<double, double> iter(curve.mData);
            while(iter.hasNext())
            {
                iter.next();
                
                double valueX = iter.value();
                double valueY = iter.key();
                
                // vertical curves must be normalized (values from 0 to 1)
                // They are drawn using a 100px width
                double x = mMarginLeft + valueX * 100;
                double y = getYForValue(valueY, false);
                
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
                last_x = x;
                last_y = y;
                ++index;
            }
            painter.drawPath(path);
        }
        else
        {
            path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);
            
            int index = 0;
            double last_x = 0;
            double last_y = 0;
            double last_value_y = 0;
            double last_value_x = 0;
            
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
                if(subData.size() > 2*mGraphWidth)
                {
                    double step = subData.size() / (2*mGraphWidth);
                    for(int i=0; i<2*mGraphWidth; ++i)
                    {
                        int idx = (int)round(i * step);
                        lightData[i] = subData[idx];
                    }
                }
                else
                {
                    lightData = subData;
                }
                
                for(int x=0; x<lightData.size(); ++x)
                {
                    double valueX = x;
                    double valueY = lightData[x];
                    
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
                if(subData.size() > 4*mGraphWidth)
                {
                    int valuesPerPixel = subData.size() / (4*mGraphWidth);
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
                while(iter.hasNext())
                {
                    iter.next();
                    double valueX = iter.key();
                    double valueY = iter.value();
                    
                    if(curve.mName == "G")
                    {
                        //qDebug() << valueX << " : " << valueY;
                    }
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
                            {
                                // histo bars must be centered around x value :
                                double dx2 = (x - last_x)/2.f;
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
                        last_value_x = valueX;
                        last_value_y = valueY;
                        ++index;
                    }
                }
                
            }
            painter.drawPath(path);
        }
        
        if(curve.mFillUnder)
        {
            // Close the path
            if(curve.mIsVertical)
                path.lineTo(mMarginLeft, mMarginTop);
            else
                path.lineTo(mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
            path.lineTo(mMarginLeft, mMarginTop + mGraphHeight);
            
            QColor c = curve.mPen.color();
            c.setAlpha(50);
            painter.fillPath(path, c);
        }
    }
}

void GraphView::exportCurrentCurves(const QString& defaultPath, const QString& csvSep, bool writeInRows) const
{
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
        
        for(int i=0; i<mCurves.size(); ++i)
        {
            if(!mCurves[i].mIsHorizontalLine &&
               !mCurves[i].mIsVerticalLine &&
               !mCurves[i].mIsVertical &&
               !mCurves[i].mIsHorizontalSections &&
               !mCurves[i].mUseVectorData)
            {
                if(writeInRows)
                {
                    const QMap<double, double>& data = mCurves[i].mData;
                    if(!abscissesWritten)
                    {
                        abscissesWritten = true;
                        QMapIterator<double, double> iter(data);
                        QStringList abscisses;
                        abscisses << "";
                        while(iter.hasNext())
                        {
                            iter.next();
                            abscisses << QString::number(iter.key());
                        }
                        rows.append(abscisses);
                    }
                    QMapIterator<double, double> iter(data);
                    QStringList ordonnees;
                    ordonnees << mCurves[i].mName;
                    while(iter.hasNext())
                    {
                        iter.next();
                        ordonnees << QString::number(iter.value());
                    }
                    rows.append(ordonnees);
                }
                else
                {
                    const QMap<double, double>& data = mCurves[i].mData;
                    if(!abscissesWritten)
                    {
                        abscissesWritten = true;
                        QMapIterator<double, double> iter(data);
                        QStringList abscisses;
                        abscisses << "";
                        rows.append(QStringList(""));
                        while(iter.hasNext())
                        {
                            iter.next();
                            rows.append(QStringList(QString::number(iter.key())));
                        }
                    }
                    if(abscissesWritten)
                    {
                        QMapIterator<double, double> iter(data);
                        rows[0] << mCurves[i].mName;
                        int index = 0;
                        while(iter.hasNext())
                        {
                            iter.next();
                            ++index;
                            rows[index] << QString::number(iter.value());
                        }
                    }
                }
            }
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