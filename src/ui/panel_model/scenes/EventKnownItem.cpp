/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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
#include "EventKnown.h"
#include "StdUtilities.h"
#include "Project.h"
#include <QtWidgets>


EventKnownItem::EventKnownItem(EventsScene* eventsScene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent):EventItem(eventsScene, event, settings, parent),
    mThumbH(20),
    mThumbVisible(true),
   mPhasesHeight (20)
{
    setEvent(event, settings);
    mScene = static_cast<AbstractScene*>(eventsScene);

    mTitleHeight = 25;
    mEltsHeight = 60;
}

EventKnownItem::~EventKnownItem()
{

}

void EventKnownItem::setEvent(const QJsonObject& event, const QJsonObject& settings)
{
    prepareGeometryChange();

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

        GraphView* graph = new GraphView();
        graph->setFixedSize(200, 50);
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

        GraphCurve curve;
        curve.mName = "Bound";
        curve.mBrush = Painting::mainColorLight;
        curve.mPen = QPen(Painting::mainColorLight, 2.);

        curve.mIsHorizontalSections = true;
        qreal tLower;
        qreal tUpper;

        tLower = bound.mFixed;
        tUpper = tLower;


        curve.mSections.append(qMakePair(tLower,tUpper));
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
}

QRectF EventKnownItem::boundingRect() const
{
    // the size is independant of the size name

    qreal h = mTitleHeight + mThumbH + mPhasesHeight + 2*AbstractItem::mEltsMargin;
    h += 50.;

    const  qreal w ( 210);

    return QRectF(-w/2, -h/2, w, h);
}

void EventKnownItem::setDatesVisible(bool visible)
{
    mThumbVisible = visible;
}

void EventKnownItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

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

    const double side = 30;//40.;
    const double top = 15;//25.;

    QRectF nameRect(rect.x() + side, rect.y() + top, rect.width() - 2*side, mTitleHeight);
    QRectF thumbRect(rect.x() + side, rect.y() + top + mEltsMargin + mTitleHeight, rect.width() - 2*side, mThumbH);

    if (mGreyedOut) //setting with setGreyedOut() just above
        painter->setOpacity(0.1);
    else
        painter->setOpacity(1.);

    // the elliptic item box
    painter->setPen(Qt::NoPen);
    painter->setBrush(eventColor);
    painter->drawEllipse(rect);

    // Name


   // QFont font (APP_SETTINGS_DEFAULT_FONT_FAMILY, 12, 50, false);
    QFont font (qApp->font());
    font.setPointSizeF(12.);

    QString name = mData.value(STATE_NAME).toString();
    const QFont ftAdapt = AbstractItem::adjustFont(font, name,nameRect);
   painter->setFont(ftAdapt);

   QFontMetrics metrics(ftAdapt);

    name = metrics.elidedText(name, Qt::ElideRight, int (nameRect.width() - 5));

    QColor frontColor = getContrastedColor(eventColor);
    painter->setPen(frontColor);
    painter->drawText(nameRect, Qt::AlignCenter, name);



    // thumbnail
    if (mThumb.isNull()) {
        painter->fillRect(thumbRect, Qt::white);
        painter->setPen(Qt::red);
        painter->drawText(thumbRect, Qt::AlignCenter, tr("Invalid bound"));
    }
    else if (mThumbVisible)
        painter->drawImage(thumbRect, mThumb, mThumb.rect());


    // Phases
    QJsonObject state = mScene->getProject()->mState;
    ChronocurveSettings chronocurveSettings = ChronocurveSettings::fromJson(state.value(STATE_CHRONOCURVE).toObject());

    if (chronocurveSettings.mEnabled) {
        int lines = getNumberChronocurveLines();

        //QRectF phasesRect(rect.x() + side, rect.y() + top + 2*mEltsMargin + mTitleHeight + mThumbH, rect.width() - 2*side, mPhasesHeight);
        QRectF paramRect(rect.x() + side, rect.y() + top + 2*mEltsMargin + mTitleHeight + mThumbH, rect.width() - 2*side, lines * mPhasesHeight);


        paramRect.adjust(4, 0, -4, -4);

        QPen pen = QPen();
        pen.setColor(CHRONOCURVE_COLOR_BORDER);
        pen.setWidth(2);
        painter->setPen(pen);
        painter->setBrush(CHRONOCURVE_COLOR_BACK);
        painter->drawRoundedRect(paramRect, 4, 4);

        int m = 3;
        paramRect.adjust(m, m, -m, -m);
        int lineX = paramRect.x();
        int lineY = paramRect.y();
        int lineH = paramRect.height() / lines;
        int lineW = paramRect.width();

        painter->setFont(font);
        painter->setPen(CHRONOCURVE_COLOR_TEXT);

        if (chronocurveSettings.showInclinaison()){
            QString text1;
            text1 += "Inc. = ";
            text1 += QLocale().toString (mData.value(STATE_EVENT_Y_INC).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_S_INC).toDouble());

            if(chronocurveSettings.showDeclinaison()){
                text1 += " Dec. = ";
                text1 += QLocale().toString(mData.value(STATE_EVENT_Y_DEC).toDouble());
            }
            painter->drawText(QRectF(lineX, lineY, lineW, lineH), Qt::AlignCenter, text1);
            //painter->drawText(paramRect, Qt::AlignCenter, text1);
        }

        if (chronocurveSettings.showIntensite()) {
            QString text2 = chronocurveSettings.intensiteLabel();
            text2 += " = ";
            text2 += QLocale().toString(mData.value(STATE_EVENT_Y_INT).toDouble());
            text2 += " ± " + QLocale().toString(mData.value(STATE_EVENT_S_INT).toDouble());
            painter->drawText(QRectF(lineX, lineY + (chronocurveSettings.showInclinaison() ? 1 : 0) * lineH, lineW, lineH), Qt::AlignCenter, text2);
        }

    } else {
        QRectF phasesRect(rect.x() + side, rect.y() + top + 2*mEltsMargin + mTitleHeight + mThumbH, rect.width() - 2*side, mPhasesHeight);

        phasesRect.adjust(1, 1, -1, -1);
        // detect selected phases and set mGreyedOut
        const QJsonArray phases = getPhases();
        const int numPhases = phases.size();
        const double w = phasesRect.width()/numPhases;

        if (numPhases == 0) {
            font.setPointSizeF(10.);
            QString noPhase = tr("No Phase");
            QFont ftAdapt = AbstractItem::adjustFont(font, noPhase, phasesRect);
            painter->setFont(ftAdapt);
            painter->fillRect(phasesRect, QColor(0, 0, 0, 180));
            painter->setPen(QColor(200, 200, 200));
            painter->drawText(phasesRect, Qt::AlignCenter, noPhase);
        } else {
            for (int i=0; i<numPhases; ++i) {
                QJsonObject phase = phases.at(i).toObject();
                QColor c(phase.value(STATE_COLOR_RED).toInt(),
                         phase.value(STATE_COLOR_GREEN).toInt(),
                         phase.value(STATE_COLOR_BLUE).toInt());
                painter->setPen(c);
                painter->setBrush(c);
                painter->drawRect(int (phasesRect.x() + i*w), int (phasesRect.y()), int(w), int (phasesRect.height()));
            }
        }
        painter->setPen(QColor(0, 0, 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(phasesRect);
    }



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
