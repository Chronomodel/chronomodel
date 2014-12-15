#include "GraphView.h"
#include "Ruler.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>

using namespace std;




void GraphView::setRangeX(const float min, const float max)
{
    mRuler->setRange(min, max);
    GraphViewAbstract::setRangeX(min, max);
}

void GraphView::setCurrentRangeX(const float min, const float max)
{
    if(mCurrentMinX != min || mCurrentMaxX || max)
    {
        mCurrentMinX = min;
        mCurrentMaxX = max;
        repaintGraph(true);
    }
}

void GraphView::zoomX(const float min, const float max)
{
    mRuler->setCurrentRange(min, max);
}


#pragma mark Constructor / Destructor

GraphView::GraphView(QWidget *parent):QWidget(parent),
mBackgroundColor(Qt::white),
mTipX(0.),
mTipY(0.),
mTipWidth(100.),
mTipHeight(40.),
mTipVisible(false),
mUseTip(true),
mShowInfos(false),
mShowScrollBar(true),
mShowAxis(true),
mShowYValues(false),
mShowGrid(true),
mStepYMinHeight(8.f)
{
    mRuler = new Ruler(this);
    connect(mRuler, SIGNAL(zoomChanged(const float, const float)), this, SLOT(setCurrentRangeX(const float, const float)));
    
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


/* ------------------------------------------------------
 Options
 ------------------------------------------------------ */
#pragma mark Options

void GraphView::setBackgroundColor(const QColor& aColor)
{
    mBackgroundColor = aColor;
    repaintGraph(true);
}

QColor GraphView::backgroundColor() const
{
    return mBackgroundColor;
}

void GraphView::addInfo(const QString& info)
{
    mInfos << info;
    repaintGraph(false);
}

void GraphView::clearInfos()
{
    mInfos.clear();
    repaintGraph(false);
}

void GraphView::showInfos(bool show)
{
    mShowInfos = show;
    repaintGraph(true);
}

void GraphView::showAxis(bool show)
{
    mShowAxis = show;
    mRuler->setVisible(mShowAxis);
    adaptMarginBottom();
}

void GraphView::showScrollBar(bool show)
{
    mShowScrollBar = show;
    mRuler->showScrollBar(mShowScrollBar);
    showAxis(mShowAxis);
}

void GraphView::showYValues(bool show, bool keepMargin)
{
    mShowYValues = show;
    setMarginLeft(mShowYValues ? 50 : keepMargin ? 50 : 0);
    setMarginTop(show ? mStepYMinHeight : 0);
    adaptMarginBottom();
}

void GraphView::showGrid(bool show)
{
    mShowGrid = show;
    repaintGraph(true);
}

void GraphView::adaptMarginBottom()
{
    if(mShowAxis)
    {
        int barH = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        setMarginBottom(mShowScrollBar ? 40 : 40 - barH);
    }
    else
    {
        setMarginBottom(mShowYValues ? mStepYMinHeight : 0);
    }
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


/* ------------------------------------------------------
 Paint
 ------------------------------------------------------ */
#pragma mark Paint

void GraphView::repaintGraph(const bool aAlsoPaintBackground, const bool aAlsoPaintGraphs)
{
    if(aAlsoPaintBackground)
    {
        mBufferedImage = QPixmap();
        mBufferedImageWithGraphs = QPixmap();
    }
    if(aAlsoPaintGraphs)
    {
        mBufferedImageWithGraphs = QPixmap();
    }
    update();
}

#pragma mark Overloads

void GraphView::resizeEvent(QResizeEvent*)
{
    updateGraphSize(width(), height());
    mRuler->setGeometry(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth, mMarginBottom);
    repaintGraph(true);
}

void GraphView::paintEvent(QPaintEvent*)
{
    updateGraphSize(width(), height());
    
    QPainter painter(this);
    paint(painter, width(), height());
    painter.end();
}

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

#pragma mark Sub-Painting

void GraphView::drawBackground(QPainter& painter)
{
    //painter.fillRect(mMarginLeft, mMarginTop, mGraphWidth, mGraphHeight, mBackgroundColor);
    painter.fillRect(0, 0, width(), height(), mBackgroundColor);
    
    for(int i=0; i<mZones.size(); ++i)
    {
        float x1 = getXForValue(mZones[i].mXStart);
        float x2 = getXForValue(mZones[i].mXEnd);
        QRectF r(x1, mMarginTop, x2 - x1, mGraphHeight);
        painter.fillRect(r, mZones[i].mColor);
        painter.drawText(r.adjusted(5, 5, -5, -5), Qt::AlignHCenter | Qt::AlignTop, mZones[i].mText);
    }
}

void GraphView::drawXAxis(QPainter& painter)
{
    /*int y = getYForValue(0);
    if(y >= mMinY && y <= mMaxY)
    {
        painter.setPen(QColor(120, 120, 120));
        painter.drawLine(mMarginLeft, y, mMarginLeft + mGraphWidth, y);
    }*/
    
    painter.setPen(QColor(0, 0, 0, 20));
    for(float y=mMarginTop+mGraphHeight; y>=mMarginTop-0.1f; y -= mStepYHeight)
    {
        painter.drawLine(QLineF(mMarginLeft, y, mMarginLeft+mGraphWidth, y));
    }
}

void GraphView::drawYValues(QPainter& painter)
{
    QFont font = painter.font();
    font.setPointSizeF(pointSize(9.f));
    painter.setFont(font);
    painter.setPen(Qt::black);
    
    float y0 = getValueForY(mMarginTop + mGraphHeight);
    int index = 0;
    for(float y=mMarginTop+mGraphHeight; y>=mMarginTop-0.1f; y -= mStepYHeight)
    {
        QRect textRect(0, y - mStepYHeight/2.f, mMarginLeft, mStepYHeight);
        
        painter.drawText(textRect,
                         Qt::AlignCenter,
                         QString::number(y0 + index * mDyPerStep));
        
        //qDebug() << index << " : " << y << ", next = " << (y-mStepYHeight) << " >= " << mMarginTop;
        ++index;
    }
}

void GraphView::drawYAxis(QPainter& painter)
{
    /*int x = getXForValue(0);
    if(x >= mMinX && x <= mMaxX)
    {
        painter.setPen(QColor(120, 120, 120));
        painter.drawLine(x, mMarginTop, x, mMarginTop + mGraphHeight);
    }*/
    
    painter.setPen(QColor(0, 0, 0, 20));
    for(float x=mMarginLeft; x<mMarginLeft + mGraphWidth; x += mStepXWidth)
    {
        painter.drawLine(QLineF(x, mMarginTop, x, mMarginTop + mGraphHeight));
    }
}

#pragma mark Painting

void GraphView::updateGraphSize(int w, int h)
{
    mGraphWidth = w - mMarginLeft - mMarginRight;
    mGraphHeight = h - mMarginTop - mMarginBottom;
    
    // Update mStepXWidth to display grid aligned on ruler ticks
    
    float interval_x = mCurrentMaxX - mCurrentMinX;
    float dx_per_pixel = interval_x / mGraphWidth;
    
    float step_x_min_width = 40.f;
    float dx_per_step = dx_per_pixel * step_x_min_width;
    
    bool limitFound = false;
    float limit = 0.001f;
    while(!limitFound)
    {
        if(dx_per_step <= limit)
        {
            dx_per_step = limit;
            limitFound = true;
        }
        limit *= 10.f;
    }
    
    mStepXWidth = dx_per_step / dx_per_pixel;
    
    // Update mStepYHeight
    
    float interval_y = mMaxY - mMinY;
    float dy_per_pixel = interval_y / mGraphHeight;
    
    mDyPerStep = dy_per_pixel * mStepYMinHeight;
    
    limitFound = false;
    limit = 0.001f;
    while(!limitFound)
    {
        if(mDyPerStep <= limit)
        {
            mDyPerStep = limit;
            limitFound = true;
        }
        limit *= 10.f;
    }
    
    mStepYHeight = mDyPerStep / dy_per_pixel;
    mStepYHeight = floorf(mStepYHeight * 10.f) / 10.f;
}

void GraphView::paint(QPainter& painter, int w, int h)
{
    if(mCurves.size() == 0)
    {
        painter.fillRect(0, 0, w, h, QColor(200, 200, 200));
        painter.setPen(QColor(100, 100, 100));
        painter.drawText(0, 0, w, h, Qt::AlignCenter, tr("Nothing to display"));
        return;
    }
    
    if(mBufferedImage.isNull())
    {
        mBufferedImage = QPixmap(w, h);
        QPainter p(&mBufferedImage);
        painter.setRenderHints(QPainter::Antialiasing);
        
        drawBackground(p);
        if(mShowGrid)
        {
            drawXAxis(p);
            drawYAxis(p);
        }
    }
    
    if(mBufferedImageWithGraphs.isNull())
    {
        mBufferedImageWithGraphs = mBufferedImage;
        QPainter p(&mBufferedImageWithGraphs);
        p.setRenderHints(QPainter::Antialiasing);
        drawCurves(p);
    }
    
    painter.drawPixmap(0, 0, mBufferedImageWithGraphs);
    
    if(mShowYValues)
    {
        drawYValues(painter);
    }
    if(mShowInfos)
    {
        QString infos = mInfos.join(" | ");
        painter.setPen(QColor(50, 50, 50));
        QFont font = painter.font();
        font.setPointSize(pointSize(11));
        painter.setFont(font);
        painter.drawText(mMarginLeft + 5, mMarginTop + 5, mGraphWidth - 10, 15, Qt::AlignRight | Qt::AlignTop, infos);
    }
    
    // Axis
    QColor axisCol(120, 120, 120);
    painter.setBrush(axisCol);
    painter.setPen(axisCol);
    painter.drawLine(mMarginLeft, mMarginTop, mMarginLeft, mMarginTop + mGraphHeight);
    painter.drawLine(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth - mMarginRight, mMarginTop + mGraphHeight);
    
    QPainterPath arrowTop;
    arrowTop.moveTo(mMarginLeft, mMarginTop);
    arrowTop.lineTo(mMarginLeft - 3, mMarginTop + 5);
    arrowTop.lineTo(mMarginLeft + 3, mMarginTop + 5);
    arrowTop.lineTo(mMarginLeft, mMarginTop);
    painter.drawPath(arrowTop);
    
    QPainterPath arrowRight;
    arrowRight.moveTo(mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
    arrowRight.lineTo(mMarginLeft + mGraphWidth - 5, mMarginTop + mGraphHeight - 3);
    arrowRight.lineTo(mMarginLeft + mGraphWidth - 5, mMarginTop + mGraphHeight + 3);
    arrowRight.lineTo(mMarginLeft + mGraphWidth, mMarginTop + mGraphHeight);
    painter.drawPath(arrowRight);
    
    
    // tip
    if(mTipVisible)
    {
        QPainterPath tipPath;
        tipPath.addRoundedRect(mTipRect, 5, 5);
        
        QFont font;
        font.setPointSizeF(pointSize(9));
        painter.setFont(font);
        
        painter.fillPath(tipPath, QColor(0, 0, 0, 180));
        painter.setPen(Qt::black);
        painter.drawPath(tipPath);
        painter.setPen(Qt::white);
        painter.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2), Qt::AlignCenter, "x : " + QString::number(mTipX));
        painter.drawText(mTipRect.adjusted(0, mTipRect.height()/2, 0, 0), Qt::AlignCenter, "y : " + QString::number(mTipY));
    }
}

void GraphView::drawCurves(QPainter& painter)
{
    // ---------------------------------------------------------------
    //  Curves
    // ---------------------------------------------------------------
    for(int curveIndex=0; curveIndex<mCurves.size(); ++curveIndex)
    {
        const GraphCurve& curve = mCurves[curveIndex];
        
        QPainterPath path;
        painter.setPen(curve.mPen);
        
        if(curve.mIsHorizontalLine)
        {
            float y = getYForValue(curve.mHorizontalValue);
            path.moveTo(mMarginLeft, y);
            path.lineTo(mMarginLeft + mGraphWidth, y);
            painter.strokePath(path, curve.mPen);
        }
        else if(curve.mIsVerticalLine)
        {
            float x = getXForValue(curve.mVerticalValue);
            path.moveTo(x, mMarginTop + mGraphHeight);
            path.lineTo(x, mMarginTop);
            painter.strokePath(path, curve.mPen);
        }
        else if(curve.mIsHorizontalSections)
        {
            float y = getYForValue(curve.mHorizontalValue);
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
            
            QMapIterator<float, float> iter(curve.mData);
            while(iter.hasNext())
            {
                iter.next();
                
                float valueX = iter.value();
                float valueY = iter.key();
                
                // vertical curves must be normalized (values from 0 to 1)
                // They are drawn using a 100px width
                float x = mMarginLeft + valueX * 100;
                float y = getYForValue(valueY, false);
                
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
            float last_x = 0;
            float last_y = 0;
            float last_value_y = 0;
            float last_value_x = 0;
            
            if(curve.mUseVectorData)
            {
                for(int x=0; x<curve.mDataVector.size(); ++x)
                {
                    float valueX = x;
                    float valueY = curve.mDataVector[x];
                    
                    if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                    {
                        float x = getXForValue(valueX, false);
                        float y = getYForValue(valueY, false);
                        
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
                /*QMapIterator<float, float> iter(curve.mData);
                while(iter.hasNext())
                {
                    iter.next();
                    float valueX = iter.key();
                    float valueY = iter.value();
                    
                    if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                    {
                        float x = getXForValue(valueX, false);
                        float y = getYForValue(valueY, false);
                        
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
                
                QMap<float, float> subData;
                if(mCurrentMinX != mMinX || mCurrentMaxX != mMaxX)
                {
                    QMapIterator<float, float> iter(curve.mData);
                    while(iter.hasNext())
                    {
                        iter.next();
                        float valueX = iter.key();
                        if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                            subData[valueX] = iter.value();
                    }
                }
                else
                {
                    subData = curve.mData;
                }
                QMap<float, float> lightMap;
                if(subData.size() > 4*mGraphWidth)
                {
                    int valuesPerPixel = subData.size() / (4*mGraphWidth);
                    //qDebug() << "Graph drawing : step = " << valuesPerPixel << ", data size = " << subData.size() << ", org data size = " << curve.mData.size();
                    QMapIterator<float, float> iter(subData);
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

                QMapIterator<float, float> iter(lightMap);
                while(iter.hasNext())
                {
                    iter.next();
                    float valueX = iter.key();
                    float valueY = iter.value();
                    
                    if(curve.mName == "G")
                    {
                        //qDebug() << valueX << " : " << valueY;
                    }
                    if(valueX >= mCurrentMinX && valueX <= mCurrentMaxX)
                    {
                        float x = getXForValue(valueX, false);
                        float y = getYForValue(valueY, false);
                        
                        if(index == 0)
                        {
                            path.moveTo(x, y);
                        }
                        else
                        {
                            if(curve.mIsHisto)
                            {
                                // histo bars must be centered around x value :
                                float dx2 = (x - last_x)/2.f;
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
                    const QMap<float, float>& data = mCurves[i].mData;
                    if(!abscissesWritten)
                    {
                        abscissesWritten = true;
                        QMapIterator<float, float> iter(data);
                        QStringList abscisses;
                        abscisses << "";
                        while(iter.hasNext())
                        {
                            iter.next();
                            abscisses << QString::number(iter.key());
                        }
                        rows.append(abscisses);
                    }
                    QMapIterator<float, float> iter(data);
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
                    const QMap<float, float>& data = mCurves[i].mData;
                    if(!abscissesWritten)
                    {
                        abscissesWritten = true;
                        QMapIterator<float, float> iter(data);
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
                        QMapIterator<float, float> iter(data);
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