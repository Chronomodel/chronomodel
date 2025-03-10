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

#include "EventKnownItem.h"

#include "EventsScene.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "GraphView.h"
#include "StateKeys.h"

#include <QtWidgets>


EventKnownItem::EventKnownItem(EventsScene* eventsScene, const QJsonObject& eventObj, const QJsonObject& settings, QGraphicsItem* parent):
    EventItem(eventsScene, eventObj, settings, parent),
    mThumbH (20)
{
    mEltsHeight = 60;
    mSize = QSize(230, 136);
    EventKnownItem::setEvent(eventObj, settings);
}

EventKnownItem::~EventKnownItem()
{

}

void EventKnownItem::setEvent(const QJsonObject& eventObj, const QJsonObject& settings)
{
    prepareGeometryChange();

    mData = eventObj;
    // ----------------------------------------------
    //  Update item position and selection
    // ----------------------------------------------
    setSelected(mData.value(STATE_IS_SELECTED).toBool());
    setPos(mData.value(STATE_ITEM_X).toDouble(),
           mData.value(STATE_ITEM_Y).toDouble());

    // ----------------------------------------------
    //  Recreate thumb
    // ----------------------------------------------
    const double tmin = settings.value(STATE_SETTINGS_TMIN).toDouble();
    const double tmax = settings.value(STATE_SETTINGS_TMAX).toDouble();

    const double fixed = eventObj[STATE_EVENT_KNOWN_FIXED].toDouble();

    if ( (tmin<=fixed) && (fixed<=tmax) ) {

        GraphView* graph = new GraphView(); // AbstractItem::mItemWidth
        //graph->setFixedSize(200, 50);
        qreal w = AbstractItem::mItemWidth - 2*(AbstractItem::mBorderWidth + AbstractItem::mEltsMargin);
        graph->setFixedSize(w, 50);
        graph->setMargins(0, 0, 0, 0);

        graph->setRangeX(tmin, tmax);
        graph->setCurrentX(tmin, tmax);
        graph->setRangeY(0, 1.);

        graph->showXAxisArrow(false);
        graph->showXAxisTicks(false);
        graph->showXAxisSubTicks(false);
        graph->showXAxisValues(false);

        graph->showYAxisArrow(false);
        graph->showYAxisTicks(false);
        graph->showYAxisSubTicks(false);
        graph->showYAxisValues(false);

        graph->setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
        graph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

        graph->setXAxisMode(GraphView::eHidden);
        graph->setYAxisMode(GraphView::eHidden);

        //---------------------

        GraphCurve curve = horizontalSection(qMakePair(fixed, fixed),"Bound", Painting::mainColorLight, QBrush(Painting::mainColorLight));
        curve.mVisible = true;
        graph->add_curve(curve);


        mThumb = QImage(graph->size(),QImage::Format_ARGB32_Premultiplied);
        graph->render(&mThumb);
        delete graph;
        graph = nullptr;

    } else
        mThumb = QImage();

    //delete bound;
    // ----------------------------------------------
    //  Repaint based on mEvent
    // ----------------------------------------------
    //update(); Done by prepareGeometryChange() at the function start

    CurveSettings curveSettings = CurveSettings::fromJson(getState_ptr()->value(STATE_CURVE).toObject());

    const int nbLines = getNumberCurveLines(curveSettings);
    mCurveTextHeight = (nbLines>0 ? nbLines*mCurveLineHeight: 0);

    const qreal h = mTitleHeight + mThumbH + mPhasesHeight + 2*AbstractItem::mEltsMargin + mCurveTextHeight + 55. + (isCurveNode()? 2*mNodeSkin + 4.: 0.);

    mSize = QSize(230 + (isCurveNode()? 2*mNodeSkin + 2.: 0.), h);

    update();
}

void EventKnownItem::setDatesVisible(const bool visible)
{
    mThumbVisible = visible;
}

void EventKnownItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* , QWidget* )
{
    painter->setRenderHint(QPainter::Antialiasing);

    const QRectF rectTotal = rectF();//QRectF(-mSize.width()/2, -mSize.height()/2, mSize.width(), mSize.height());
    const QRectF rect = isCurveNode() ? rectTotal.adjusted(mNodeSkin + 1., mNodeSkin + 1., -mNodeSkin - 1., -mNodeSkin - 1.) : rectTotal;

    const QColor eventColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
                               mData.value(STATE_COLOR_GREEN).toInt(),
                               mData.value(STATE_COLOR_BLUE).toInt());

    if (isSelected()) {
        painter->setPen(QPen(Painting::mainColorDark, 3.));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
    }

    const qreal w = AbstractItem::mItemWidth - 2*(AbstractItem::mBorderWidth + AbstractItem::mEltsMargin);

    const qreal side = (rect.width() - w )/2.;
    const qreal top = 10;

    const QRectF nameRect(rect.x() + side, rect.y() + top, rect.width() - 2*side, mTitleHeight);
    const QRectF thumbRect(rect.x() + side, rect.y() + top + AbstractItem::mEltsMargin + mTitleHeight, w, mThumbH);

    painter->setOpacity(mGreyedOut ? 0.35 : 1.);

    // The elliptic item box
    painter->setPen(Qt::NoPen);
    painter->setBrush(eventColor);
    painter->drawEllipse(rect);

    // Name
    QString name = mData.value(STATE_NAME).toString();

    QFont font ;
    font.setPixelSize(14);
    font.setStyle(QFont::StyleNormal);
    font.setBold(true);
    font.setItalic(false);
    painter->setFont(font);

    QFontMetrics fm(font);

    name = fm.elidedText(name, Qt::ElideRight, int (nameRect.width() - 5));

    QColor frontColor = getContrastedColor(eventColor);
    painter->setPen(frontColor);
    painter->drawText(nameRect, Qt::AlignCenter, name);


    // Thumbnail
    if (mThumb.isNull()) {
        painter->fillRect(thumbRect, Qt::white);
        painter->setPen(Qt::red);
        painter->drawText(thumbRect, Qt::AlignCenter, tr("Invalid bound"));
    }
    else if (mThumbVisible)
        painter->drawImage(thumbRect, mThumb, mThumb.rect());

    // Phases
    CurveSettings curveSettings = CurveSettings::fromJson(getState_ptr()->value(STATE_CURVE).toObject());

    QRectF curveRect(rect.x() + side, rect.y() + top + 3*mEltsMargin + mTitleHeight + mThumbH, rect.width() - 2*side,  mCurveTextHeight);

    if (mThumbVisible)
        paintBoxCurveParameter(painter, curveRect, curveSettings);

    // Phases
    font.setPixelSize(12);


    QRectF phasesRect(rect.x() + side, rect.y() + top + 3*AbstractItem::mEltsMargin + mTitleHeight + mThumbH + mCurveTextHeight + ( mCurveTextHeight>0 ? 1*AbstractItem::mEltsMargin : 0 ), rect.width() - 2*side, mPhasesHeight);

    paintBoxPhases(painter, phasesRect);

    // Border
    painter->setBrush(Qt::NoBrush);
    if (isCurveNode() && curveRect.height()>0) {
        painter->setPen(QPen(Painting::mainColorDark, 2.5));
        painter->drawEllipse(rectTotal);
    }

    if (mMergeable) {
        painter->setPen(QPen(Qt::white, 5.));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));

        painter->setPen(QPen(Painting::mainColorLight, 3., Qt::DashLine));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));

    } else if (isSelected()){
        painter->setPen(QPen(Qt::white, 5.));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));

        painter->setPen(QPen(Qt::red, 3.));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
    }
    // restore Opacity
    painter->setOpacity(1.);

}

void EventKnownItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    e->ignore();
}

QRectF EventKnownItem::toggleRect() const
{
    return QRectF();
}
