/* ---------------------------------------------------------------------
Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "GraphView.h"

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include "Painting.h"
#include "MainWindow.h"

#include <QtWidgets>
#include <algorithm>
#include <QtSvg>
#include <QLocale>

class StudyPeriodSettings;

GraphView::GraphView(QWidget *parent):
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
    mXAxisMode(eAllTicks),
    mYAxisMode(eAllTicks),
    mOverflowArrowMode(eNone),
    mAutoAdjustYScale(false),
    mShowInfos(false),
    mBackgroundColor(Qt::white),
    mThickness(1),
    mOpacity(100),
    mCanControlOpacity(false),
    mTipX(0.),
    mTipY(0.),
    mTipComment(""),
    mTipWidth(110.),
    mTipHeight(40.),
    mTipVisible(false),
    mUseTip(true),
    mUnitFunctionX(nullptr),
    mUnitFunctionY(nullptr)
{
    GraphViewAbstract::setParent(parent);
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

    setRangeY(0., 1.);
    resetNothingMessage();

    connect(this, &GraphView::signalCurvesThickness, this, &GraphView::updateCurvesThickness);

}

GraphView::GraphView(const GraphView& graph, QWidget *parent):
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

    connect(this, &GraphView::signalCurvesThickness, this, &GraphView::updateCurvesThickness);

    mCurves = graph.mCurves;
    mZones = graph.mZones;

   //mBufferBack = QPixmap();
}

void GraphView::copyFrom(const GraphView &graph)
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
    mCanControlOpacity =graph.mCanControlOpacity;
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

    //mBufferBack = graph.mBufferBack;
}

GraphView::~GraphView()
{
    mUnitFunctionX = nullptr;
    mUnitFunctionY = nullptr;
    mCurves.clear();
    mZones.clear();
}

void GraphView::adjustYScale()
{
    if (mAutoAdjustYScale) {
        qreal yMax = -HUGE_VAL;
        qreal yMin =  HUGE_VAL;

        for (const auto &curve : mCurves) {
            if (curve.mVisible) {
                if (curve.isVertical()) { // used for the measurement in the calibration process
                    yMin = std::min(yMin, curve.mData.firstKey());
                    yMax = std::max(yMax, curve.mData.lastKey());
                } else if (!curve.isVerticalLine() && !curve.isHorizontalSections()) {

                    if (curve.isVectorData()) {
                        const QList<qreal> &subData = getVectorDataInRange(curve.mDataVector, mCurrentMinX, mCurrentMaxX, qreal (0.), qreal (curve.mDataVector.size()));
                        if (!subData.empty()) {
                            yMin = std::min(yMin, range_min_value(subData));
                            yMax = std::max(yMax, range_max_value(subData));
                        }

                    } else if (curve.isShape()) {
                        const auto &curveInf = curve.mShape.first;
                        const auto &curveSup = curve.mShape.second;
                        const QMap<qreal, qreal> &subDataInf = getMapDataInRange(curveInf, mCurrentMinX, mCurrentMaxX);
                        if (!subDataInf.empty())
                            yMin = std::min(yMin, map_min(subDataInf).value());

                        const QMap<qreal, qreal> &subDataSup = getMapDataInRange(curveSup, mCurrentMinX, mCurrentMaxX);
                        if (!subDataSup.empty())
                            yMax = std::max(yMax, map_max(subDataSup).value());

                    } else if (!curve.mData.empty()) {
                        yMin = std::min(yMin, map_min(curve.mData, mCurrentMinX, mCurrentMaxX).value());
                        yMax = std::max(yMax, map_max(curve.mData, mCurrentMinX, mCurrentMaxX).value());

                    } else if (curve.mMap.data.size() > 0) {// map
                        yMin = std::min(yMin, curve.mMap.rangeY.first);
                        yMax = std::max(yMax, curve.mMap.rangeY.second);

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
            if (mAxisToolY.mMinMaxOnly || !mYAxisTicks) {
                if (yMax == yMin) {
                    if (yMax>0) {
                        yMin = 0.;
                    } else {
                        yMax = 0.;
                    }
                }
                setRangeY(yMin, yMax);

            } else {
                Scale yScale;
                yScale.findOptimal(yMin, yMax, 7);
                setRangeY(yScale.min, yScale.max);
                setYScaleDivision(yScale);
            }
        }
    }
}

void GraphView::zoomX(const type_data min, const type_data max)
{
    if (mCurrentMinX != min || mCurrentMaxX != max || mMinY>=mMaxY || mAutoAdjustYScale) {
        mCurrentMinX = min;
        mCurrentMaxX = max;

        mAxisToolX.updateValues(width(), 10., min, max);
        
        adjustYScale();
        repaintGraph(true);

    }

}

void GraphView::changeXScaleDivision (const Scale &sc)
{
    setXScaleDivision(sc);
    repaintGraph(true);
}

void GraphView::changeXScaleDivision (const double &major, const int & minor)
{
    setXScaleDivision(major, minor);
    repaintGraph(true);
}

/* ------------------------------------------------------
 *  Options
 * ------------------------------------------------------*/

void GraphView::setBackgroundColor(const QColor &color)
{
    mBackgroundColor = color;
}

QColor GraphView::getBackgroundColor() const
{
    return mBackgroundColor;
}

void GraphView::addInfo(const QString& info)
{
    mInfos << info;
}

void GraphView::clearInfos()
{
    mInfos.clear();
 }

void GraphView::showInfos(bool show)
{
    mShowInfos = show;
 }

void GraphView::setNothingMessage(const QString& message)
{
    mNothingMessage = message;
}

void GraphView::resetNothingMessage()
{
    mNothingMessage = tr("Nothing to display");
 }

// Just Setter no action
void GraphView::showXAxisLine(bool show)     { mXAxisLine = show;}
void GraphView::showXAxisArrow(bool show)    { mXAxisArrow = show;}
void GraphView::showXAxisTicks(bool show)    { mXAxisTicks = show;}
void GraphView::showXAxisSubTicks(bool show) { mXAxisSubTicks = show;}
void GraphView::showXAxisValues(bool show)   { mXAxisValues = show;}

void GraphView::showYAxisLine(bool show)     { mYAxisLine = show;}
void GraphView::showYAxisArrow(bool show)    { mYAxisArrow = show;}
void GraphView::showYAxisTicks(bool show)    { mYAxisTicks = show;}
void GraphView::showYAxisSubTicks(bool show) { mYAxisSubTicks = show;}
void GraphView::showYAxisValues(bool show)   { mYAxisValues = show;}


void GraphView::setXAxisMode(AxisMode mode)
{
   mXAxisMode = mode;
        mAxisToolX.mShowText = (mXAxisMode != eHidden);
}

void GraphView::setYAxisMode(AxisMode mode)
{
    if (mYAxisMode != mode) {
        mYAxisMode = mode;

        mAxisToolY.mMinMaxOnly = (mYAxisMode == eMinMax);
        switch (mYAxisMode) {
        case eMinMax:
            showYAxisValues(false);
            showYAxisTicks(false);
            showYAxisSubTicks(false);
            mYAxisArrow = false;
            mAxisToolY.mShowText = true;
            break;
        case eMinMaxHidden:
            showYAxisValues(false);
            showYAxisTicks(false);
            showYAxisSubTicks(false);
            mYAxisArrow = false;
            mAxisToolY.mShowText = false;
            break;
        case eHidden:
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

/**
 * @brief If active is true, the current view automaticaly adjust Y axis on the next paint.
 */
void GraphView::autoAdjustYScale(bool active)
{
    mAutoAdjustYScale = active;
}

void GraphView::setGraphFont(const QFont &font)
{
    setFont(font);
    repaintGraph(true);
}
/**
 * @brief GraphView::setGraphsThickness throw signal to update thickness
 * @param value
 */
void GraphView::setGraphsThickness(int value)
{
    emit signalCurvesThickness(value);
}

/**
 * @brief GraphView::updateCurvesThickness slot which modify thickness
 * @param value
 */
void GraphView::updateCurvesThickness(int value)
{
    mThickness = value;
    repaintGraph(true);
}

void GraphView::setCurvesOpacity(int value)
{
    mOpacity = value;
    repaintGraph(true);
}

void GraphView::setCanControlOpacity(bool can)
{
    mCanControlOpacity = can;
}

void GraphView::setFormatFunctX(DateConversion f)
{
    mUnitFunctionX = f;
}

void GraphView::setFormatFunctY(DateConversion f)
{
    mUnitFunctionY = f;
}

/* ------------------------------------------------------
 Curves & Zones
 ------------------------------------------------------ */

void GraphView::add_curve(const GraphCurve &curve)
{
    mCurves.append(curve);
}

void GraphView::removeCurve(const QString &name)
{
    for (int i=0; i<mCurves.size(); ++i) {
        if (mCurves.at(i).mName == name) {
            mCurves.removeAt(i);
            break;
        }
    }
}

void GraphView::removeAllCurves()
{
    mCurves.clear();
    repaintGraph(false);
}

void GraphView::reserveCurves(const int size)
{
    mCurves.reserve(size);
}

void GraphView::setCurveVisible(const QString &name, const bool visible)
{
    std::ranges::for_each(mCurves, [name, visible](auto& c) {
        if (c.mName == name && c.mVisible != visible) {
            c.mVisible = visible;
        } });

}

GraphCurve* GraphView::getCurve(const QString &name)
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

void GraphView::add_zone(const GraphZone &zone)
{
    mZones.append(zone);
}

void GraphView::remove_all_zones()
{
    mZones.clear();
}


void GraphView::set_points_visible(const QString &name, const bool visible)
{
    for (auto&& ref : refPoints) {
        if (ref.name == name) {
            ref.setVisible(visible);
        }
    }

}

//  Mouse events & Tool Tip
void GraphView::enterEvent(QEnterEvent *e)
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

void GraphView::setTipXLab(const QString &lab)
{
    mTipXLab = lab =="" ? "":  lab + " = ";
}

void GraphView::setTipYLab(const QString &lab)
{
    mTipYLab = lab =="" ? "":  lab + " = ";
}

void GraphView::resizeEvent(QResizeEvent* )
{
    if (width() == 0 || height() == 0)
        return;
/*
    if (!mBufferBack.isNull()) {
        const qreal sx = qreal(rect().width()) / qreal (mBufferBack.rect().width());
        const qreal sy = qreal (rect().height()) / qreal (mBufferBack.rect().height());

        if (sx>1. || sy>1.) {
            mBufferBack = QPixmap(width(), height());
            updateGraphSize(width(), height());
            paintToDevice(&mBufferBack);

        } else {// l'image est trop deformée, c'est moche !!
            mBufferBack = mBufferBack.transformed(QTransform::fromScale(sx, sy), Qt::SmoothTransformation);
        }

    }
    else {
        mBufferBack = QPixmap(width(), height());
        updateGraphSize(width(), height());
        paintToDevice(&mBufferBack);
    }
*/
    repaintGraph(true);
}

void GraphView::updateGraphSize(qreal w, qreal h)
{
#ifdef Q_OS_MAC
     mBottomSpacer = 0.02 * h;
#else
     mBottomSpacer = 0.04 * h;
#endif

    mGraphWidth = std::max(0., w - mMarginLeft - mMarginRight);
    mGraphHeight = std::max(0., h - mMarginTop - mMarginBottom - mBottomSpacer);
    mAxisToolX.updateValues(int (mGraphWidth), int (mStepMinWidth), mCurrentMinX, mCurrentMaxX);
    mAxisToolY.updateValues(int (mGraphHeight), 12, mMinY, mMaxY);
}

void GraphView::repaintGraph(const bool aAlsoPaintBackground)
{
    (void) aAlsoPaintBackground;
  /*  if (aAlsoPaintBackground) {
        mBufferBack = QPixmap();
    } */
    update();
}

void GraphView::paintEvent(QPaintEvent* )
{

    /* ----------------------------------------------------
     *  Nothing to draw !
     * ----------------------------------------------------*/
    if (mCurves.size() == 0 && mZones.size() == 0 && refPoints.size() == 0) {
        QPainter p(this);
        p.setFont(font());
        p.fillRect(0, 0, width(), height(), mBackgroundColor);
        p.setPen(QColor(100, 100, 100));
        p.drawText(0, 0, width(), height(), Qt::AlignCenter, mNothingMessage);

        return;
    }

    /* resize build mBufferBack, so we don't need to
     * rebuid a graph. We need it in the resizeEvent
     * */
  /*  if (!mBufferBack.isNull() && !mTipVisible) {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing);
        p.drawPixmap(mBufferBack.rect(), mBufferBack, rect());
        return;
    }
*/
    updateGraphSize(width(), height());
    if ((mGraphWidth <= 0) || (mGraphHeight <= 0))
        return;

  //  mBufferBack = QPixmap();
    paintToDevice(this);

    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing);
  //  p.drawPixmap(mBufferBack.rect(), mBufferBack, rect());

    /* ----------------------------------------------------
     *  Tool Tip (above all) Draw horizontal and vertical red line
     * ----------------------------------------------------*/
    if (mTipVisible && (!mTipXLab.isEmpty() || !mTipYLab.isEmpty())) {
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
                    p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2), Qt::AlignCenter, mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
                else
                    p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2), Qt::AlignCenter, mTipXLab + stringForLocal(mTipX));
                if (mUnitFunctionY)
                    p.drawText(mTipRect.adjusted(0, mTipRect.height()/2., 0, 0), Qt::AlignCenter, mTipYLab + stringForLocal(mUnitFunctionY(mTipY)));
                else
                    p.drawText(mTipRect.adjusted(0, mTipRect.height()/2., 0, 0), Qt::AlignCenter, mTipYLab + stringForLocal(mTipY));

            } else {
                if (mUnitFunctionX)
                    p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()*2./3.), Qt::AlignCenter, mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
                else
                    p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()*2./3.), Qt::AlignCenter, mTipXLab + stringForLocal(mTipX));

                if (mUnitFunctionY)
                    p.drawText(mTipRect.adjusted(0, mTipRect.height()*1./3., 0, -mTipRect.height()*1./3.), Qt::AlignCenter, mTipYLab + stringForLocal(mUnitFunctionY(mTipY)));
                else
                    p.drawText(mTipRect.adjusted(0, mTipRect.height()*1./3., 0, -mTipRect.height()*1./3.), Qt::AlignCenter, mTipYLab + stringForLocal(mTipY));

                p.drawText(mTipRect.adjusted(0, mTipRect.height()*2./3., 0, 0), Qt::AlignCenter, " ⇨ " + mTipComment);

            }

        } else if (!mTipXLab.isEmpty()) {
            if (mTipComment.isEmpty()) {
                if (mUnitFunctionX)
                    p.drawText(mTipRect, Qt::AlignCenter, mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
                else
                    p.drawText(mTipRect, Qt::AlignCenter, mTipXLab + stringForLocal(mTipX));

            } else {
                if (mUnitFunctionX)
                    p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2), Qt::AlignCenter, mTipXLab + stringForLocal(mUnitFunctionX(mTipX)));
                else
                    p.drawText(mTipRect.adjusted(0, 0, 0, -mTipRect.height()/2), Qt::AlignCenter, mTipXLab + stringForLocal(mTipX));

                p.drawText(mTipRect.adjusted(0, mTipRect.height()/2., 0, 0), Qt::AlignCenter, " ⇨ " + mTipComment);
            }
        } else if (!mTipYLab.isEmpty()) {
            if (mUnitFunctionY)
                p.drawText(mTipRect, Qt::AlignCenter, mTipYLab + stringForLocal(mUnitFunctionY(mTipY)));
            else
                p.drawText(mTipRect, Qt::AlignCenter, mTipYLab + stringForLocal(mTipY));
       }
    }
}


/**
 * @brief Draw graphics
 */
void GraphView::paintToDevice(QPaintDevice* device)
{
    QPainter p(device);
    p.setFont(this->font());

    p.setRenderHints(QPainter::Antialiasing);

    /* ----------------------- Background ----------------------*/

    p.fillRect(rect(), mBackgroundColor);

    QFont font = p.font();

    /* ----------------------------------------------------
     *  Horizontal axis
     * ----------------------------------------------------*/
     QPen pen = QPen(Qt::black, 1);
     pen.setWidth(pen.width() * mThickness);
     p.setPen(pen);
     if (!mLegendX.isEmpty() && mXAxisValues) {
          QRectF tr(mMarginLeft, mGraphHeight + mMarginTop - mMarginBottom, mGraphWidth, mMarginBottom);
          p.setPen(Qt::black);
          p.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, mLegendX);
     }

    mAxisToolX.mShowArrow = mXAxisArrow;
    mAxisToolX.mShowSubs = mXAxisTicks;
    mAxisToolX.mShowSubSubs = mXAxisSubTicks;
    mAxisToolX.mShowText = mXAxisValues;

    mAxisToolX.updateValues(int (mGraphWidth), int (mStepMinWidth), mCurrentMinX, mCurrentMaxX);
    mAxisToolX.paint(p, QRectF(mMarginLeft, mMarginTop + mGraphHeight , mGraphWidth , mMarginBottom), -1.,mUnitFunctionX);

    /* ----------------------------------------------------
     *  Vertical axis
     * ----------------------------------------------------*/
    adjustYScale();

    if (mYAxisLine) {
        mAxisToolY.mShowArrow = mYAxisArrow;
        mAxisToolY.mShowSubs = mYAxisTicks;
        mAxisToolY.mShowSubSubs = mYAxisSubTicks;

        if (mAutoAdjustYScale && mYAxisMode == eHidden && mShowInfos) {
            const QString minMaxText = QString(tr( "Min = %1  /  Max = %2")).arg(stringForLocal(mMinY), stringForLocal(mMaxY));
            mInfos.clear();
            mInfos.append(minMaxText);
        }

        mAxisToolY.updateValues(int (mGraphHeight), int (mStepMinWidth), mMinY, mMaxY);
        mAxisToolY.paint(p, QRectF(0, mMarginTop, mMarginLeft, mGraphHeight), -1., mUnitFunctionY);

     }


    /* ---------------------- Zones --------------------------*/

    for (auto&& zone : mZones) {
        QRect r(int (getXForValue(zone.mXStart)),
                int (mMarginTop),
                int (getXForValue(zone.mXEnd) - getXForValue(zone.mXStart)),
                int (mGraphHeight));

        if (r.width()>1) {
            p.save();
            p.setClipRect(r);

            p.fillRect(r, zone.mColor);
            p.setPen(zone.mColor.darker());
            QFont fontZone = p.font();
            fontZone.setWeight(QFont::Black);
            fontZone.setPointSizeF(pointSize(20));
            p.setFont(fontZone);
            p.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, zone.mText);
            p.restore();
        }
    }

    /* mOverflowArrowMode, draw a arrow on the rigth or on the left
     * if the data are over the window (current min, current max)  */

     if (mOverflowArrowMode != eNone) {
         qreal arrowSize (7.);
         const qreal yo (mGraphHeight/2.) ;
         const QColor gradColDark(150, 150, 150);
         const QColor gradColLigth(190, 190, 190);

         if (mOverflowArrowMode == eBothOverflow || mOverflowArrowMode == eUnderMin) {

             type_data maxData = type_data (- INFINITY);
             for (auto& curve : mCurves)
                 if (curve.mVisible && curve.mData.size()>0)
                    maxData = std::max(maxData, curve.mData.lastKey());

                 else if (curve.mVisible && curve.isVerticalLine())
                     maxData = std::max(maxData, curve.mVerticalValue);

                 else if (curve.mVisible && curve.isHorizontalSections())
                     for (auto& section : curve.mSections )
                         maxData = std::max(maxData, section.second);


             if ( maxData <= mCurrentMinX) {
                 const qreal xl = mMarginLeft + 2*arrowSize;
                 const QPolygonF LeftTriangle (std::initializer_list<QPointF>({ QPointF(xl - arrowSize*1.5, yo),
                                                                      QPointF(xl, yo - arrowSize),
                                                                      QPointF(xl , yo + arrowSize) }));
                 QLinearGradient linearGradientL(xl - arrowSize, yo, xl, yo);
                         linearGradientL.setColorAt(0, gradColDark );
                         linearGradientL.setColorAt(1, gradColLigth );
                 p.setBrush(linearGradientL);
                 p.setPen(Qt::NoPen);
                 p.drawPolygon(LeftTriangle);
             }

         }
         if (mOverflowArrowMode == eBothOverflow || mOverflowArrowMode == eOverMax) {
             type_data minData = type_data (INFINITY);

             for (auto& curve : mCurves) {
                 if (curve.mVisible && curve.mData.size()>0)
                    minData = std::min(minData, curve.mData.firstKey());

                 else if (curve.mVisible && curve.isVerticalLine())
                     minData = std::min(minData, curve.mVerticalValue);

                 else if (curve.mVisible && curve.isHorizontalSections())
                     for (auto& section : curve.mSections )
                         minData = std::min(minData, section.first);
             }

             if (mCurrentMaxX <= minData) {
                 const qreal xr = mGraphWidth + mMarginLeft - 2*arrowSize;
                 const QPolygonF RigthTriangle (std::initializer_list<QPointF>({ QPointF(xr + arrowSize*1.5, yo),
                                                                      QPointF(xr, yo - arrowSize),
                                                                      QPointF(xr , yo + arrowSize) }));

                 QLinearGradient linearGradientR(xr, yo, xr + arrowSize, yo);
                         linearGradientR.setColorAt(0, gradColLigth );
                         linearGradientR.setColorAt(1, gradColDark);

                 p.setBrush(linearGradientR);
                 p.setPen(Qt::NoPen);

                 p.drawPolygon(RigthTriangle);
             }
         }

     }



    /* ----------------------------------------------------
     *  Curves
     * ----------------------------------------------------*/
      drawCurves(p);


    /* ----------------------------------------------------
     *  Graph specific infos at the top right
     * ----------------------------------------------------*/
    // Used by Multicalibration
   //if (mShowInfos && mYAxisMode == eHidden) {
   if (mShowInfos) {
        QFontMetrics fm (font);
        p.setFont(font);
        p.setPen(Painting::borderDark);
        int y = 0;
        int lineH (fm.height());
        for (const auto& info : mInfos) {
            p.drawText(int (1.2 * mMarginLeft), int (mMarginTop  + y), int (mGraphWidth - 1.2*mMarginLeft -mMarginRight), lineH, Qt::AlignLeft | Qt::AlignBottom, info);
            y += lineH;
        }

    }
    p.end();
}

QPainterPath GraphView::makePath (const QMap<double, double> &map, const bool showBorder) const
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
            if ( mCurrentMinX<=iFirst.key() && iFirst.key()<= mCurrentMaxX ) {

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
                if (mMinY <= iFirst.value()  && iFirst.value()<= mMaxY) {
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
                    xNext = interpolate(mMaxY,  lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
                    yNext = mMaxY;

                } else if (iNext.value() < mMinY) { // 5 - =4.a
                    xNext = interpolate(mMinY,  lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
                    yNext = mMinY;

                } else {
                    xNext = iNext.key();
                    yNext = iNext.value();
                }
            } else {
                if (iNext.value() > mMaxY) { // 2-
                    xNext = interpolate(mMaxY,  lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
                    if (xNext < mCurrentMaxX) { // 2.a - = 1
                        yNext = mMaxY;

                    } else { // 2.b
                        xNext = mCurrentMaxX;
                        yNext = interpolateValueInQMap(mCurrentMaxX, map);
                    }

                } else if (iNext.value() < mMinY) { // 4 -
                    xNext = interpolate(mMinY,  lastGoodIter.value(), iNext.value(), lastGoodIter.key(), iNext.key());
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
    if ((mGraphWidth<=0) || (mGraphHeight<=0))
        return;

    painter.save();
    // Draw curves inside axis only (not in margins!)
    //painter.setClipRect(int (mMarginLeft), int (mMarginTop), int(mGraphWidth), int (mGraphHeight));

    for (auto& curve : mCurves) {
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
                    if (curve.mDataVector.isEmpty())
                        return;

                    QList<type_data> subData = getVectorDataInRange(curve.mDataVector, mCurrentMinX, mCurrentMaxX, mMinX, mMaxX);

                    QList<type_data> lightData;
                    const type_data dataStep = type_data(subData.size()) / type_data(mGraphWidth);
                    if (dataStep > 1) {
                        for (int i = 0; i < mGraphWidth; ++i) {
                            const int idx = int (floor(i * dataStep));
                            lightData.append(subData.at(idx));
                        }

                    } else
                        lightData = subData;

                    bool isFirst = true;

                    for (int i = 0; i<lightData.size(); ++i) {
                        // Use "dataStep" only if lightData is different of subData !
                        const type_data valueX = mCurrentMinX + ((dataStep > 1) ? i * dataStep : i);
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
                            //last_x = x;
                            last_y = y;

                        }
                    }
                    painter.drawPath(path);

                }
            }
        }
    }

    for (auto& refPoint : refPoints) {
        if (refPoint.isVisible()) {
            QPen pen = refPoint.pen;
            pen.setWidth(pen.width() * mThickness);
            painter.setPen(pen);

            type_data xmin = refPoint.Xmin;
            type_data xmax = refPoint.Xmax;
            type_data ymin = refPoint.Ymin;
            type_data ymax = refPoint.Ymax;
            if (xmax >= mCurrentMinX && xmin <= mCurrentMaxX && ymax >= mMinY && ymin <= mMaxY) {
                const type_data xmoy = (xmax + xmin) / 2.;
                const type_data ymoy = (ymax + ymin) / 2.;

                QPen refPointsPen = pen;
                refPointsPen.setColor(refPoint.color);
                refPointsPen.setBrush(refPoint.color);
                refPointsPen.setWidthF(pen.widthF());
                QPainterPath pathPoint;
                // ----
                const qreal penWidth = std::max(2., pen.widthF());

                const qreal yPlot = getYForValue(ymoy, true);
                const qreal xPlot = getXForValue(xmoy);

                qreal xMinPlot = getXForValue(xmin);
                qreal xMaxPlot = getXForValue(xmax);
                if (xMaxPlot - xMinPlot < 2*penWidth) {
                    const qreal xMean = (xMaxPlot+xMinPlot)/2.;
                    xMinPlot = xMean - penWidth;
                    xMaxPlot = xMean + penWidth;
                }

                const qreal rayPlot = 1.5*penWidth;

                QColor bg (getBackgroundColor());
                bg.setAlpha(100);
                const QRectF border_h (xMinPlot - 1, yPlot - (penWidth + 1)/2., xMaxPlot - xMinPlot + 1, penWidth + 1 );
                const QRectF border_v (xPlot -(penWidth + 1)/2., getYForValue(ymin, true) - (1+penWidth)/2., penWidth + 1, (getYForValue(ymax, true) - getYForValue(ymin, true)) + penWidth +2 );

                switch (refPoint.type) {
                case CurveRefPts::eCross:
                    refPointsPen.setWidthF(penWidth);

                    /*pathPoint.moveTo( xMinPlot, yPlot );
                    pathPoint.lineTo( xMaxPlot, yPlot );

                    pathPoint.moveTo( xPlot, getYForValue(ymin, true) );
                    pathPoint.lineTo( xPlot, getYForValue(ymax, true) );
                    painter.strokePath(pathPoint, refPointsPen);*/

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
                    //refPointsPen.setStyle(Qt::CustomDashLine);
                    //refPointsPen.setDashPattern(QList<qreal>{1, 1});
                    painter.setBrush(refPointsPen.brush());
                    painter.setPen(refPointsPen);
                    painter.strokePath(pathPoint, refPointsPen);
                    break;

                case CurveRefPts::eDotLineCross:
                    painter.drawEllipse(QRectF( xPlot - penWidth, yPlot - penWidth,  penWidth*2., penWidth*2.));

                    pathPoint.moveTo( xMinPlot, yPlot);
                    pathPoint.lineTo( xMaxPlot, yPlot);

                    refPointsPen.setWidthF(penWidth - 1.);
                    //refPointsPen.setStyle(Qt::DotLine);
                    refPointsPen.setStyle(Qt::CustomDashLine);
                    refPointsPen.setDashPattern(QList<qreal>{1, 5});

                    painter.setBrush(refPointsPen.brush());
                    painter.setPen(refPointsPen);
                    painter.strokePath(pathPoint, refPointsPen);

                    //pathPoint.clear();
                    refPointsPen.setStyle(Qt::SolidLine);
                    refPointsPen.setWidthF(penWidth);
                    //pathPoint.moveTo( xPlot, getYForValue(ymin, true));
                    //pathPoint.lineTo( xPlot, getYForValue(ymax, true));
                    //painter.strokePath(pathPoint, refPointsPen);

                    painter.setPen(QPen(bg, 1));
                    painter.drawRect(border_v);
                    //painter.drawRect(border_h);

                    break;

                case CurveRefPts::eCustomDashLineCross:
                    pathPoint.moveTo( xMinPlot, yPlot);
                    pathPoint.lineTo( xMaxPlot, yPlot);

                    refPointsPen.setWidthF(pen.widthF());
                    refPointsPen.setStyle(Qt::CustomDashLine);
                    refPointsPen.setDashPattern(QList<qreal>{5, 5});

                    painter.setBrush(refPointsPen.brush());
                    painter.setPen(refPointsPen);
                    painter.strokePath(pathPoint, refPointsPen);

                    pathPoint.clear();
                    refPointsPen.setStyle(Qt::SolidLine);
                    pathPoint.moveTo( xPlot, getYForValue(ymin, true));
                    pathPoint.lineTo( xPlot, getYForValue(ymax, true));
                    painter.strokePath(pathPoint, refPointsPen);

                    //painter.setPen(QPen(bg, 1));
                    //painter.drawRect(border_v);
                    //painter.drawRect(border_h);
                    break;

                case CurveRefPts::eRoundLine:
                    pathPoint.addEllipse(xPlot - rayPlot, yPlot - rayPlot, rayPlot*2., rayPlot*2.);

                    pathPoint.moveTo( xPlot, getYForValue(ymin, true));
                    pathPoint.lineTo( xPlot, getYForValue(ymax, true));

                    refPointsPen.setWidthF( pen.widthF()); // not penWidth
                    refPointsPen.setStyle(Qt::SolidLine);
                    painter.setBrush(Qt::NoBrush);
                    painter.setPen(refPointsPen);
                    painter.strokePath(pathPoint, refPointsPen);
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
    painter.restore();

}

void GraphView::drawMap(GraphCurve& curve, QPainter& painter)
{
    const double minY = curve.mMap.minY();
    const double maxY = curve.mMap.maxY();
    const double minX = curve.mMap.minX();
    const double maxX = curve.mMap.maxX();
    const double maxVal = curve.mMap.max_value;
    const double minVal = curve.mMap.min_value;

    const qreal rectXSize = (getXForValue(maxX, false) - getXForValue(minX, false) ) / (curve.mMap.column()-1);
    const qreal rectYSize = (getYForValue(minY, false) - getYForValue(maxY, false) ) / (curve.mMap.row()-1)  ;
    QPen rectPen;
    rectPen.setStyle(Qt::NoPen);

    QColor col;

    painter.setRenderHint(QPainter::Antialiasing);
    qreal xtop = getXForValue(minX, true);


    for (unsigned c1 = 1 ; c1 < curve.mMap.column(); c1++) {
        const double tReal = c1*(maxX-minX)/(curve.mMap.column()-1) + minX;
        qreal xbottom;
        if (tReal <= mCurrentMinX || c1 == (curve.mMap.column()-1) )
            xbottom = getXForValue(tReal, true);
        else
            xbottom = getXForValue(tReal, true) + rectXSize/2.;

        if (tReal< mCurrentMinX || mCurrentMaxX< tReal) {
            xtop = xbottom;
            continue;
        }


        double ytop = getYForValue(minY, true) + rectYSize/2.;

        for (unsigned r = 1 ; r < curve.mMap.row(); r++) {
            const double val = curve.mMap.at(c1, r);
            const qreal yReal = r*(maxY-minY)/(curve.mMap.row()-1) + minY;
            qreal ybottom = getYForValue(yReal, true) - rectYSize/2.;

            if ( val > minVal) {
#ifdef DEBUG
                if (false) {
                   /* col = QColor(Qt::yellow);

                    if (val > val99)
                        col = QColor(Qt::black);
                    else if (val > val95)
                        col = QColor(Qt::gray);
                    else if (val > val50)
                        col = QColor(Qt::blue);
                    else if ( val > val25)
                        col = QColor(Qt::red);

                    col.setAlphaF(0.5);
                    */
                } else {
#endif
                    const double alp = (val - minVal)/ (maxVal - minVal);
#ifdef DEBUG
                    if (alp > 1)
                        qDebug()<<"[GraphView::drawMap] alpha> 1 ??";
                    if (sqrt(alp) > 1)
                        qDebug()<<"[GraphView::drawMap] sqrt(alpha)> 1 ??";
#endif
                    col = QColor(curve.mPen.color());
                    col.setAlphaF(sqrt(alp));
#ifdef DEBUG
                }
#endif

                painter.setPen(Qt::NoPen);
                painter.setBrush(col);

                QRectF rectMap;
                rectMap.setCoords(xtop, ytop, xbottom, ybottom);
                painter.drawRect(rectMap);

            }
            ytop = ybottom;
        }
        xtop = xbottom;
    }

}

QPainterPath GraphView::makeSubShape(QMap<double, double> mapInf, QMap<double, double> mapSup) const
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

void GraphView::drawShape(GraphCurve &curve, QPainter& painter)
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

void GraphView::drawDensity(GraphCurve &curve, QPainter& painter)
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
    type_data last_x = getXForValue(valueX);//(0);
    type_data last_y = getYForValue(type_data (0), true) ;//(0);

    QPainterPath path;
    //path.moveTo(mMarginLeft, mMarginTop + mGraphHeight);
    path.moveTo(last_x, last_y);
    // Detect square signal front-end without null value at the begin of the QMap
    // e.g calibration of Unif-typo-ref
    if (iter.hasNext()) {
        if (valueY == (iter.peekNext()).value()) {
          //  if (valueX >= mCurrentMinX && valueX <= mCurrentMaxX) {
                path.moveTo( getXForValue(valueX), getYForValue(type_data (0), true) );
                path.lineTo( getXForValue(valueX), getYForValue(valueY, true) );
          //  }
        }
    }

    iter.toFront();
    bool isFirst = true;

    if (curve.mBrush != Qt::NoBrush) {
        isFirst = false;
        last_x = getXForValue(valueX, true);
        last_y = getYForValue(type_data (0.), false);
    }
    while (iter.hasNext()) {
        iter.next();
        valueX = iter.key();
        valueY = iter.value();

       // if (valueX >= mCurrentMinX && valueX <= mCurrentMaxX) {
            qreal x = getXForValue(valueX, true);
            qreal y = getYForValue(valueY, false);

            if (isFirst) {
                if (curve.mIsRectFromZero) {
                    path.moveTo(x, getYForValue(0., false));
                    path.lineTo(x, y);
                } else
                    path.moveTo(x, y);

                isFirst=false;

            } else {

                if (curve.isHisto()) {
                    // histo bars must be centered around x value :
                    const qreal dx2 = (x - last_x)/2.;
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
       // }
    }




    if (path.elementCount()  == 1) { //there is only one value
        last_x = getXForValue(valueX, true);
        last_y = getYForValue(type_data (0.), false);
        path.lineTo(last_x, last_y);


    }

    // Detect square signal back-end without null value at the end of the QMap
    // e.i calibration of Unif-typo-ref

    if (curve.mIsRectFromZero && lightMap.size()>1) {
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
        path.lineTo(last_x, getYForValue(type_data (0.), true));

       // painter.setPen(curve.mPen);
        //QBrush brush = painter.brush();
        painter.fillPath(path, painter.brush());//, curve.mBrush);
        painter.strokePath(path, curve.mPen);//, pen);

    } else
        painter.drawPath(path);


}


//Save & Export

/**
 * @brief Export a density with locale setting and separator and specific step
 * @todo Maybe we can use QString QLocale::createSeparatedList(const QStringList & list) const
 */
void GraphView::exportCurrentDensities(const QString &defaultPath, const QLocale locale, const QString &csvSep, double step) const
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
        type_data xMin = INFINITY;
        type_data xMax = INFINITY;

        for (auto& curve : mCurves) {
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
            x = (double)(i)*step + xMin;
            list.clear();

            list << locale.toString(x);
            for (auto& curve : mCurves) {
                if (!curve.mData.empty() &&
                     curve.isDensityCurve() &&
                    curve.mVisible) {

                    const type_data xi = interpolateValueInQMap(x, curve.mData);
                    list<<locale.toString(xi, 'g', 15);

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


void GraphView::exportCurrentVectorCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, bool writeInRows, int offset) const
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

        qDebug()<<"[GraphView::exportCurrentVectorCurves] "<<" nbCurve to export"<<mCurves.size();

        for (auto&& c : mCurves ) {
            if ( !c.mVisible || c.mDataVector.empty() )
                continue;

            const QList<type_data>& data = c.mDataVector;
            // the new DataVector is longer than the last, we need to expand the size of rows
            if (data.size()>rowsCount-2) {
                abscissesWritten = false;
                rowsCount = rows.count();
            }

            if (!abscissesWritten) {
                for (qsizetype i = offset+rowsCount; i<data.size()+1; ++i) {
                    // we add 1 to the line number, because the index of vector start to 0-> false now 0 is the init
                    rows.append(QStringList(locale.toString(i-rowsCount+1))+emptyColumn);

                }
                abscissesWritten = true;

            }
            //prepare adding row
            emptyColumn<<"";

            if (abscissesWritten) {
                    rows[0] << c.mName;
                    for (int i=offset; i<data.size(); ++i)
                        rows[i-offset+1]<< locale.toString(data[i],'g', 15);

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
        for (const auto& r : rows) {
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

    const QString fiName = MainWindow::getInstance()->getNameProject();
    const QString defaultFilename = defaultPath + "/"+ fiName.mid(0, fiName.size()-4) +"_"+ graph_title +"graph_data.csv";

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
        double xMin = INFINITY;
        double xMax = -INFINITY;

        for (auto& curve : mCurves) {
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
            for (auto& c : mCurves)
                if (c.mVisible)
                    nbData = std::max(nbData, (int) c.mData.count());

            step = ceil(xMax - xMin)/ (nbData - 1);

        } else
            nbData = ceil(xMax - xMin)/ step + 1;

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
                    list<<c.mName + " Inf";
                    list<<c.mName + " Sup"; // for example env G
                }
            }
        }
        rows<<list;

        // 3 - Create Row, with each curve
        //  Create data in row
        double x;

        for (int i = 0; i < nbData; ++i) {
            x = (double)(i)*step + xMin;
            list.clear();

            list << locale.toString(x);

            for (auto& c : mCurves) {
                if (c.mVisible) {
                    if (c.isDensityCurve()) {
                        if (c.mData.firstKey()<= x && x<= c.mData.lastKey())
                            list<<locale.toString(interpolateValueInQMap(x, c.mData), 'g', 15); // for example G
                        else
                            list<< "NaN";

                    } else if (c.isFunction()) {
                        if (c.mData.firstKey()<= x && x<= c.mData.lastKey())
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
void GraphView::exportReferenceCurves(const QString &defaultPath, const QLocale locale, const QString &csvSep, double step, QString filename) const
{
    if (step <= 0)
        step = 1;

    if (filename == "") {
        const QString fiName = MainWindow::getInstance()->getNameProject();

        const QString defaultFilename = defaultPath + "/"+ fiName.mid(0, fiName.size()-4) + "_ref.csv";
        qDebug()<<"GraphView::exportReferenceCurves : "<<defaultFilename;
        filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                tr("Save Reference Curve as..."),
                                                defaultFilename, "CSV (*.csv)");
    }
    if (filename.isEmpty())
        return;

    QFile file(filename);

    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug()<<"[GraphView::exportReferenceCurves] Save Reference Curve as..."<<filename;

        QList<QStringList> rows;
        QStringList list;
        // 1 -Create the header
        list << "X Axis";
        list << "G";
        list << "err G";
        type_data xMin = INFINITY;
        type_data xMax = INFINITY;

        QMap <type_data, type_data> G, G_Sup;
        for (auto& curve : mCurves) {
            if (curve.mName == "G") {
                G = curve.mData;
                xMin = G.firstKey();
                xMax = G.lastKey();

            } else if (curve.mName == "G Env") {
                G_Sup = curve.mShape.second;
            }

        }
        if (std::isinf(xMin) || std::isinf(xMax))
            return;

        rows<<list;
        rows.reserve(ceil( (xMax - xMin)/step) );

        // 3 - Create Row, with each curve
        //  Create data in row

        int nbData = (xMax - xMin)/ step;
        QLocale csvLocal (locale);
        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);
        auto xMax_BCAD = DateUtils::convertFromAppSettingsFormat(xMax);
        auto xMin_BCAD = DateUtils::convertFromAppSettingsFormat(xMin);

        if (xMin_BCAD<xMax_BCAD) {
            for (int i = 0; i <= nbData; ++i) {
                const auto x = (type_data)(i)*step + xMin;
                const auto t = DateUtils::convertFromAppSettingsFormat(x);
                list.clear();

                list << csvLocal.toString(t);
                // Il doit y avoir au moins trois courbes G, GSup, Ginf et nous exportons G et ErrG
                const type_data xi = interpolateValueInQMap(x, G); // G
                const type_data err_xi = interpolateValueInQMap(x, G_Sup); // GSup
                list<<csvLocal.toString(xi, 'g', 15);
                list<<csvLocal.toString((err_xi-xi)/1.96, 'g', 15);

                //      }
                rows<<list;
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
                list<<csvLocal.toString(xi, 'g', 15);
                list<<csvLocal.toString((err_xi-xi)/1.96, 'g', 15);

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
QString GraphView::getInfo(char sep)
{
    return mInfos.join(sep);// ( isShow() ? mInfos.join(sep) : "");
}

bool GraphView::isShow()
{
    return mShowInfos;
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
    mMarginTop = 0;
    mMarginBottom = 0;
    mMarginLeft = 0;

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
GraphTitle:: GraphTitle(QString title, QWidget* parent):
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

    mMarginTop = mTitleHeight/2.;
    mMarginBottom = mTitleHeight/2.;
    mMarginLeft = mTitleHeight/2.;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}

GraphTitle::GraphTitle(QString title, QColor titleBarColor, QWidget* parent):
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

    mMarginTop = mTitleHeight/2.;
    mMarginBottom = 0;
    mMarginLeft = 20;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}

GraphTitle::GraphTitle(QString title, QString subTitle, QWidget* parent):
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

    mMarginTop =  mTitleHeight/2.;
    mMarginBottom = 0;
    mMarginLeft = 20;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();

}

GraphTitle::GraphTitle(QString title, QString subTitle, QColor backGround, QWidget* parent):
    mTitle(title),
    mSubTitle(subTitle),
    mBackgroundColor(backGround),
    mTitleBarColor(backGround)
{
    GraphViewAbstract::setParent(parent);

    QFont boldFont (font());
    boldFont.setBold(true);
    mTitleHeight = QFontMetrics(boldFont).height();

    mMarginTop =  mTitleHeight/2. ;
    mMarginBottom = 0;
    mMarginLeft = 20;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}

GraphTitle::GraphTitle(QString title, QString commentTitle, QString subTitle, QWidget* parent):
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

    mMarginTop =  mTitleHeight/2. ;
    mMarginBottom = 0;
    mMarginLeft = 20;

    QFont itaFont (font());
    itaFont.setItalic(true);
    mSubTitleHeight = QFontMetrics(itaFont).height();
}


GraphTitle::~GraphTitle()
{

}

qreal GraphTitle::height()
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
    const qreal w = parentWidget()->width();

    if (!mTitle.text().isEmpty()) {

        if (!mSubTitle.text().isEmpty()) {
            p.fillRect(0, 0, w , height(), mBackgroundColor);

            p.setPen(Qt::black);
            QFont boldFont (font());
            boldFont.setBold(true);

            QFont itaFont (font());
            itaFont.setItalic(true);

            p.setFont(boldFont);
            qreal fontShift = QFontMetrics(boldFont).height()/2.;
            p.drawStaticText(mMarginLeft, mTitleHeight/2. - fontShift + mMarginTop, mTitle);
            const int shift = QFontMetrics(boldFont).horizontalAdvance(mTitle.text());

            if (!mCommentTitle.text().isEmpty()) {
                p.setFont(itaFont);
                p.drawStaticText(mMarginLeft + shift, mTitleHeight/2. - fontShift + mMarginTop, mCommentTitle);

            }

            p.setFont(itaFont);
            fontShift = QFontMetrics(itaFont).height()/2.;
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
            p.drawStaticText(mMarginLeft, mTitleHeight/2. - fontShift + mMarginTop, mTitle);

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
        p.drawStaticText(mMarginLeft, mSubTitleHeight/2. - fontShift + mMarginTop, mSubTitle);

    }

}

void GraphTitle::repaintGraph(const bool aAlsoPaintBackground)
{
    (void) aAlsoPaintBackground;
    update();
}
