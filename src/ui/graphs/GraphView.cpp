/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "GraphView.h"

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include "Painting.h"
#include "MainWindow.h"

#include <algorithm>

#include <QLocale>
#include <QColor>
#include <QWidget>
#include <QMouseEvent>
#include <QFileDialog>
#include <QApplication>
#include <QSvgGenerator>
#include <regex>

class StudyPeriodSettings;

GraphView::GraphView(QWidget* parent):
    GraphViewAbstract(parent),
    mStepMinWidth(3), // define when the minor scale on axis can appear
    mXAxisLine(true),
    mXAxisArrow(true),
    mXAxisTicks(true),
    mXAxisSubTicks(true),
    mXAxisValues(true),
    mYAxisLine(true),
    mYAxisArrow(false),
    mYAxisTicks(true),
    mYAxisSubTicks(true),
    mYAxisValues(true),
    mXAxisMode(AxisMode::eAllTicks),
    mYAxisMode(AxisMode::eAllTicks),
    mOverflowArrowMode(OverflowDataArrowMode::eNone),
    mUpdateTimer(new QTimer(this)),
    mAutoAdjustYScale(false),
    mShowInfos(false),
    mBackgroundColor(Qt::white),
    mThickness(1),
    mOpacity(100),
    mCanControlOpacity(false),
    mTipX(0.0),
    mTipY(0.0),
    mTipComment(""),
    mTipWidth(110.),
    mTipHeight(40.),
    mTipVisible(false),
    mUseTip(true),
    mCurves(),
    mZones(),
    mUnitFunctionX(nullptr),
    mUnitFunctionY(nullptr)

{
    mAxisToolX.mIsHorizontal = true;
    mAxisToolX.mShowArrow = true;
    mAxisToolY.mIsHorizontal = false;
    mAxisToolX.mShowArrow = true;
    mAxisToolY.mShowSubs = true; // false

    mTipRect.setTop(0);
    mTipRect.setLeft(0);
    mTipRect.setWidth(mTipWidth);
    mTipRect.setHeight(mTipHeight);

    setMouseTracking(true);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));

    setRangeY(0.0, 1.0);
    resetNothingMessage();

    //connect(this, &GraphView::signalCurvesThickness, this, &GraphView::updateCurvesThickness);

    connect(mUpdateTimer, &QTimer::timeout, this, &GraphView::autoUpdate);
    mUpdateTimer->setSingleShot(true); // Ne s'exécute qu'une fois

    mLastFastPaint.start();

}

GraphView::GraphView(const GraphView& graph, QWidget *parent):
    GraphViewAbstract(parent),
    mStepMinWidth(graph.mStepMinWidth), // define minorCount scale on axis
    mXAxisLine(graph.mXAxisLine),
    mXAxisArrow(graph.mXAxisArrow),
    mXAxisTicks(graph.mXAxisTicks),
    mXAxisSubTicks(graph.mXAxisSubTicks),
    mXAxisValues(graph.mXAxisValues),
    mYAxisLine(graph.mYAxisLine),
    mYAxisArrow(graph.mYAxisArrow),
    mYAxisTicks(graph.mYAxisTicks),
    mYAxisSubTicks(graph.mYAxisSubTicks),
    mYAxisValues(graph.mYAxisValues),
    mXAxisMode(graph.mXAxisMode),
    mYAxisMode(graph.mYAxisMode),
    mOverflowArrowMode(graph.mOverflowArrowMode),
    //mRendering(graph.mRendering),
    mAutoAdjustYScale(graph.mAutoAdjustYScale),
    mShowInfos(graph.mShowInfos),
    mBackgroundColor(graph.mBackgroundColor),
    mThickness(graph.mThickness),
    mOpacity(graph.mOpacity),
    mCanControlOpacity(graph.mCanControlOpacity),
    mTipX(graph.mTipX),
    mTipY(graph.mTipY),
    mTipComment(graph.mTipComment),
    mTipWidth(graph.mTipWidth),
    mTipHeight(graph.mTipHeight),
    mTipVisible(graph.mTipVisible),
    mUseTip(graph.mUseTip),
    mCurves(),
    mZones(),
    mUnitFunctionX(nullptr),
    mUnitFunctionY(nullptr)
{
    GraphViewAbstract::setParent(parent);
    mCurrentMinX = graph.mCurrentMinX;
    mCurrentMaxX = graph.mCurrentMaxX;

    mAxisToolX.mIsHorizontal = graph.mAxisToolX.mIsHorizontal;
    mAxisToolX.mShowArrow = graph.mAxisToolX.mShowArrow;
    mAxisToolY.mIsHorizontal = graph.mAxisToolY.mIsHorizontal;
    mAxisToolX.mShowArrow = graph.mAxisToolX.mShowArrow;
    mAxisToolY.mShowSubs = graph.mAxisToolY.mShowSubs;

    mTipRect.setTop(0);
    mTipRect.setLeft(0);
    mTipRect.setWidth(mTipWidth);
    mTipRect.setHeight(mTipHeight);

    setMouseTracking(true);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));

    setRangeY(0., 1.);
    resetNothingMessage();

    //connect(this, &GraphView::signalCurvesThickness, this, &GraphView::updateCurvesThickness);

    mCurves = graph.mCurves;
    mZones = graph.mZones;

   //mBufferBack = QPixmap();
}

void GraphView::copyFrom(const GraphView& graph)
{
    mStepMinWidth = graph.mStepMinWidth; // define minorCount scale on axis
    mXAxisLine = graph.mXAxisLine;
    mXAxisArrow = graph.mXAxisArrow;
    mXAxisTicks = graph.mXAxisTicks;
    mXAxisSubTicks = graph.mXAxisSubTicks;
    mXAxisValues = graph.mXAxisValues;
    mYAxisLine = graph.mYAxisLine;
    mYAxisArrow = graph.mYAxisArrow;
    mYAxisTicks = graph.mYAxisTicks;
    mYAxisSubTicks = graph.mYAxisSubTicks;
    mYAxisValues = graph.mYAxisValues;
    mXAxisMode = graph.mXAxisMode;
    mYAxisMode = graph.mYAxisMode;
    mOverflowArrowMode = graph.mOverflowArrowMode;
    //mRendering = graph.mRendering;
    mAutoAdjustYScale = graph.mAutoAdjustYScale;
    mShowInfos = graph.mShowInfos;
    mBackgroundColor =graph.mBackgroundColor;
    mThickness = graph.mThickness;
    mOpacity = graph.mOpacity;
    mCanControlOpacity = graph.mCanControlOpacity;
    mTipX = graph.mTipX;
    mTipY = graph.mTipY;
    mTipComment = graph.mTipComment;
    mTipWidth = graph.mTipWidth;
    mTipHeight = graph.mTipHeight;
    mTipVisible = graph.mTipVisible;
    mUseTip = graph.mUseTip;
    mCurrentMinX = graph.mCurrentMinX;
    mCurrentMaxX = graph.mCurrentMaxX;

    mAxisToolX.mIsHorizontal = graph.mAxisToolX.mIsHorizontal;
    mAxisToolX.mShowArrow = graph.mAxisToolX.mShowArrow;
    mAxisToolY.mIsHorizontal = graph.mAxisToolY.mIsHorizontal;
    mAxisToolX.mShowArrow = graph.mAxisToolX.mShowArrow;
    mAxisToolY.mShowSubs = graph.mAxisToolY.mShowSubs;

    mTipRect.setTop(0);
    mTipRect.setLeft(0);
    mTipRect.setWidth(mTipWidth);
    mTipRect.setHeight(mTipHeight);

    setMouseTracking(true);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));

    setRangeY(0., 1.);

    resetNothingMessage();

    mCurves = graph.mCurves;
    mZones = graph.mZones;

}

GraphView::~GraphView()
{
    mUnitFunctionX = nullptr;
    mUnitFunctionY = nullptr;
    mCurves.clear();
    mZones.clear();
    refPoints.clear();
    mInfos.clear();

}

void GraphView::adjustYScale()
{
    if (mAutoAdjustYScale) {
        qreal yMax = -HUGE_VAL;
        qreal yMin =  HUGE_VAL;

        for (const GraphCurve& curve : std::as_const(mCurves)) {
            if (curve.mVisible) {
                if (curve.isVertical()) { // used for the measurement in the calibration process
                    yMin = std::min(yMin, curve.mData.firstKey());
                    yMax = std::max(yMax, curve.mData.lastKey());

                } else if (!curve.isVerticalLine() && !curve.isHorizontalSections()) {

                    if (curve.isVectorData()) {
                        const std::vector<qreal> &subData = getVectorDataInRange(curve.mDataVector, mCurrentMinX, mCurrentMaxX, qreal (0.), qreal (curve.mDataVector.size()));
                        if (!subData.empty()) {
                            const double min = range_min_value(subData);
                            const double max = range_max_value(subData);
                            //if (!std::isfinite(min))
                                yMin = std::min(yMin, min);
                            //if (!std::isfinite(max)) {
                                 yMax = std::max(yMax, max);
                            //}
                        }

                    } else if (curve.isShape()) {
                        const auto& curveInf = curve.mShape.first;
                        const auto& curveSup = curve.mShape.second;
                        const QMap<qreal, qreal>& subDataInf = getMapDataInRange(curveInf, mCurrentMinX, mCurrentMaxX);
                        if (!subDataInf.empty())
                            yMin = std::min(yMin, map_min(subDataInf).value());

                        const QMap<qreal, qreal>& subDataSup = getMapDataInRange(curveSup, mCurrentMinX, mCurrentMaxX);
                        if (!subDataSup.empty())
                            yMax = std::max(yMax, map_max(subDataSup).value());

                    } else if (!curve.mData.empty()) {
                        yMin = std::min(yMin, map_min(curve.mData, mCurrentMinX, mCurrentMaxX).value());
                        yMax = std::max(yMax, map_max(curve.mData, mCurrentMinX, mCurrentMaxX).value());

                    } else if (!curve.mMap.isEmpty()) {// map
                        yMin = std::min(yMin, curve.mMap.minY());
                        yMax = std::max(yMax, curve.mMap.maxY());

                    }  else if (curve.isHorizontalLine()) {
                        yMin = std::min(yMin, curve.mHorizontalValue);
                        yMax = std::max(yMax, curve.mHorizontalValue);
                    }
                }

            }
        }

        for (auto& rf : refPoints) {
            yMin = std::min(yMin, rf.Ymin);
            yMax = std::max(yMax, rf.Ymax);
        }

        if (yMax >= yMin) {

            if (mAxisToolY.mSupport == AxisTool::AxisSupport::eMin_Max) {// || !mYAxisTicks) {
                if (yMax == yMin) {
                        if (yMax>0) {
                            yMin = 0.;
                        } else {
                            yMax = 0.;
                        }
                }

            } else if (mAxisToolY.mSupport == AxisTool::AxisSupport::eAllways_Positive) {// || !mYAxisTicks) {
                        yMin = 0.;


            } else if (mAxisToolY.mSupport == AxisTool::AxisSupport::eAllways_Negative) {// || !mYAxisTicks) {
                yMax = 0.;


            }  else {
                Scale yScale;
                yScale.findOptimal(yMin, yMax, 7);
                setYScaleDivision(yScale);

                yMin = yScale.min;
                yMax = yScale.max;

            }
            setRangeY(yMin, yMax);

        }
    }
}

void GraphView::zoomX(const type_data min, const type_data max)
{
    if (mCurrentMinX != min || mCurrentMaxX != max || mMinY>=mMaxY || mAutoAdjustYScale) {
        mCurrentMinX = min;
        mCurrentMaxX = max;

        mAxisToolX.updateValues(width(), 10.0, min, max);
        
        adjustYScale();

        if ((width() <= 0) || (height() <= 0))
            return;

        repaintGraph();

    }

}

void GraphView::changeXScaleDivision (const Scale& sc)
{
    changeXScaleDivision(sc.mark, sc.tip);
}

void GraphView::changeXScaleDivision (double major, int minor)
{
    setXScaleDivision(major, minor);
    repaintGraph();
    update();
}

/* ------------------------------------------------------
 *  Options
 * ------------------------------------------------------*/


void GraphView::setYAxisMode(AxisMode mode)
{
    if (mYAxisMode != mode) {
        mYAxisMode = mode;

       // mAxisToolY.mMinMaxOnly = (mYAxisMode == eMinMax);

        switch (mYAxisMode) {
        case AxisMode::eMinMax:
            showYAxisValues(false);
            showYAxisTicks(false);
            showYAxisSubTicks(false);
            mYAxisArrow = false;
            mAxisToolY.mShowText = true;
            break;
        case AxisMode::eMinMaxHidden:
            showYAxisValues(false);
            showYAxisTicks(false);
            showYAxisSubTicks(false);
            mYAxisArrow = false;
            mAxisToolY.mShowText = false;
            break;
        case AxisMode::eHidden:
            showYAxisValues(false);
            showYAxisTicks(false);
            showYAxisSubTicks(false);
            mYAxisArrow = false;
            break;
        default:
            showYAxisValues(true);
            showYAxisTicks(true);
            showYAxisSubTicks(true);
            mYAxisArrow = false;
            mAxisToolY.mShowText = false;
            break;
        }

     }
}



void GraphView::setGraphFont(const QFont& font)
{
    setFont(font);
    repaintGraph();
}


/**
 * @brief GraphView::updateCurvesThickness slot which modify thickness
 * @param value
 */
void GraphView::updateCurvesThickness(int value)
{
    mThickness = value;
    repaintGraph();
}



void GraphView::updateCurvesOpacity(int value)
{
    mOpacity = value;
    repaintGraph();
}





/* ------------------------------------------------------
 Curves & Zones
 ------------------------------------------------------ */

void GraphView::add_curve(const GraphCurve& curve)
{
    mCurves.append(curve);
}

void GraphView::removeCurve(const QString& name)
{
    for (int i=0; i<mCurves.size(); ++i) {
        if (mCurves.at(i).mName == name) {
            mCurves.removeAt(i);
            break;
        }
    }
    mCurves.squeeze();
}

void GraphView::removeAllCurves()
{
    mCurves.clear();
    mZones.clear();
    mVectorCache = QPicture();
    mCacheValid = false;
    mRasterCache = QPixmap();
    mIsScrolling = false;

}

void GraphView::reserveCurves(const int size)
{
    mCurves.reserve(size);
}

void GraphView::setCurveVisible(const QString& name, const bool visible)
{
    std::ranges::for_each(mCurves, [name, visible](auto& c) {
        if (c.mName == name && c.mVisible != visible) {
            c.mVisible = visible;
        } });
    repaintGraph();
}

void GraphView::setCurveVisible(const QStringList& names, const bool visible)
{
    std::ranges::for_each(mCurves, [&names, visible](auto& c) {
        const bool shouldBeVisible = names.contains(c.mName) ? visible : false;
        if (c.mVisible != shouldBeVisible) {
            c.mVisible = shouldBeVisible;
        }
    });
    repaintGraph();
}

void GraphView::setCurveVisible(std::initializer_list<const char*> names, const bool visible)
{
    std::ranges::for_each(mCurves, [&names, visible](auto& c) {
        const bool shouldBeVisible = std::ranges::any_of(names, [&c](const char* name) {
            return c.mName == name;
        }) ? visible : false;
        if (c.mVisible != shouldBeVisible) {
            c.mVisible = shouldBeVisible;
        }
    });
    repaintGraph();
}

GraphCurve* GraphView::getCurve(const QString& name)
{
    QList<GraphCurve>::Iterator result = std::ranges::find_if(mCurves.begin(), mCurves.end(), [name](auto c) {return c.mName == name;});
    return (result!= mCurves.end()) ? &(*result) : nullptr;
}

const QList<GraphCurve>& GraphView::getCurves() const
{
    return mCurves;
}

int GraphView::numCurves() const
{
    return (int)mCurves.size();
}

void GraphView::add_zone(const GraphZone& zone)
{
    mZones.append(zone);
}

void GraphView::set_points_visible(const QString& name, const bool visible)
{
    for (auto&& ref : refPoints) {
        if (ref.name == name) {
            ref.setVisible(visible);
        }
    }

}

//  Mouse events & Tool Tip
void GraphView::enterEvent(QEnterEvent* e)
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
    qreal x = e->pos().x();
    qreal y = e->pos().y();

    if (mUseTip && x >= mMarginLeft && x <= (mMarginLeft + mGraphWidth) && y >= mMarginTop && y <= (mMarginTop + mGraphHeight))  {
        mTipVisible = true;
        QRectF old_rect = mTipRect;

        int cursorW = 15;
        int cursorH = 15;

        if (mMarginLeft + mGraphWidth - x <= (mTipWidth + cursorW))
            x -= mTipWidth;
        else
            x += cursorW;

        if (mMarginTop + mGraphHeight - y <= (mTipHeight + cursorH))
            y -= mTipHeight;
        else
            y += cursorH;

        mTipRect.setLeft(x - 0.5);
        mTipRect.setTop(y - 0.5);

        mTipRect.setWidth(mTipWidth);
        mTipRect.setHeight(mTipHeight);

        mTipX = getValueForX(e->position().x() );
        mTipY = getValueForY(e->position().y());

        const qreal Xpos = getValueForX(e->position().x());
        const qreal Ypos = getValueForY(e->position().y());

        const qreal errXp = getValueForX(e->position().x() + 3);
        const qreal errXm = getValueForX(e->position().x() - 3);

        const qreal errYp = getValueForY(e->position().y() - 3);
        const qreal errYm = getValueForY(e->position().y() + 3);

        mTipComment = "";
        for (auto&& ref : refPoints) {
            if (ref.isVisible()) {
                const qreal Xmid = (ref.Xmin + ref.Xmax) / 2.;
                const qreal Ymid = (ref.Ymin + ref.Ymax) / 2.;

                if ((ref.Xmin<=Xpos && Xpos<=ref.Xmax && Ymid<=errYp && errYm<=Ymid)
                    || (Xmid<=errXp && errXm<=Xmid && Ymid<=errYp && errYm<=Ymid )
                    || (ref.Ymin<=Ypos && Ypos<=ref.Ymax && Xmid<=errXp && errXm<=Xmid) )  {
                    mTipComment = ref.comment;
                }
            }

        }

        update(old_rect.adjusted(-20, -20, 20, 20).toRect());


    } else
        mTipVisible = false;

    update(mTipRect.adjusted(-20, -20, 20, 20).toRect());

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

void GraphView::resizeEvent(QResizeEvent* )
{
    if (width() == 0 || height() == 0)
        return;

    repaintGraph();
}

void GraphView::updateGraphSize(qreal w, qreal h)
{
#ifdef Q_OS_MAC
     mBottomSpacer = 0.02 * h;
#else
     mBottomSpacer = 0.04 * h;
#endif

    mGraphWidth = std::max(0.0, w - mMarginLeft - mMarginRight);
    mGraphHeight = std::max(0.0, h - mMarginTop - mMarginBottom - mBottomSpacer);
    mAxisToolX.updateValues(int (mGraphWidth), int (mStepMinWidth), mCurrentMinX, mCurrentMaxX);
    mAxisToolY.updateValues(int (mGraphHeight), 12, mMinY, mMaxY);
}


void GraphView::repaintGraph()
{
    mVectorDirty = true;
    mCacheValid  = false;
    update();                  // coalescé : 1 seul paintEvent
}

void GraphView::rebuildCaches()
{
    if ((width() <= 0) || (height() <= 0))
        return;

    updateGraphSize(width(), height());

    if ((mGraphWidth <= 0) || (mGraphHeight <= 0))
        return;

    // 1. Dessiner en vectoriel
    mVectorCache = QPicture();
    QRect rect = QRect(0, 0, mGraphWidth, mGraphHeight);
    mVectorCache.setBoundingRect(rect); // IMPORTANT : définir la zone avant de dessiner

    /* ----------------------------------------------------
     *  Nothing to draw !
     * ----------------------------------------------------*/
    if (mCurves.size() == 0 && mZones.size() == 0 && refPoints.size() == 0) {
        QPainter p(&mVectorCache);
        p.setFont(font());
        p.fillRect(0, 0, width(), height(), mBackgroundColor);
        p.setPen(QColor(100, 100, 100));
        p.drawText(0, 0, width(), height(), Qt::AlignCenter, mNothingMessage);
        p.end();

    } else {
        paintToDevice(&mVectorCache);
        mVectorCache.setBoundingRect(rect); // Confirmer la zone
    }

    // 2. Créer le cache raster haute résolution
    //updateRasterCache();
    mCacheValid = false;
    mVectorDirty = false;

}


void GraphView::updateRasterCache()
{
#ifdef DEBUG
    const qreal dpr = devicePixelRatioF();// /5000.0;
#else
    const qreal dpr = devicePixelRatioF();
#endif
    mRasterCache = QPixmap(size() * dpr);
    mRasterCache.setDevicePixelRatio(dpr);
    mRasterCache.fill(Qt::transparent);

    QPainter p(&mRasterCache);
    p.drawPicture(0, 0, mVectorCache);
}

void GraphView::drawTooltip()
{
    QPainterPath tipPath;
    if (mTipRect.width() < 2) {
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

    if (!mTipXLab.isEmpty() && !mTipYLab.isEmpty()) {
        if (mTipComment.isEmpty()) {
            if (mUnitFunctionX)
                p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()*0.5),
                           Qt::AlignCenter, mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
            else
                p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2),
                           Qt::AlignCenter, mTipXLab + stringForLocal(mTipX));

            if (mUnitFunctionY)
                p.drawText(mTipRect.adjusted(0, mTipRect.height()*0.5, 0, 0),
                           Qt::AlignCenter, mTipYLab + stringForLocal(mUnitFunctionY(mTipY)));
            else
                p.drawText(mTipRect.adjusted(0, mTipRect.height()*0.5, 0, 0),
                           Qt::AlignCenter, mTipYLab + stringForLocal(mTipY));

        } else {
            if (mUnitFunctionX)
                p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()*0.666),
                           Qt::AlignCenter, mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
            else
                p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()*0.666),
                           Qt::AlignCenter, mTipXLab + stringForLocal(mTipX));

            if (mUnitFunctionY)
                p.drawText(mTipRect.adjusted(0, mTipRect.height()*0.333, 0, -mTipRect.height()*1./3.),
                           Qt::AlignCenter, mTipYLab + stringForLocal(mUnitFunctionY(mTipY)));
            else
                p.drawText(mTipRect.adjusted(0, mTipRect.height()*0.333, 0, -mTipRect.height()*1./3.),
                           Qt::AlignCenter, mTipYLab + stringForLocal(mTipY));

            p.drawText(mTipRect.adjusted(0, mTipRect.height()*0.666, 0, 0),
                       Qt::AlignCenter, " ⇨ " + mTipComment);
        }

    } else if (!mTipXLab.isEmpty()) {
        if (mTipComment.isEmpty()) {
            if (mUnitFunctionX)
                p.drawText(mTipRect, Qt::AlignCenter,
                           mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
            else
                p.drawText(mTipRect, Qt::AlignCenter,
                           mTipXLab + stringForLocal(mTipX));

        } else {
            if (mUnitFunctionX)
                p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()*0.5),
                           Qt::AlignCenter, mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
            else
                p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2),
                           Qt::AlignCenter, mTipXLab + stringForLocal(mTipX));

            p.drawText(mTipRect.adjusted(0, mTipRect.height()*0.5, 0, 0),
                       Qt::AlignCenter, " ⇨ " + mTipComment);
        }

    } else if (!mTipYLab.isEmpty()) {
        if (mUnitFunctionY)
            p.drawText(mTipRect, Qt::AlignCenter,
                       mTipYLab + stringForLocal(mUnitFunctionY(mTipY)));
        else
            p.drawText(mTipRect, Qt::AlignCenter,
                       mTipYLab + stringForLocal(mTipY));
    }
}


void GraphView::paintEvent(QPaintEvent *)
{
    if (mVectorDirty)
        rebuildCaches();

    const qint64 sinceLastFast = mLastFastPaint.elapsed();

    // Entrée en mode scroll
    if (sinceLastFast < 100) {
        if (!mIsScrolling) {
            mIsScrolling = true;
            mCacheValid = false;
           // qDebug() << "[GraphView::paintEvent] -> Passage en MODE SCROLL (sinceLastFast =" << sinceLastFast << "ms)";
        }
    }
    // Retour au mode qualité
    else if (mIsScrolling && sinceLastFast > 200) {
        mIsScrolling = false;
        mCacheValid = false;
       // qDebug() << "[GraphView::paintEvent] -> Retour en MODE QUALITE (sinceLastFast =" << sinceLastFast << "ms)";
    }

    // Log du temps entre deux paint events
    //qDebug() << "[GraphView::paintEvent] paintEvent: sinceLastFast =" << sinceLastFast << "ms, scrolling =" << mIsScrolling;

    // Met à jour le marqueur de temps à chaque paint
    mLastFastPaint.restart();

    QPainter painter(this);

    if (mIsScrolling) {
        // Mode scroll : affichage rapide via cache raster
        if (!mCacheValid) {
            //qDebug() << "[GraphView::paintEvent ] Régénération du cache RASTER";
            mRasterCache = QPixmap(size());
            mRasterCache.fill(Qt::transparent);

            QPainter cachePainter(&mRasterCache);
            cachePainter.setRenderHint(QPainter::Antialiasing);
            cachePainter.drawPicture(0, 0, mVectorCache);
            mCacheValid = true;
        }
        painter.drawPixmap(0, 0, mRasterCache);
        mUpdateTimer->start(500); // Démarrer le timer de mise à jour
    } else {
        // Mode qualité : dessin vectoriel
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPicture(0, 0, mVectorCache);
        mUpdateTimer->stop(); // Arrêter le timer si on sort du mode scroll
    }

    painter.end();

    // Tooltip par-dessus
    if (mTipVisible && (!mTipXLab.isEmpty() || !mTipYLab.isEmpty()))
        drawTooltip();
}

/**
 * @brief Draw graphics
 */
void GraphView::paintToDevice(QPaintDevice* device)
{
    QPainter painter(device);
    if (!painter.isActive()) {
        qDebug() << "[GraphView::paintToDevice] device not active!";
        return; // Gestion d'erreur
    }
    const QRect rect = QRect(0, 0, mGraphWidth, mGraphHeight);
    painter.setFont(this->font());
    painter.setViewport(rect);  // Définir le viewport
    painter.setRenderHints(QPainter::Antialiasing);

    /* ----------------------- Background ----------------------*/

    painter.fillRect(this->rect(), mBackgroundColor);

    QFont font = painter.font();

    /* ----------------------------------------------------
     *  Horizontal axis
     * ----------------------------------------------------*/
    QPen pen = QPen(Qt::black);//, 1);

    if (!mLegendX.isEmpty() && mXAxisValues) {
        QRectF tr(mMarginLeft, mGraphHeight + mMarginTop - mMarginBottom, mGraphWidth, mMarginBottom);
        painter.setPen(Qt::black);
        painter.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, mLegendX);
    }
    pen.setWidth(pen.width() * mThickness);
    painter.setPen(pen);

    if (mXAxisLine) {
        mAxisToolX.mShowArrow = mXAxisArrow;
        mAxisToolX.mShowSubs = mXAxisTicks;
        mAxisToolX.mShowSubSubs = mXAxisSubTicks;
        mAxisToolX.mShowText = mXAxisValues;

        mAxisToolX.updateValues(int (mGraphWidth), int (mStepMinWidth), mCurrentMinX, mCurrentMaxX);
        mAxisToolX.paint(painter, QRectF(mMarginLeft, mMarginTop + mGraphHeight , mGraphWidth , mMarginBottom), -1.,mUnitFunctionX);
    }
    /* ----------------------------------------------------
     *  Vertical axis
     * ----------------------------------------------------*/
    adjustYScale();

    if (mYAxisLine) {
        mAxisToolY.mShowArrow = mYAxisArrow;
        mAxisToolY.mShowSubs = mYAxisTicks;
        mAxisToolY.mShowSubSubs = mYAxisSubTicks;

       /* No more used since v3.2.8
        * if (mAutoAdjustYScale && mYAxisMode == eHidden && mShowInfos) {
            const QString minMaxText = QString(tr( "Min = %1  /  Max = %2")).arg(stringForLocal(mMinY), stringForLocal(mMaxY));
            mInfos.clear();
            mInfos.append(minMaxText);
        */

        mAxisToolY.updateValues(int (mGraphHeight), int (mStepMinWidth), mMinY, mMaxY);
        mAxisToolY.paint(painter, QRectF(0, mMarginTop, mMarginLeft, mGraphHeight), -1., mUnitFunctionY);

     }


    /* ---------------------- Zones --------------------------*/

    for (auto&& zone : mZones) {
        QRectF r(getXForValue(zone.mXStart),
                mMarginTop,
                getXForValue(zone.mXEnd) - getXForValue(zone.mXStart),
                mGraphHeight);

        if (r.width()>1) {
            painter.save();
            painter.setClipRect(r);

            painter.fillRect(r, zone.mColor);
            painter.setPen(zone.mColor.darker());
            QFont fontZone = painter.font();
            fontZone.setWeight(QFont::Black);
            fontZone.setPointSizeF(pointSize(20));
            painter.setFont(fontZone);
            painter.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, zone.mText);
            painter.restore();
        }
    }

    /* mOverflowArrowMode, draw a arrow on the rigth or on the left
     * if the data are over the window (current min, current max)  */

     if (mOverflowArrowMode != OverflowDataArrowMode::eNone) {
         qreal arrowSize (7.);
         const qreal yo (mGraphHeight / 2.0) ;
         const QColor gradColDark(150, 150, 150);
         const QColor gradColLigth(190, 190, 190);

         if (mOverflowArrowMode == OverflowDataArrowMode::eBothOverflow || mOverflowArrowMode == OverflowDataArrowMode::eUnderMin) {

             type_data maxData = - std::numeric_limits<type_data>::max();
             for (auto& curve : mCurves)
                 if (curve.mVisible && curve.mData.size()>0)
                    maxData = std::max(maxData, curve.mData.lastKey());

                 else if (curve.mVisible && curve.isVerticalLine())
                     maxData = std::max(maxData, curve.mVerticalValue);

                 else if (curve.mVisible && curve.isHorizontalSections())
                     for (auto& section : curve.mSections )
                         maxData = std::max(maxData, section.second);


             if ( maxData < mCurrentMinX) {
                 const qreal xl = mMarginLeft + 2*arrowSize;
                 const QPolygonF LeftTriangle (std::initializer_list<QPointF>({ QPointF(xl - arrowSize*1.5, yo),
                                                                      QPointF(xl, yo - arrowSize),
                                                                      QPointF(xl , yo + arrowSize) }));
                 QLinearGradient linearGradientL(xl - arrowSize, yo, xl, yo);
                         linearGradientL.setColorAt(0, gradColDark );
                         linearGradientL.setColorAt(1, gradColLigth );
                 painter.setBrush(linearGradientL);
                 painter.setPen(Qt::NoPen);
                 painter.drawPolygon(LeftTriangle);
             }

         }
         if (mOverflowArrowMode == OverflowDataArrowMode::eBothOverflow || mOverflowArrowMode == OverflowDataArrowMode::eOverMax) {
             type_data minData = std::numeric_limits<type_data>::max();;

             for (auto& curve : mCurves) {
                 if (curve.mVisible && curve.mData.size() > 0)
                    minData = std::min(minData, curve.mData.firstKey());

                 else if (curve.mVisible && curve.isVerticalLine())
                     minData = std::min(minData, curve.mVerticalValue);

                 else if (curve.mVisible && curve.isHorizontalSections())
                     for (auto& section : curve.mSections )
                         minData = std::min(minData, section.first);
             }

             if (mCurrentMaxX < minData) {
                 const qreal xr = mGraphWidth + mMarginLeft - 2.0 * arrowSize;
                 const QPolygonF RigthTriangle (std::initializer_list<QPointF>({ QPointF(xr + arrowSize*1.5, yo),
                                                                      QPointF(xr, yo - arrowSize),
                                                                      QPointF(xr , yo + arrowSize) }));

                 QLinearGradient linearGradientR(xr, yo, xr + arrowSize, yo);
                         linearGradientR.setColorAt(0, gradColLigth );
                         linearGradientR.setColorAt(1, gradColDark);

                 painter.setBrush(linearGradientR);
                 painter.setPen(Qt::NoPen);

                 painter.drawPolygon(RigthTriangle);
             }
         }

     }



    /* ----------------------------------------------------
     *  Curves
     * ----------------------------------------------------*/
    drawCurves(painter);


    /* ----------------------------------------------------
     *  Graph specific infos at the top right
     * ----------------------------------------------------*/
    // Used by Multicalibration

    if (mShowInfos) {
        QFontMetrics fm (font);
        painter.setFont(font);
        painter.setPen(Painting::borderDark);
        int y = 0;
        int lineH (fm.height());
        for (const QString& info : std::as_const(mInfos)) {
            painter.drawText(int (1.2 * mMarginLeft), int (mMarginTop  + y), int (mGraphWidth - 1.2*mMarginLeft -mMarginRight), lineH, Qt::AlignLeft | Qt::AlignBottom, info);
            y += lineH;
        }

    }
    painter.end();
}

// Implémentation de la méthode d'update
void GraphView::autoUpdate()
{
    //qDebug() << "[GraphView::autoUpdate] -> Retour en MODE QUALITE";
    update(); // Demande une mise à jour de l'affichage
}

QPainterPath GraphView::makePath (const QMap<double, double>& map, const bool showBorder) const
{
    QPainterPath path;
    if (map.isEmpty())
        return path;

    if ( map.lastKey() <= mCurrentMinX || map.firstKey() >= mCurrentMaxX)
        return path;



    QMap<double, double>::const_iterator iFirst = map.begin();
    while (iFirst != map.end() && ( iFirst.key()< mCurrentMinX || iFirst.key()> mCurrentMaxX ||  iFirst.value()< mMinY || iFirst.value()> mMaxY) ) {
        ++iFirst;
    }


    // ******* 7 cas
    if ( iFirst == map.end()) {
        return path;
    }

    qreal xPrev, yPrev;
    if ( iFirst != map.begin()) {
        const auto iPrev = std::prev(iFirst);
        if (iPrev.key() < mCurrentMinX) {

            if (iPrev.value() > mMaxY) { //

                xPrev = interpolate(mMaxY, iPrev.value(), iFirst.value(), iPrev.key(), iFirst.key());
                if (xPrev < mCurrentMinX) { // OK
                    xPrev = mCurrentMinX;
                    yPrev = interpolateValueInQMap(mCurrentMinX, map);

                } else { // OK
                    yPrev = mMaxY;
                }


            } else if (iPrev.value() < mMinY) { //
                xPrev = interpolate(mMinY, iPrev.value(), iFirst.value(), iPrev.key(), iFirst.key());
                if (xPrev < mCurrentMinX) { // OK
                    xPrev = mCurrentMinX;
                    yPrev = interpolateValueInQMap(mCurrentMinX, map);

                } else { // OK
                    yPrev = mMinY;
                }


            } else { //OK
                xPrev = mCurrentMinX;
                yPrev = interpolateValueInQMap(mCurrentMinX, map);
            }
        } else {
            if (iPrev.value() > mMaxY) { // 2-
                xPrev = interpolate(mMaxY, iPrev.value(), iFirst.value(), iPrev.key(), iFirst.key());
                yPrev = mMaxY;


            } else if (iPrev.value() < mMinY) { // 4 -
                xPrev = interpolate(mMinY, iPrev.value(), iFirst.value(), iPrev.key(), iFirst.key());
                yPrev = mMinY;

            } else { // 3 OK
                xPrev = iFirst.key();
                yPrev = iFirst.value();
            }
        }
    }
    else { // 6 - no next
        xPrev = iFirst.key();
        yPrev = iFirst.value();
    }

    path.moveTo(getXForValue(xPrev, true), getYForValue(yPrev, true));

    // *****

    decltype(iFirst) lastGoodIter = map.end(); // repere le dernier point visible
    bool movePoint = false;


    if (showBorder) {
        while (iFirst != map.end()) {
            if ( mCurrentMinX <= iFirst.key() && iFirst.key() <= mCurrentMaxX ) {

                        if (mMinY <= iFirst.value()  && iFirst.value()<= mMaxY  ) {
                            path.lineTo(getXForValue(iFirst.key(), true), getYForValue(iFirst.value(), true));
                            lastGoodIter = iFirst;

                        } else {
                            path.moveTo(getXForValue(iFirst.key(), true), getYForValue(iFirst.value(), true));
                        }

            }

            ++iFirst;
        }
        --iFirst;

    } else {
        while (iFirst != map.end()) {
            if ( mCurrentMinX <= iFirst.key() && iFirst.key() <= mCurrentMaxX ) {
                if (mMinY <= iFirst.value()  && iFirst.value() <= mMaxY) {
                    if (movePoint) {

                        if (iFirst != map.begin()) {
                            auto iPrev = std::prev(iFirst);
                            if (iPrev.value() > mMaxY) { // 2-
                                xPrev = interpolate(mMaxY, iPrev.value(), iFirst.value(), iPrev.key(), iFirst.key());
                                yPrev = mMaxY;


                            } else if (iPrev.value() < mMinY) { // 4 -
                                xPrev = interpolate(mMinY, iPrev.value(), iFirst.value(), iPrev.key(), iFirst.key());
                                yPrev = mMinY;

                            }
                            path.moveTo(getXForValue(xPrev, true), getYForValue(yPrev, true));
                            path.lineTo(getXForValue(iFirst.key(), true), getYForValue(iFirst.value(), true));
                        } else {
                            path.moveTo(getXForValue(iFirst.key(), true), getYForValue(iFirst.value(), true));
                        }


                    } else {
                        path.lineTo(getXForValue(iFirst.key(), true), getYForValue(iFirst.value(), true));
                        lastGoodIter = iFirst;
                    }
                    movePoint = false;

                } else {
                    movePoint = true;
                }
            }
            ++iFirst;
        }
        --iFirst;
    }
    // The Last point does not exist, mCurrentMaxX does not exist in the table, it must be interpolated


    if (lastGoodIter != iFirst && lastGoodIter != map.end()) {
          // ******* 7 cas
        auto iNext = std::next(lastGoodIter);
        qreal xNext, yNext;
        if ( iNext != map.end()) {

            if (iNext.key() < mCurrentMaxX) {
                if (iNext.value() > mMaxY) { // 1 - =2.a
                    xNext = interpolate(mMaxY, lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
                    yNext = mMaxY;

                } else if (iNext.value() < mMinY) { // 5 - =4.a
                    xNext = interpolate(mMinY, lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
                    yNext = mMinY;

                } else {
                    xNext = iNext.key();
                    yNext = iNext.value();
                }
            } else {
                if (iNext.value() > mMaxY) { // 2-
                    xNext = interpolate(mMaxY, lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
                    if (xNext < mCurrentMaxX) { // 2.a - = 1
                        yNext = mMaxY;

                    } else { // 2.b
                        xNext = mCurrentMaxX;
                        yNext = interpolateValueInQMap(mCurrentMaxX, map);
                    }

                } else if (iNext.value() < mMinY) { // 4 -
                    xNext = interpolate(mMinY, lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
                    if (xNext < mCurrentMaxX) { // 4.a - = 5
                        yNext = mMinY;

                    } else { // 4.b
                        xNext = mCurrentMaxX;
                        yNext = interpolateValueInQMap(mCurrentMaxX, map);
                    }

                } else { // 3
                    xNext = mCurrentMaxX;
                    yNext = interpolateValueInQMap(mCurrentMaxX, map);
                }
            }

        } else { // 6 - no next
            xNext = iNext.key();
            yNext = iNext.value();
        }

        path.lineTo(getXForValue(xNext, true), getYForValue(yNext, true));
    // *****
    }
    return path;
}

void GraphView::drawCurves(QPainter& painter)
{
    if ((mGraphWidth <= 0) || (mGraphHeight <= 0))
        return;

    painter.save();
    // Draw curves inside axis only (not in margins!)
    //painter.setClipRect(int (mMarginLeft), int (mMarginTop), int(mGraphWidth), int (mGraphHeight));
    { // RAII guard, garantie de passer par painter.restore()
        for (const GraphCurve& curve : std::as_const(mCurves)) {
            if (curve.mVisible) {
                QPainterPath path;

                QPen pen = curve.mPen;
                pen.setWidth(pen.width() * mThickness);
                painter.setPen(pen);

                QBrush brush = curve.mBrush;

                if (mCanControlOpacity) {
                    QColor c = brush.color();
                    c.setAlpha(curve.mBrush.color().alpha()*mOpacity / 100);
                    brush.setColor(c);
                }

                painter.setBrush(brush);

                if (curve.isHorizontalLine()) {
                    const qreal y = getYForValue(curve.mHorizontalValue);
                    path.moveTo(mMarginLeft, y);
                    path.lineTo(mMarginLeft + mGraphWidth, y);

                    painter.strokePath(path, pen);

                } else if (curve.isVerticalLine()) {
                    const qreal x = getXForValue(curve.mVerticalValue, false);
                    path.moveTo(x, mMarginTop + mGraphHeight);
                    path.lineTo(x, mMarginTop);

                    painter.strokePath(path, pen);

                } else if (curve.isHorizontalSections()) { // used for Bound and Unif-Typo

                    const qreal y1 = getYForValue(mMaxY);
                    const qreal y0 = getYForValue(mMinY);
                    path.moveTo(mMarginLeft, y0);

                    for (auto& section : curve.mSections ) {
                        const qreal x1 = getXForValue(section.first, false);
                        const qreal x2 = getXForValue(section.second, false);

                        path.lineTo(x1, y0);
                        path.lineTo(x1, y1);
                        path.lineTo(x2, y1);
                        path.lineTo(x2, y0);
                    }
                    path.lineTo(mMarginLeft + mGraphWidth, y0);

                    painter.setClipRect(mMarginLeft, mMarginTop, mGraphWidth, mGraphHeight);
                    painter.fillPath(path, brush);
                    painter.strokePath(path, pen);


                } else if (curve.isTopLineSections()) { // Used for credibility, phase time range
                    const qreal y1 =  mMarginTop - curve.mPen.width();

                    for (auto& section : curve.mSections ) {
                        const type_data s1 = section.first;
                        const type_data s2 = section.second;

                        if (s1<mCurrentMaxX && s2>mCurrentMinX) {
                            const qreal x1 = getXForValue(s1, true);
                            const qreal x2 = getXForValue(s2, true);

                            painter.drawLine(QPointF(x1, y1),QPointF(x2, y1));
                        }
                    }

                } else if (curve.isVertical()) {
                    path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);

                    int index (0);
                    qreal last_y (0.);

                    QMap<type_data, type_data>::const_iterator iter = curve.mData.cbegin();

                    while (iter != curve.mData.cend()) {
                        type_data valueX = iter.value();
                        type_data valueY = iter.key();

                        // vertical curves must be normalized (values from 0 to 1)
                        // They are drawn using a 100px width
                        qreal x = mMarginLeft + valueX * 100.;
                        qreal y = getYForValue(valueY, false);

                        y = std::clamp(y, mMarginTop, mMarginTop + mGraphHeight);

                        if (index == 0) {
                            path.lineTo(x, y);

                        } else {
                            if (curve.isHisto())
                                path.lineTo(x, last_y);
                            path.lineTo(x, y);
                        }
                        last_y = y;
                        ++index;
                        ++iter;
                    }
                    path.lineTo(mMarginLeft, mMarginTop);
                    painter.drawPath(path);

                } else if (curve.isCurveMap()) {
                    /* -------------------------- Map ---------------------------*/
                    drawMap(curve, painter);


                } else if (curve.isShape()) {

                    drawShape(curve, painter);


                } else if (curve.isFunction()) {

                    path = makePath(curve.mData, false);
                    painter.drawPath(path);

                } else if (curve.isDensityCurve()) {

                    drawDensity(curve, painter);

                } else { // it's horizontal curve

                    path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);

                    qreal last_y = 0.;

                    if (curve.isVectorData()) {
                        // Down sample vector
                        if (curve.mDataVector.empty()) continue;
                        // return;

                        std::vector<type_data> subData = getVectorDataInRange(curve.mDataVector, mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);

                        // répartition uniforme
                        const auto n = mGraphWidth;
                        const auto m = subData.size();
                        std::vector<type_data> lightData;
                        if (m > n ) {

                            for (size_t i = 0; i < n; ++i) {
                                size_t idx = i * (m - 1) / (n - 1);
                                lightData.push_back(subData[idx]);
                            }
                        } else {
                            lightData = subData;
                        }

                        bool isFirst = true;

                        const type_data xStep = (mCurrentMaxX - mCurrentMinX) / type_data(lightData.size() - 1);


                        for (size_t i = 0; i<lightData.size(); ++i) {
                            // Use "dataStep" only if lightData is different of subData !
                            //const type_data valueX = mCurrentMinX + ((dataStep > 1) ? i * dataStep : (type_data)i);
                            const type_data valueX = mCurrentMinX + i * xStep;
                            const type_data valueY = lightData.at(i);

                            if (valueX >= mCurrentMinX && valueX <= mCurrentMaxX && mMinY<= valueY && valueY <= mMaxY) {

                                qreal x = getXForValue(valueX, false);
                                qreal y = getYForValue(valueY, false);

                                if (isFirst) {
                                    path.moveTo(x, y);
                                    isFirst=false;

                                } else {
                                    if (curve.isHisto())
                                        path.lineTo(x, last_y);
                                    path.lineTo(x, y);
                                }

                                last_y = y;

                            }
                        }
                        painter.drawPath(path);

                    }
                }
            }
        }

        for (const auto& refPoint : refPoints) {
            if (refPoint.isVisible()) {
                QPen pen = refPoint.pen;
                pen.setWidth(pen.width() * mThickness);
                painter.setPen(pen);

                type_data xmin = refPoint.Xmin;
                type_data xmax = refPoint.Xmax;
                type_data ymin = refPoint.Ymin;
                type_data ymax = refPoint.Ymax;
                if (xmax >= mCurrentMinX && xmin <= mCurrentMaxX && ymax >= mMinY && ymin <= mMaxY) {
                    const type_data xmoy = (xmax + xmin) / 2.0;
                    const type_data ymoy = (ymax + ymin) / 2.0;

                    QPen refPointsPen = pen;
                    refPointsPen.setColor(refPoint.color);
                    refPointsPen.setBrush(refPoint.color);
                    refPointsPen.setWidthF(pen.widthF());
                    QPainterPath pathPoint;
                    // ----
                    const qreal penWidth = std::max(2.0, pen.widthF());

                    const qreal yPlot = getYForValue(ymoy, true);
                    const qreal xPlot = getXForValue(xmoy);

                    qreal xMinPlot = getXForValue(xmin);
                    qreal xMaxPlot = getXForValue(xmax);
                    if (xMaxPlot - xMinPlot < 2*penWidth) {
                        const qreal xMean = (xMaxPlot+xMinPlot) / 2.0;
                        xMinPlot = xMean - penWidth;
                        xMaxPlot = xMean + penWidth;
                    }

                    const qreal rayPlot = 1.5*penWidth;

                    QColor bg (backgroundColor());
                    bg.setAlpha(100);
                    const QRectF border_h (xMinPlot - 1.0, yPlot - (penWidth + 1.0) / 2.0, xMaxPlot - xMinPlot + 1.0, penWidth + 1.0 );
                    const QRectF border_v (xPlot -(penWidth + 1.0) / 2.0, getYForValue(ymin, true) - (1+penWidth)/2., penWidth + 1.0, (getYForValue(ymax, true) - getYForValue(ymin, true)) + penWidth + 2.0 );

                    switch (refPoint.type) {
                    case CurveRefPts::eCross:
                        refPointsPen.setWidthF(penWidth);

                        painter.setBrush(refPointsPen.brush());
                        painter.setPen(QPen(bg, 1));
                        painter.drawRect(border_v);
                        painter.drawRect(border_h);

                        painter.setPen(refPointsPen);
                        painter.drawEllipse(QRectF( xPlot - penWidth, yPlot - penWidth,  penWidth*2., penWidth*2.));
                        break;

                    case CurveRefPts::eLine:
                        refPointsPen.setWidthF(penWidth);

                        // Draw a line with a border color background
                        painter.setBrush(refPointsPen.brush());

                        painter.setPen(QPen(bg, 1));
                        painter.drawRect(border_h);

                    break;

                    case CurveRefPts::eDotLine:
                        pathPoint.moveTo( xMinPlot, yPlot );
                        pathPoint.lineTo( xMaxPlot, yPlot );

                        refPointsPen.setWidthF(penWidth);
                        refPointsPen.setStyle(Qt::DotLine);
                        painter.setBrush(refPointsPen.brush());
                        painter.setPen(refPointsPen);
                        painter.strokePath(pathPoint, refPointsPen);
                    break;

                    case CurveRefPts::eDotLineCross:
                    {    painter.drawEllipse(QRectF( xPlot - penWidth, yPlot - penWidth,  penWidth*2., penWidth*2.));

                        pathPoint.moveTo( xMinPlot, yPlot);
                        pathPoint.lineTo( xMaxPlot, yPlot);

                        refPointsPen.setWidthF(penWidth - 1.);
                        refPointsPen.setStyle(Qt::CustomDashLine);
                        refPointsPen.setDashPattern(QList<qreal>{1, 5});

                        painter.setBrush(refPointsPen.brush());

                        // Gestion de la couleur grise avec transparence
                        QColor grayColor = Qt::gray;
                        grayColor.setAlphaF(0.5); // 0.5 = 70% d'opacité (3-50% de transparence)
                        refPointsPen.setColor(grayColor);

                        painter.setPen(refPointsPen);
                        painter.strokePath(pathPoint, refPointsPen);

                        refPointsPen.setStyle(Qt::SolidLine);
                        refPointsPen.setWidthF(penWidth);

                        painter.setPen(QPen(bg, 1));
                        painter.drawRect(border_v);
                    }
                    break;

                    case CurveRefPts::eCustomDashLineCross:
                    {    // --- 1. TRACÉ DE LA LIGNE POINTILLÉE GRISE ET TRANSPARENTE ---
                        pathPoint.moveTo(xMinPlot, yPlot);
                        pathPoint.lineTo(xMaxPlot, yPlot);

                        // Sauvegarde de la couleur originale
                        QColor originalColor = refPointsPen.color();

                        // Configuration du style pointillé
                        refPointsPen.setWidthF(pen.widthF());
                        refPointsPen.setStyle(Qt::CustomDashLine);
                        refPointsPen.setDashPattern(QList<qreal>{5, 5});

                        // Gestion de la couleur grise avec transparence
                        QColor grayColor = Qt::gray;
                        grayColor.setAlphaF(0.7); // 0.7 = 70% d'opacité (30% de transparence)
                        refPointsPen.setColor(grayColor);

                        painter.setBrush(refPointsPen.brush());
                        painter.setPen(refPointsPen);
                        painter.strokePath(pathPoint, refPointsPen);

                        // --- 2. TRACÉ DE LA CROIX (COULEUR ORIGINALE ET OPAQUE) ---
                        pathPoint.clear();

                        // Rétablissement du style plein et de la couleur d'origine
                        refPointsPen.setStyle(Qt::SolidLine);
                        refPointsPen.setColor(originalColor);

                        painter.setPen(refPointsPen);

                        pathPoint.moveTo(xPlot, getYForValue(ymin, true));
                        pathPoint.lineTo(xPlot, getYForValue(ymax, true));
                        painter.strokePath(pathPoint, refPointsPen);
                    }
                    break;


                    case CurveRefPts::eRoundLine:
                    {    pathPoint.addEllipse(xPlot - rayPlot, yPlot - rayPlot, rayPlot*2., rayPlot*2.);

                        pathPoint.moveTo( xPlot, getYForValue(ymin, true));
                        pathPoint.lineTo( xPlot, getYForValue(ymax, true));

                        refPointsPen.setWidthF( pen.widthF()); // not penWidth
                        refPointsPen.setStyle(Qt::SolidLine);

                        painter.setBrush(Qt::NoBrush);
                        painter.setPen(refPointsPen);
                        painter.strokePath(pathPoint, refPointsPen);
                    }
                    break;

                    case CurveRefPts::ePoint:
                        pathPoint.addEllipse(xPlot - rayPlot, yPlot - rayPlot, rayPlot*2., rayPlot*2.);

                        refPointsPen.setWidthF( penWidth);
                        refPointsPen.setStyle(Qt::SolidLine);
                        painter.setBrush(refPointsPen.brush());
                        painter.setPen(refPointsPen);
                        painter.strokePath(pathPoint, refPointsPen);
                    break;

                    default:
                        break;
                    }

                }

            }

        }
    }
    painter.restore();

}


// Lissage simple : moyenne glissante sur une fenêtre de taille 2 * demi
std::vector<double> smooth_histogram(const std::vector<double>& histo, int demi)
{
    std::vector<double> smooth(histo.size(), 0.0);
    for (size_t i = 0; i < histo.size(); ++i) {
        double sum = 0.0;
        int count = 0;
        for (int j = -demi; j <= demi; ++j) {
            int idx = static_cast<int>(i) + j;
            if (idx >= 0 && idx < static_cast<int>(histo.size())) {
                sum += histo[idx];
                count++;
            }
        }
        smooth[i] = sum / count;
    }
    return smooth;
}


// Fonction pour assombrir/éclaircir une couleur
QColor adjustColor(const QColor& base, double factor) {
    factor = std::clamp(factor, 0.0, 1.0);

    const int r = std::clamp(293.92 * pow(factor,3) - 476.17 * pow(factor,2) + 318.25 * factor + base.red(), 0.0, 247.0);
    const int g = std::clamp(-83.48 * pow(factor,3) + 138.88 * pow(factor,2) + 14.75 * factor + base.green(), 0.0, 247.0);
    const int b = std::clamp(-1.26 * pow(factor,3) + 3.31 * pow(factor,2) - 11.22 * factor + base.blue(), 0.0, 247.0);
    return QColor(r, g, b);

}

// Génère 5 ColorStops depuis une couleur principale
std::vector<ColorStop> generateStopsFromBaseColor(const QColor& baseColor) {
    return {
        {0.0, adjustColor(baseColor, 0.2)},  // très foncé
        {0.25, adjustColor(baseColor, 0.45)},// foncé
        {0.5, baseColor},                    // normal
        {0.75, adjustColor(baseColor, 0.75)},// clair
        {1.0, QColor(247, 247, 247)}         // presque blanc
    };
}

// Génère une fonction de couleur à partir de la couleur de base
std::function<QColor(double)> makeColorGradientFromBase(const QColor& baseColor, int resolution = 256) {
    auto stops = generateStopsFromBaseColor(baseColor);
    std::vector<QColor> cache(resolution);

    for (int i = 0; i < resolution; ++i) {
        double t = static_cast<double>(i) / (resolution - 1);
        for (size_t j = 0; j < stops.size() - 1; ++j) {
            if (t <= stops[j + 1].pos) {
                double ratio = (t - stops[j].pos) / (stops[j + 1].pos - stops[j].pos);
                const QColor& c1 = stops[j].color;
                const QColor& c2 = stops[j + 1].color;

                /* int r = static_cast<int>(c1.red()   * (1 - ratio) + c2.red()   * ratio);
                int g = static_cast<int>(c1.green() * (1 - ratio) + c2.green() * ratio);
                int b = static_cast<int>(c1.blue()  * (1 - ratio) + c2.blue()  * ratio);
                */

                int r = static_cast<int>(c1.red()   * (1 - ratio) + c2.red()   * ratio);
                int g = static_cast<int>(c1.green() * (1 - ratio) + c2.green() * ratio);
                int b = static_cast<int>(c1.blue()  * (1 - ratio) + c2.blue()  * ratio);


                cache[i] = QColor(r, g, b);
                break;
            }
        }
    }

    return [cache, resolution](double normVal) -> QColor {
        normVal = std::clamp(normVal, 0.0, 1.0);
        int index = static_cast<int>(normVal * (resolution - 1));
        return cache[index];
    };
}

void GraphView::drawMap(const GraphCurve& curve, QPainter& painter)
{
    const CurveMap& map = curve.mMap;
    const int numCols = static_cast<int>(map.column());
    const int numRows = static_cast<int>(map.row());
    if (numCols < 2 || numRows < 2)
        return;

    const double minX = map.minX(), maxX = map.maxX();
    const double minY = map.minY(), maxY = map.maxY();
    const double minVal = map.minValue(), maxVal = map.maxValue();
    const double span = maxVal - minVal;
    if (span <= 0.)
        return;

    // 1. Table de couleurs (256 entrées) calculée une seule fois
    constexpr int LUT = 256;
    std::array<QRgb, LUT> lut;
    for (int k = 0; k < LUT; ++k) {
        const QColor col = ColorStops::getColorFromStops(double(k) / (LUT - 1), curve.mColorStops);
        lut[k] = qPremultiply(col.rgba());
    }

    // 2. Remplissage direct des pixels : 1 pixel = 1 cellule
    QImage img(numCols, numRows, QImage::Format_ARGB32_Premultiplied);
    for (int r = 0; r < numRows; ++r) {
        // la ligne 0 de l'image est en haut ; la ligne 0 de la map est minY (en bas)
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(numRows - 1 - r));
        for (int c = 0; c < numCols; ++c) {
            const double val = map(c, r);
            if (val > minVal) {
                const int k = std::clamp(int((val - minVal) / span * (LUT - 1) + 0.5), 0, LUT - 1);
                line[c] = lut[k];
            } else {
                line[c] = 0; // transparent
            }
        }
    }

    // 3. Rectangle cible : chaque pixel est centré sur son nœud
    const double halfX = 0.5 * (maxX - minX) / (numCols - 1);
    const double halfY = 0.5 * (maxY - minY) / (numRows - 1);

    const QRectF target(QPointF(getXForValue(minX - halfX, false), getYForValue(maxY + halfY, false)),
                        QPointF(getXForValue(maxX + halfX, false), getYForValue(minY - halfY, false)));

    painter.save();
    painter.setClipRect(QRectF(mMarginLeft, mMarginTop, mGraphWidth, mGraphHeight));
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false); // cellules nettes, comme avant
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.drawImage(target, img);
    painter.restore();
}
QPainterPath GraphView::makeSubShape(const QMap<double, double>& mapInf, const QMap<double, double>& mapSup) const
{
    QPainterPath fillPath;
    if (mapInf.isEmpty() && mapSup.isEmpty())
        return fillPath;

    // First curve is the inferior Curve
    QPainterPath pathInf = makePath(mapInf, false);

    // Second curve
    QPainterPath pathSup = makePath(mapSup, false);

    if (pathInf.elementCount() == 0 && pathSup.elementCount() == 0)
        return fillPath;

    QPointF pFirstInf, pFirstSup;
    if (pathInf.elementCount() > 0)
        pFirstInf = QPointF(pathInf.elementAt(0).x, pathInf.elementAt(0).y);

    if (pathSup.elementCount() > 0)
        pFirstSup = QPointF(pathSup.elementAt(0).x, pathSup.elementAt(0).y);


    // traitement des coins
    if (pathInf.elementCount() > 0 && pathSup.elementCount() > 0 ) {

        fillPath.addPath(pathInf);

        // connexion pathInf + pathSup

        if ( pathInf.currentPosition().y() != pathSup.currentPosition().y()) {
            if (pathInf.currentPosition().x() > pathSup.currentPosition().x()) { // triangle Top Right
                fillPath.lineTo(pathInf.currentPosition().x(), pathSup.currentPosition().y() );
                fillPath.lineTo(pathSup.currentPosition());

            } else  if (pathInf.currentPosition().x() < pathSup.currentPosition().x()) {  // triangle Bottom Right
                fillPath.lineTo(pathSup.currentPosition().x(), pathInf.currentPosition().y() );
                fillPath.lineTo(pathSup.currentPosition());

            } else { //pathInf.currentPosition().x() == pathSup.currentPosition().x() // line Rigth
                fillPath.lineTo(pathSup.currentPosition());
            }

        } else {//pathInf.currentPosition().y() == pathSup.currentPosition().y() // line Top or Bottom, Rigth
            fillPath.lineTo(pathSup.currentPosition());
        }

        fillPath.addPath(pathSup.toReversed());
        //fillPath.connectPath(pathSup.toReversed());

        // **********  close the path
        if ( pFirstInf.y() != pFirstSup.y()) {
            if (pFirstInf.x() < pFirstSup.x()) { // triangle Top Left
                fillPath.lineTo(pFirstInf.x(), pFirstSup.y() );
                fillPath.lineTo(pFirstInf);

            } else if (pFirstInf.x() > pFirstSup.x()) {  // triangle Bottom Left
                fillPath.lineTo(pFirstSup.x(), pFirstInf.y() );
                fillPath.lineTo(pFirstInf);

            } else { //pFirstInf.x() == pFirstSup.x() // line Left
                fillPath.lineTo(pFirstInf);
            }
        } else {//pFirstInf.y() == pFirstSup.y() // line Top or Bottom
            fillPath.lineTo(pFirstInf);
        }

    } else if (pathInf.elementCount() == 0) { // triangle on Bottom

        fillPath.addPath(pathSup);
        if (pFirstSup.y() < fillPath.currentPosition().y()) { //triangle Bottom left
            fillPath.lineTo( fillPath.currentPosition().x(), getYForValue(mMinY, true) );
            fillPath.lineTo( pFirstSup.x(), getYForValue(mMinY, true) );
            fillPath.lineTo(pFirstSup); // close triangle

        } else {//triangle Bottom Rigth
            fillPath.lineTo( fillPath.currentPosition().x(), getYForValue(mMinY, true) );
            fillPath.lineTo( pFirstSup.x(), getYForValue(mMinY, true) );

            fillPath.lineTo(pFirstSup); // close triangle
        }

    } else { // pathSecond.elementCount() == 0 // triangle Top
        fillPath.addPath(pathInf);
        if (pFirstInf.y() > fillPath.currentPosition().y()) { //triangle Top Left
            fillPath.lineTo( fillPath.currentPosition().x(), getYForValue(mMaxY, true) );
            fillPath.lineTo( pFirstInf.x(), getYForValue(mMaxY, true) );
            fillPath.lineTo(pFirstInf); // close triangle

        } else {//triangle Top Rigth
            fillPath.lineTo( fillPath.currentPosition().x(), getYForValue(mMaxX, true) );
            fillPath.lineTo( pFirstInf.x(), getYForValue(mMaxY, true) );
            fillPath.lineTo(pFirstInf); // close triangle
        }

    }
    return fillPath;

}

void GraphView::drawShape(const GraphCurve& curve, QPainter& painter)
{
    if (curve.mShape.first.isEmpty() && curve.mShape.second.isEmpty())
            return;

    painter.setPen(curve.mPen);

    auto brush = curve.mBrush;
    if (mCanControlOpacity) {
        QColor c = brush.color();
        c.setAlpha(curve.mBrush.color().alpha()*mOpacity / 100);
        brush.setColor(c);
    }
    // Creation des subPath, on continue tant qu'une des courbes et dans la fenêtre
    auto iFirst = curve.mShape.first.begin(); // courbe Inf
    auto iSecond = curve.mShape.second.begin();

    QMap<double, double> mapInf;
    QMap<double, double> mapSup;

    int nbPoints = 0;
    while (iFirst != curve.mShape.first.end()) {

        bool addPoint = false;

        if ( iFirst != curve.mShape.first.end() && mCurrentMinX <= iFirst.key() && iFirst.key() <= mCurrentMaxX) { // contrainte sur X
            if (iSecond.value() >= mMinY && iFirst.value()<= mMaxY) {

                if (nbPoints == 0 && iFirst != curve.mShape.first.begin()) {
                    mapInf[std::prev(iFirst).key()] =  std::prev(iFirst).value() ;
                    mapSup[std::prev(iSecond).key()] = std::prev(iSecond).value() ;
                }
                mapInf[iFirst.key()] = std::max(mMinY, iFirst.value()) ;
                mapSup[iSecond.key()] = std::min(mMaxY, iSecond.value() );
                addPoint = true;
            }


        }

        nbPoints += addPoint ? 1 : 0 ;


        const bool newShape = not (addPoint) && (nbPoints > 1) ;

        if (newShape) {
            if ( iFirst != curve.mShape.first.end()) {
                 mapInf[iFirst.key()] =  iFirst.value() ;
                 mapSup[iSecond.key()] = iSecond.value() ;
            }

            // tracer des subPath
           const auto subPath = makeSubShape(mapInf, mapSup);
           painter.fillPath(subPath, brush);
           nbPoints = 0;
           mapInf.clear();
           mapSup.clear();

        }
        ++iFirst;
        ++iSecond;
    }

    if (iFirst == curve.mShape.first.end() && (nbPoints > 1)) {
        // tracer des subPath restant
       const auto subPath = makeSubShape(mapInf, mapSup);
       painter.fillPath(subPath, brush);
       //nbPoints = 0;
       mapInf.clear();
       mapSup.clear();

    }


    // First curve is the inferior Curve
    QPainterPath pathInfBorder = makePath(curve.mShape.first, false);

    // Second curve
    QPainterPath pathSupBorder = makePath(curve.mShape.second, false);

    // Border
    painter.setPen(curve.mPen);
    QPen pen = curve.mPen;
    pen.setWidth(pen.width() * mThickness);

    painter.strokePath(pathInfBorder, pen);
    painter.strokePath(pathSupBorder, pen);

}

void GraphView::drawDensity(const GraphCurve& curve, QPainter& painter)
{
    // Down sample curve for better performances
    if (curve.mData.isEmpty())
        return;
    QMap<type_data, type_data> subData = curve.mData;
    subData = getMapDataInRange(subData, mCurrentMinX, mCurrentMaxX);

    if (subData.isEmpty())
        return;

    QMap<type_data, type_data> lightMap;

    if (subData.size() > mGraphWidth) { //always used in the items thumbnails
        int valuesPerPixel = (int)subData.size() /int(mGraphWidth);
        if (valuesPerPixel == 0)
            valuesPerPixel = 1;
        QMap<type_data, type_data>::const_iterator iter = subData.cbegin();
        int index (0);

        while (iter != subData.cend()) {
            if ((index % valuesPerPixel) == 0)
                lightMap[iter.key()] = iter.value();
            ++index;
            ++iter;
        }
    } else
        lightMap = subData;

    // Draw

    QMapIterator<type_data, type_data> iter(lightMap);
    iter.toFront();
    if (!iter.hasNext())
        return;

    iter.next();
    type_data valueX = iter.key();
    type_data valueY = iter.value();
    type_data last_valueY = 0.;
    type_data last_x = getXForValue(valueX);
    type_data last_y = getYForValue(0.0, true) ;

    QPainterPath path;
    path.moveTo(last_x, last_y);
    // Detect square signal front-end without null value at the begin of the QMap
    // e.g calibration of Unif-typo-ref
    if (iter.hasNext()) {
        if (valueY == (iter.peekNext()).value()) {
            path.moveTo( getXForValue(valueX), getYForValue(0.0, true) );
            path.lineTo( getXForValue(valueX), getYForValue(valueY, true) );

        }
    }

    iter.toFront();
    bool isFirst = true;

    if (curve.mBrush != Qt::NoBrush) {
        isFirst = false;
        last_x = getXForValue(valueX, true);
        last_y = getYForValue(0.0, false);
    }
    while (iter.hasNext()) {
        iter.next();
        valueX = iter.key();
        valueY = iter.value();


        qreal x = getXForValue(valueX, true);
        qreal y = getYForValue(valueY, true);

        if (isFirst) {
            if (curve.mIsRectFromZero) {
                path.moveTo(x, getYForValue(0.0, true));
                path.lineTo(x, y);
            } else
                path.moveTo(x, y);

            isFirst=false;

        } else {

            if (curve.isHisto()) {
                // histo bars must be centered around x value :
                const qreal dx2 = (x - last_x) / 2.0;
                path.lineTo(x - dx2, last_y);
                path.lineTo(x - dx2, y);

            } else if (curve.mIsRectFromZero) {

                if (last_valueY != 0 && valueY != 0) {

                    path.lineTo(x, y);

                } else if (last_valueY == 0 && valueY != 0) {
                    // Draw a front end of a square signal some 0 at the begin and at the end

                    // e.i plot the HPD surface

                    path.lineTo(x, last_y);
                    path.lineTo(x, y);

                } else if (last_valueY != 0 && valueY == 0) {
                    // Draw a back end of a square signal some 0 at the begin and at the end
                    // e.i plot the HPD surface
                    path.lineTo(last_x, last_y);
                    path.lineTo(last_x, y);
                }

            } else
                path.lineTo(x, y);

        }
        last_x = x;
        last_y = y;

        last_valueY = valueY;

    }


    if (path.elementCount()  == 1) { //there is only one value
        last_x = getXForValue(valueX, true);
        last_y = getYForValue(0.0, false);
        path.lineTo(last_x, last_y);
    }

    // Detect square signal back-end without null value at the end of the QMap
    // e.i calibration of Unif-typo-ref

    if (curve.mIsRectFromZero && lightMap.size() > 1) {
        QMapIterator<type_data, type_data> lastIter(lightMap);
        lastIter.toBack();
        lastIter.previous();
        if ( lastIter.value() == lastIter.peekPrevious().value() ) {
            type_data x = lastIter.key();
            if ( x > mCurrentMinX && x < mCurrentMaxX)
                path.lineTo(getXForValue(x, true), getYForValue(0, true) );
        }
    }


    if (curve.mIsRectFromZero && (curve.mBrush != Qt::NoBrush) ) {
        // Close the path on the left side
        path.lineTo(last_x, getYForValue(0.0, true));
        painter.fillPath(path, painter.brush());
        painter.strokePath(path, curve.mPen);

    } else
        painter.drawPath(path);


}


//Save & Export

/**
 * @brief Export a density with locale setting and separator and specific step
 * @todo Maybe we can use QString QLocale::createSeparatedList(const QStringList & list) const
 */
void GraphView::exportCurrentDensities(const QString& defaultPath, const QLocale& locale, const QString& csvSep, double step) const
{
    if (step <= 0)
        step = 1;

    const QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph data as..."),
                                                    defaultPath, "CSV (*.csv)");

    QFile file (filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << "GraphView::exportCurrentDensityCurves"<<" nbCurve to export" << mCurves.size();

        QList<QStringList> rows;
        QStringList list;

        list << " X Axis";
        type_data xMin = std::numeric_limits<type_data>::max();;
        type_data xMax = std::numeric_limits<type_data>::max();;

        for (const GraphCurve& curve : std::as_const(mCurves)) {
            if (!curve.mData.empty() &&
                curve.isDensityCurve() &&
                curve.mVisible) {

                // 1 -Create the header
                list << curve.mName;
                // 2 - Find x Min and x Max period, on all curve, we suppose Qmap is order
                if ( std::isinf(xMin) ) {// firstCurveVisible) {
                    xMin = curve.mData.firstKey();
                    xMax = curve.mData.lastKey();
                } else {
                    xMin = qMin(xMin, curve.mData.firstKey());
                    xMax = qMax(xMax, curve.mData.lastKey());
                }
            } else if (curve.mData.empty() &&
                curve.isHorizontalSections() &&
                curve.mVisible) { // if it's a bound

                // 1 -Create the header
                list << curve.mName;

                if ( std::isinf(xMin) ) {// firstCurveVisible) {
                    xMin = curve.mSections.at(0).first - step;
                    xMax = curve.mSections.at(0).second +  step;
                } else {
                    xMin = qMin(xMin, curve.mSections.at(0).first - step);
                    xMax = qMax(xMax, curve.mSections.at(0).second +  step);
                }

            } else continue;
        }
        if (std::isinf(xMin) || std::isinf(xMax))
            return;

        rows<<list;
        rows.reserve(ceil( (xMax - xMin + 1)/step) );

        // 3 - Create Row, with each curve
        //  Create data in row
        double x;
        int nbData = (xMax - xMin)/ step;
        for (int i = 0; i <= nbData; ++i) {
            x = static_cast<double>(i)*step + xMin;
            list.clear();

            list << locale.toString(x);
            for (auto& curve : mCurves) {
                if (!curve.mData.empty() &&
                     curve.isDensityCurve() &&
                    curve.mVisible) {

                    const type_data xi = interpolateValueInQMap(x, curve.mData);
                    list<<locale.toString(xi, 'g', 15);

                } else if (curve.mData.empty() &&
                           curve.isHorizontalSections() &&
                           curve.mVisible) { // if it's a bound

                    if (x <= curve.mSections.at(0).first && curve.mSections.at(0).second < x + step ) {
                        list<<locale.toString(1., 'g', 15);

                    } else if (curve.mSections.at(0).first <= x  && x <= curve.mSections.at(0).second ) {

                        const type_data xi = 1. / (curve.mSections.at(0).second - curve.mSections.at(0).first);
                        list<<locale.toString(xi, 'g', 15);

                    } else {
                        list<<locale.toString(0., 'g', 15);
                    }


                } else continue;

            }
            rows<<list;
        }

        // 4 - Save Qlist
        QTextStream output(&file);
        const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
        const QString projectName = tr("Project filename : %1").arg(MainWindow::getInstance()->getNameProject());

        output << "# "+ version + "\r";
        output << "# "+ projectName + "\r";
        output << "# "+ DateUtils::getAppSettingsFormatStr() + "\r";

        for (auto& row : rows) {
            output << row.join(csvSep);
            output << "\r";
        }
        file.close();
    }

}


void GraphView::exportCurrentVectorCurves(const QString& defaultPath, const QLocale& locale, const QString& csvSep, bool writeInRows, int offset) const
{
    Q_UNUSED(writeInRows);
    qDebug()<<"GraphView::exportCurrentVectorCurves";
    QString filter = "CSV (*.csv)";
    QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph data as..."),
                                                    defaultPath,
                                                    filter);
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        bool abscissesWritten = false;
        QList<QStringList> rows;

        rows.append(QStringList("# X Axis"));
        QMap<type_data, QList<type_data> > rowsData;

        qsizetype rowsCount = rows.count();
        QStringList emptyColumn;

        qDebug() << "[GraphView::exportCurrentVectorCurves] " << " nbCurve to export" << mCurves.size();

        for (auto&& c : mCurves ) {
            if ( !c.mVisible || c.mDataVector.empty() )
                continue;

            const std::vector<type_data>& data = c.mDataVector;
            // the new DataVector is longer than the last, we need to expand the size of rows
            if ((qsizetype)data.size() > rowsCount-2) {
                abscissesWritten = false;
                rowsCount = rows.count();
            }

            if (!abscissesWritten) {
                for (size_t i = offset+rowsCount; i < data.size() + 1; ++i) {
                    // we add 1 to the line number, because the index of vector start to 0-> false now 0 is the init
                    rows.append(QStringList(locale.toString(i - rowsCount + 1)) + emptyColumn);

                }
                abscissesWritten = true;

            }
            //prepare adding row
            emptyColumn << "";

            if (abscissesWritten) {
                    rows[0] << c.mName;
                for (int i = offset; i<(int)data.size(); ++i)
                        rows[i-offset+1] << locale.toString(data[i],'g', 15);

            }

        }

        QMapIterator<type_data, QList<type_data> > iter2(rowsData);
        while(iter2.hasNext()) {
            iter2.next();
            QStringList list;

            list << locale.toString(iter2.key(),'g', 15);

            for (int i=0; i<iter2.value().size(); ++i)
                list << locale.toString(iter2.value().at(i),'g', 15);

            rows.append(list);
        }

        QTextStream output(&file);
        for (const auto& r : std::as_const(rows)) {
            output << r.join(csvSep);
            output << "\n";
        }
        file.close();
    }
}

void GraphView::exportCurrentCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, double step, QString graph_title) const
{
    if (graph_title != "")
        graph_title += "_";

    // Remplace les espaces par un souligné
    const std::string str_title = std::regex_replace(graph_title.toStdString(), std::regex(" "), "_");

    const QString fiName = MainWindow::getInstance()->getNameProject();
    const QString defaultFilename = defaultPath + "/"+ fiName.mid(0, fiName.size()-4) + "_" + QString::fromStdString(str_title) + "graph_data.csv";

    const QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph data as..."),
                                                    defaultFilename, "CSV (*.csv)");

    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug()<<"[GraphView::exportCurrentCurve] Save graph data as..." << filename;

        QList<QStringList> rows;
        QStringList list;
        // 1 -Create the header
      /*  list << "X Axis";
        list << "G";
        list << "G sup 95";
        list << "G inf 95"; */
        double xMin = std::numeric_limits<double>::max();
        double xMax = - std::numeric_limits<double>::max();

        for (const GraphCurve& curve : std::as_const(mCurves)) {
            if ((curve.isDensityCurve()||curve.isFunction()) && curve.mVisible) {
                // 2 - Find x Min and x Max period, on all curve, we suppose Qmap is order
                if ( std::isinf(xMin) ) {// firstCurveVisible) {
                    xMin = curve.mData.firstKey();
                    xMax = curve.mData.lastKey();

                } else {
                    xMin = qMin(xMin, curve.mData.firstKey());
                    xMax = qMax(xMax, curve.mData.lastKey());
                }

            } else if (curve.isShape() && curve.mVisible) {

                if ( std::isinf(xMin) ) {// firstCurveVisible) {
                    xMin = curve.mShape.first.firstKey();
                    xMax = curve.mShape.first.lastKey();

                } else {
                    xMin = qMin(xMin, curve.mShape.first.firstKey());
                    xMax = qMax(xMax, curve.mShape.first.lastKey());
                }
            }
            else continue;
        }
        if (std::isinf(xMin) || std::isinf(xMax))
            return;

        int nbData = 0 ;
        if (step <= 0.) {
            for (const GraphCurve& c : std::as_const(mCurves))
                if (c.mVisible)
                    nbData = std::max(nbData, (int) c.mData.count());

            step = ceil(xMax - xMin)/ (nbData - 1);

        } else
            nbData = floor((xMax - xMin)/ step);

        rows.reserve(nbData + 1); // the header

        // 1 -Create the header
        list << "X Axis";
        for (auto& c : mCurves) {
            if (c.mVisible) {
                if (c.isDensityCurve())
                    list << c.mName;

                else if (c.isFunction())
                    list << c.mName; // for example G

                else if (c.isShape()) {
                    list << c.mName + " Inf";
                    list << c.mName + " Sup"; // for example env G
                }
            }
        }
        rows<<list;

        // 3 - Create Row, with each curve
        //  Create data in row
        double x;

        for (int i = 0; i < nbData; ++i) {
            x = static_cast<double>(i)*step + xMin;
            list.clear();

            list << locale.toString(x);

            for (auto& c : mCurves) {
                if (c.mVisible) {
                    if (c.isDensityCurve()) {
                        if (c.mData.firstKey() <= x && x <= c.mData.lastKey())
                            list<<locale.toString(interpolateValueInQMap(x, c.mData), 'g', 15); // for example G
                        else
                            list<< "0";

                    } else if (c.isFunction()) {
                        if (c.mData.firstKey() <= x && x <= c.mData.lastKey())
                            list<<locale.toString(interpolateValueInQMap(x, c.mData), 'g', 15); // for example G
                        else
                            list<< "NaN";

                    } else if (c.isShape()) {
                        if (c.mShape.first.firstKey()<= x && x<= c.mShape.first.lastKey()) {
                            list<<locale.toString(interpolateValueInQMap(x, c.mShape.first), 'g', 15);
                            list<<locale.toString(interpolateValueInQMap(x, c.mShape.second), 'g', 15); // for example env G
                        } else {
                            list<< "NaN";
                            list<< "NaN";
                        }
                    }
                }
            }

            rows<<list;
        }

        // 4 - Save Qlist
        QTextStream output(&file);
        const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
        const QString projectName = tr("Project filename : %1").arg(MainWindow::getInstance()->getNameProject());

        output << "# "+ version + "\r";
        output << "# "+ projectName + "\r";
        output << "# "+ DateUtils::getAppSettingsFormatStr() + "\r";

        for (auto& row : rows) {
            output << row.join(csvSep);
            output << "\r";
        }
        file.close();
    }

}

/**
 * @brief GraphView::exportReferenceCurves
 * @param defaultPath
 * @param locale
 * @param csvSep
 * @param step
 */
void GraphView::exportReferenceCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, double step, QString filename, const double threshold, bool isHPD) const
{
    if (step <= 0)
        step = 1;

    if (filename == "") {
        const QString fiName = MainWindow::getInstance()->getNameProject();

        const QString defaultFilename = defaultPath + "/"+ fiName.mid(0, fiName.size()-4) + (isHPD ? "_HPD_ref.csv":"_Gauss_ref.csv");
        qDebug() << "GraphView::exportReferenceCurves : " << defaultFilename;
        filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                tr("Save Reference Curve as..."),
                                                defaultFilename, "CSV (*.csv)");
    }
    if (filename.isEmpty())
        return;

    QFile file(filename);

    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << "[GraphView::exportReferenceCurves] Save Reference Curve as..." << filename;

        QList<QStringList> rows;
        QStringList list;
        // 1 -Create the header
        list << "X Axis";
        if (isHPD) {
            list << "HPD Mid";
            list << "HPD err";

        } else {
            list << "G";
            list << "err G";
        }

        type_data xMin = std::numeric_limits<type_data>::max();
        type_data xMax = std::numeric_limits<type_data>::max();

        QMap <type_data, type_data> G, G_Sup;
        if (isHPD) {
            for (const GraphCurve& curve : std::as_const(mCurves)) {
                if (curve.mName == "G HPD Mid") {
                    G = curve.mData;
                    xMin = G.firstKey();
                    xMax = G.lastKey();

                } else if (curve.mName == "G HPD Env") {
                    G_Sup = curve.mShape.second;
                }

            }
        } else {
            for (const GraphCurve& curve : std::as_const(mCurves)) {
                if (curve.mName == "G Mean") {
                    G = curve.mData;
                    xMin = G.firstKey();
                    xMax = G.lastKey();

                } else if (curve.mName == "G Gauss Env") {
                    G_Sup = curve.mShape.second;
                }
            }
        }

        if (std::isinf(xMin) || std::isinf(xMax))
            return;

        rows << list;
        rows.reserve(ceil( (xMax - xMin)/step) );

        // 3 - Create Row, with each curve
        //  Create data in row

        int nbData = (xMax - xMin)/ step;
        QLocale csvLocal (locale);
        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);
        auto xMax_BCAD = DateUtils::convertFromAppSettingsFormat(xMax);
        auto xMin_BCAD = DateUtils::convertFromAppSettingsFormat(xMin);

        // il faut retrouver l'erreur sigma sur la courbe à partir de l'enveloppe et du seuil courrant
        // https://en.wikipedia.org/wiki/Standard_score
        // Quantile normal pour 1 - alpha/2

        const double z_score = zCritical(threshold); // Pour 95% z=1.96
        // Déduction de sigma à partir de la largeur de l'intervalle
        //return std::abs((b - a) / (2.0 * z_score));
        if (xMin_BCAD < xMax_BCAD) {
            for (int i = 0; i <= nbData; ++i) {
                const auto x = static_cast<type_data>(i)*step + xMin;
                const auto t = DateUtils::convertFromAppSettingsFormat(x);
                list.clear();

                list << csvLocal.toString(t);
                // Il doit y avoir au moins trois courbes G, GSup, Ginf et nous exportons G et ErrG
                const type_data xi = interpolateValueInQMap(x, G); // G
                const type_data err_xi = interpolateValueInQMap(x, G_Sup); // GSup
                list << csvLocal.toString(xi, 'g', 15);

                // il faut retrouver l'erreur sigma sur la courbe à partir de l'enveloppe et du seuil courrant
                //auto sigma = (err_xi - xi) / 1.96; // dans le cas d'un seuil à 95%
                auto sigma = (err_xi - xi) / z_score;
                list << csvLocal.toString(sigma, 'g', 15);

                //      }
                rows << list;
            }
        } else {
            for (int i = nbData; i >= 0; --i) {
                const auto x = (type_data)(i)*step + xMin;
                const auto t = DateUtils::convertFromAppSettingsFormat(x);
                list.clear();

                list << csvLocal.toString(t);
                // Il doit y avoir au moins trois courbes G, GSup, Ginf et nous exportons G et ErrG
                const type_data xi = interpolateValueInQMap(x, G); // G
                const type_data err_xi = interpolateValueInQMap(x, G_Sup); // GSup
                list << csvLocal.toString(xi, 'g', 15);
                list << csvLocal.toString((err_xi - xi) / 1.96, 'g', 15);

                //      }
                rows<<list;
            }
        }
        // 4 - Save Qlist

        QTextStream output(&file);
        const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
        const QString projectName = tr("Project filename : %1").arg(MainWindow::getInstance()->getNameProject());

        output << "# "+ version + "\r";
        output << "# "+ projectName + "\r";
        if (isHPD) {
          output << "# "+ tr("Curve derived from the %1% HPD").arg(threshold) + " % \r";
        }
        output << "# BC/AD \r";//DateUtils::getAppSettingsFormatStr() + "\r";

        for (auto& row : rows) {
            output << row.join(csvSep);
            output << "\r";
        }
        file.close();
    }

}


bool GraphView::saveAsSVG(const QString& fileName, const QString& graphTitle, const QString& svgDescrition, const bool withVersion, const int versionHeight)
{
    if (fileName.isEmpty()) {
        return false;

    } else {
        //Rendering memoRendering= mRendering;
        QRect rTotal(withVersion ? QRect(0, 0, width(), height()+versionHeight) : QRect(0, 0, width(), height()));

        const int graphRigthShift = 40;

        QRect rGraph(0, 0, width(), height());
        /* We can not have a svg graph in eSD Rendering Mode */
        //  setRendering(eHD);
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
        painter.drawText( graphRigthShift,0, graphRigthShift+fm.horizontalAdvance(graphTitle), versionHeight,
                         Qt::AlignCenter,
                         svgGen.title());

        if (withVersion) {
            painter.setPen(Qt::black);
            painter.drawText(0, rGraph.y()+rGraph.height(), rGraph.width(),versionHeight,
                             Qt::AlignCenter,
                             qApp->applicationName() + " " + qApp->applicationVersion());
        }
        painter.end();

        //setRendering(memoRendering);

        return true;

    }

}


#pragma mark GraphTitle
/**
 * @brief GraphTitle::GraphTitle
 * @param parent
 * Le subTitle is in italic
 */
GraphTitle::GraphTitle(QWidget *parent):
    mTitle("myTitle"),
    mCommentTitle(""),
    mSubTitle(""),
    mBackgroundColor(Qt::white),
    mTitleBarColor(Qt::white),
    mAutoAdjustTitleHeight(true),
    mAutoAdjustSubTitleHeight(true)
{
    GraphViewAbstract::setParent(parent);
    mMarginTop = 0.0;
    mMarginBottom = 0.0;
    mMarginLeft = 0.0;

    QFont boldFont (font());
    boldFont.setBold(true);
    mTitleHeight = QFontMetrics(boldFont).height();

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();

}

/**
 * @brief GraphTitle::GraphTitle just a title bar, with margin on top and bottom
 * @param title
 * @param parent
 */
GraphTitle::GraphTitle(const QString& title, QWidget* parent):
    mTitle(title),
    mCommentTitle(""),
    mSubTitle(""),
    mBackgroundColor(Qt::white),
    mTitleBarColor(Qt::white),
    mAutoAdjustTitleHeight(true),
    mAutoAdjustSubTitleHeight(true)
{
    GraphViewAbstract::setParent(parent);

    QFont boldFont (font());
    boldFont.setBold(true);
    mTitleHeight = QFontMetrics(boldFont).height();

    mMarginTop = mTitleHeight / 2.0;
    mMarginBottom = mTitleHeight / 2.0;
    mMarginLeft = mTitleHeight / 2.0;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}

GraphTitle::GraphTitle(const QString& title, const QColor& titleBarColor, QWidget* parent):
    mTitle(title),
    mCommentTitle(""),
    mSubTitle(""),
    mBackgroundColor(Qt::white),
    mTitleBarColor(titleBarColor),
    mAutoAdjustTitleHeight(true),
    mAutoAdjustSubTitleHeight(true)
{
    GraphViewAbstract::setParent(parent);

    QFont boldFont (font());
    boldFont.setBold(true);
    mTitleHeight = QFontMetrics(boldFont).height();

    mMarginTop = mTitleHeight / 2.0;
    mMarginBottom = 0.0;
    mMarginLeft = 20.0;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}

GraphTitle::GraphTitle(const QString& title, const QString& subTitle, QWidget* parent):
    mTitle(title),
    mCommentTitle(""),
    mSubTitle(subTitle),
    mBackgroundColor(Qt::white),
    mTitleBarColor(Qt::white),
    mAutoAdjustTitleHeight(true),
    mAutoAdjustSubTitleHeight(true)
{
    GraphViewAbstract::setParent(parent);

    QFont boldFont (font());
    boldFont.setBold(true);
    mTitleHeight = QFontMetrics(boldFont).height();

    mMarginTop =  mTitleHeight / 2.0;
    mMarginBottom = 0.0;
    mMarginLeft = 20.0;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();

}

GraphTitle::GraphTitle(const QString& title, const QString& subTitle, const QColor& backGround, QWidget* parent):
    mTitle(title),
    mSubTitle(subTitle),
    mBackgroundColor(backGround),
    mTitleBarColor(backGround)
{
    GraphViewAbstract::setParent(parent);

    QFont boldFont (font());
    boldFont.setBold(true);
    mTitleHeight = QFontMetrics(boldFont).height();

    mMarginTop =  mTitleHeight / 2.0 ;
    mMarginBottom = 0.0;
    mMarginLeft = 20.0;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}

GraphTitle::GraphTitle(const QString& title, const QString& commentTitle, const QString& subTitle, QWidget* parent):
    mTitle(title),
    mCommentTitle(commentTitle),
    mSubTitle(subTitle),
    mBackgroundColor(Qt::white),
    mTitleBarColor(Qt::white)
{
    GraphViewAbstract::setParent(parent);

    QFont boldFont (font());
    boldFont.setBold(true);
    mTitleHeight = QFontMetrics(boldFont).height();

    mMarginTop =  mTitleHeight / 2.0 ;
    mMarginBottom = 0.0;
    mMarginLeft = 20.0;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}


qreal GraphTitle::height() const
{
    if (!mTitle.text().isEmpty()) {
        if (!mSubTitle.text().isEmpty())
            return mTitleHeight + mSubTitleHeight + mMarginTop + mMarginBottom;
        else
            return mTitleHeight + mMarginTop + mMarginBottom;

    } else if (!mSubTitle.text().isEmpty()) {
        return mSubTitleHeight + mMarginTop + mMarginBottom;

    } else
        return 0.;
}

void GraphTitle::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const qreal w = width();//parentWidget()->width();

    if (!mTitle.text().isEmpty()) {

        if (!mSubTitle.text().isEmpty()) {
            p.fillRect(0, 0, w , height(), mBackgroundColor);

            p.setPen(Qt::black);
            QFont boldFont (font());
            boldFont.setBold(true);

            QFont itaFont (font());
            itaFont.setItalic(true);

            p.setFont(boldFont);
            qreal fontShift = QFontMetrics(boldFont).height() / 2.0;
            p.drawStaticText(mMarginLeft, mTitleHeight / 2.0 - fontShift + mMarginTop, mTitle);
            const int shift = QFontMetrics(boldFont).horizontalAdvance(mTitle.text());

            if (!mCommentTitle.text().isEmpty()) {
                p.setFont(itaFont);
                p.drawStaticText(mMarginLeft + shift, mTitleHeight / 2.0 - fontShift + mMarginTop, mCommentTitle);

            }

            p.setFont(itaFont);
            fontShift = QFontMetrics(itaFont).height() / 2.0;
            p.drawStaticText(mMarginLeft, mTitleHeight + mSubTitleHeight/2. - fontShift + mMarginTop, mSubTitle);

            p.setPen(mTitleBarColor);
            p.drawRect(0, 0, w , height());

        } else {
            p.fillRect(0, 0, w , height(), mBackgroundColor);

            p.setPen(Qt::black);
            QFont boldFont (font());
            boldFont.setBold(true);
            p.setFont(boldFont);
            const qreal fontShift = QFontMetrics(boldFont).height()/2.;
            p.drawStaticText(mMarginLeft, mTitleHeight / 2.0 - fontShift + mMarginTop, mTitle);

            p.setPen(mTitleBarColor);
            p.drawRect(0, 0, w , height());

        }

    } else if (!mSubTitle.text().isEmpty()) {
        p.fillRect(0, 0, w , height(), mBackgroundColor);

        p.setPen(Qt::black);
        QFont itaFont (font());
        itaFont.setItalic(true);
        p.setFont(itaFont);
        const qreal fontShift = QFontMetrics(itaFont).height()/2.;
        p.drawStaticText(mMarginLeft, mSubTitleHeight / 2.0 - fontShift + mMarginTop, mSubTitle);

    }

}

void GraphTitle::repaintGraph()
{
    update();
}


