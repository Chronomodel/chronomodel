/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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
#include "Phase.h"
#include "Event.h"
#include "EventsScene.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "GraphView.h"
#include "EventBound.h"
#include "StdUtilities.h"
#include "Project.h"

#include <QtWidgets>


EventKnownItem::EventKnownItem(EventsScene* eventsScene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent):EventItem(eventsScene, event, settings, parent),
    mThumbH(20),
    mThumbVisible(true)
{
    mScene = static_cast<AbstractScene*>(eventsScene);

    mTitleHeight = 25;
    mEltsHeight = 60;

    setEvent(event, settings);
}

EventKnownItem::~EventKnownItem()
{

}

void EventKnownItem::setEvent(const QJsonObject& event, const QJsonObject& settings)
{
   // prepareGeometryChange();

    mData = event;

    // ----------------------------------------------
    //  Update item position and selection
    // ----------------------------------------------
    setSelected(mData.value(STATE_IS_SELECTED).toBool());
    setPos(mData.value(STATE_ITEM_X).toDouble(),
           mData.value(STATE_ITEM_Y).toDouble());
    // ----------------------------------------------
    //  Check if item should be greyed out
    // ----------------------------------------------
    //updateGreyedOut();
    mGreyedOut = false;
    // ----------------------------------------------
    //  Recreate thumb
    // ----------------------------------------------
    const double tmin = settings.value(STATE_SETTINGS_TMIN).toDouble();
    const double tmax = settings.value(STATE_SETTINGS_TMAX).toDouble();
    const double step = settings.value(STATE_SETTINGS_STEP).toDouble();

    EventKnown bound ;
    bound = EventKnown::fromJson(event);
    // if Fixed Bound with fixed value in study period or uniform Bound with bound.mUniformStart<bound.mUniformEnd
   /* if(  ( (bound.mKnownType==EventKnown::eFixed) && (tmin<=bound.mFixed) && (bound.mFixed<=tmax) )
      || ( (bound.mKnownType==EventKnown::eUniform) &&
           (bound.mUniformStart<bound.mUniformEnd)  && (bound.mUniformStart< tmax) && (bound.mUniformEnd>tmin)     )) */
    if ( (tmin<=bound.mFixed) && (bound.mFixed<=tmax) ) {
        bound.updateValues(tmin, tmax, step);

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

        graph->setXAxisMode(GraphView::eHidden);
        graph->setYAxisMode(GraphView::eHidden);

        //---------------------

        /*GraphCurve curve;
        curve.mName = "Bound";
        curve.mBrush = Painting::mainColorLight;
        curve.mPen = QPen(Painting::mainColorLight, 2.);

        curve.mType = GraphCurve::eHorizontalSections;
        qreal tLower;
        qreal tUpper;

        tLower = bound.mFixed;
        tUpper = tLower;
        curve.mSections.push_back(qMakePair(tLower,tUpper));

        */
        const GraphCurve curve = horizontalSection(qMakePair(bound.mFixed, bound.mFixed),"Bound", Painting::mainColorLight, QBrush(Painting::mainColorLight));

        graph->addCurve(curve);
        //---------------------

        mThumb = QImage(graph->size(),QImage::Format_ARGB32_Premultiplied);
        graph->render(&mThumb);
        delete graph;

    } else
        mThumb = QImage();

    // ----------------------------------------------
    //  Repaint based on mEvent
    // ----------------------------------------------
    //update(); Done by prepareGeometryChange() at the function start
    QJsonObject state = mScene->getProject()->mState;
    CurveSettings curveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());

    const int nbLines = getNumberCurveLines(curveSettings);
    mCurveTextHeight = (nbLines>0 ? nbLines*mCurveLineHeight: 0);
}

QRectF EventKnownItem::boundingRect() const
{
    // the size is independant of the size name
   // int nbLines = getNumberCurveLines();
   // mCurveTextHeight = (nbLines>0 ? nbLines*mCurveLineHeight + AbstractItem::mEltsMargin: 0);
    qreal h = mTitleHeight + mThumbH + mPhasesHeight + 2*AbstractItem::mEltsMargin + mCurveTextHeight;
    h += 55.;

    const qreal w = 230;

    return QRectF(-w/2, -h/2, w, h);
}

void EventKnownItem::setDatesVisible(bool visible)
{
    mThumbVisible = visible;
}

void EventKnownItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    (void) option;
    (void) widget;

    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect = boundingRect();

    const QColor eventColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
                               mData.value(STATE_COLOR_GREEN).toInt(),
                               mData.value(STATE_COLOR_BLUE).toInt());

    if (isSelected()) {
        painter->setPen(QPen(Painting::mainColorDark, 3.));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
    }

    // w is the same value fixed in DateItem::boundingRect()
    const qreal w = AbstractItem::mItemWidth - 2*(AbstractItem::mBorderWidth + AbstractItem::mEltsMargin);

    const qreal side = (rect.width() - w )/2.;
    const qreal top = 10;//25.;

    QRectF nameRect(rect.x() + side, rect.y() + top, rect.width() - 2*side, mTitleHeight);
    QRectF thumbRect(rect.x() + side, rect.y() + top + AbstractItem::mEltsMargin + mTitleHeight, w, mThumbH);

    if (mGreyedOut) //setting with setGreyedOut() just above
        painter->setOpacity(0.1);
    else
        painter->setOpacity(1.);

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
    QJsonObject state = mScene->getProject()->mState;
    CurveSettings curveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());
    //int nbLines = getNumberCurveLines(curveSettings);

    QRectF curveRect(rect.x() + side, rect.y() + top + 3*mEltsMargin + mTitleHeight + mThumbH, rect.width() - 2*side,  mCurveTextHeight);
    paintBoxCurveParameter(painter, curveRect, curveSettings);

    // Phases
    font.setPixelSize(12);


    QRectF phasesRect(rect.x() + side, rect.y() + top + 3*AbstractItem::mEltsMargin + mTitleHeight + mThumbH + mCurveTextHeight + ( mCurveTextHeight>0 ? 1*AbstractItem::mEltsMargin : 0 ), rect.width() - 2*side, mPhasesHeight);

    paintBoxPhases(painter, phasesRect);

    // Border
    painter->setBrush(Qt::NoBrush);
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
