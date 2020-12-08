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

#include "EventItem.h"
#include "Event.h"
#include "Phase.h"
#include "Date.h"
#include "Painting.h"
#include "DateItem.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "MainWindow.h"
#include "Project.h"
#include "ArrowTmpItem.h"

EventItem::EventItem(EventsScene* scene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent):AbstractItem(scene, parent),
    mWithSelectedPhase(false),
    mShowAllThumbs(true),
    mPhasesHeight (20)
{
    mTitleHeight = 20;
    mEltsHeight =  DateItem::mTitleHeight +  DateItem::mEltsHeight ;

    setEvent(event, settings);
    mScene = static_cast<AbstractScene*>(scene);

}

EventItem::~EventItem()
{
    mScene = nullptr;
}

/**
 * @brief EventItem::mousePressEvent Overwrite of AbstractItem::mousePressEvent
 * @param e
 */
void EventItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
     EventsScene* itemScene = dynamic_cast<EventsScene*>(mScene);

    if ((this != itemScene->currentEvent()) && (!itemScene->mDrawingArrow) && (e->modifiers() != Qt::ControlModifier)) {// && (!itemScene->mSelectKeyIsDown)) {
        itemScene->clearSelection();
    }

    if (itemScene->selectedItems().size()<2 ) {
        if (!itemScene->itemClicked(this, e))
            setZValue(2.);
        else
            itemScene->mTempArrow->setFrom(pos().x(), pos().y());
    }

    QGraphicsObject::mousePressEvent(e);
}

//Event Managment
QJsonObject& EventItem::getEvent()
{
    return mData;
}

void EventItem::setEvent(const QJsonObject& event, const QJsonObject& settings)
{
    prepareGeometryChange();

    // ----------------------------------------------
    //  Update item position and selection
    // ----------------------------------------------
    const bool isSelected = event.value(STATE_IS_SELECTED).toBool() || event.value(STATE_IS_CURRENT).toBool();
    setSelected(isSelected);
    setPos(event.value(STATE_ITEM_X).toDouble(),
                event.value(STATE_ITEM_Y).toDouble());

    // ----------------------------------------------
    //  Check if item should be greyed out
    // ----------------------------------------------
    const QJsonArray phases = getPhases();
    mWithSelectedPhase = false;

    for (auto &&phase : phases) {
            if ((mWithSelectedPhase == false) && (phase.toObject().value(STATE_IS_SELECTED) == true))
                    mWithSelectedPhase = true;
     }

    const bool noHide = dynamic_cast<EventsScene*>(mScene)->showAllThumbs();

    if (mWithSelectedPhase || noHide)
        setGreyedOut(false);
    else
        setGreyedOut(true);
    // ----------------------------------------------
    //  Calculate item size
    // ----------------------------------------------
    qreal h = mTitleHeight + mPhasesHeight + 1* AbstractItem::mBorderWidth + 1*AbstractItem::mEltsMargin;

    const QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();

    const int count = dates.size();
    if (count > 0)
        h += count * (mEltsHeight + AbstractItem::mEltsMargin);
    else
        h += AbstractItem::mEltsMargin + mEltsHeight;

     mSize = QSize(AbstractItem::mItemWidth, h);

    if (event.value(STATE_EVENT_DATES).toArray() != mData.value(STATE_EVENT_DATES).toArray() || mSettings != settings) {
        // ----------------------------------------------
        //  Delete Date Items
        // ----------------------------------------------
        QList<QGraphicsItem*> dateItems = childItems();
        for (int i=0; i<dateItems.size(); ++i) {
            mScene->removeItem(dateItems[i]);
            delete dateItems[i];
        }

        // ----------------------------------------------
        //  Re-create Date Items
        // ----------------------------------------------
        const QColor color(event.value(STATE_COLOR_RED).toInt(),
                     event.value(STATE_COLOR_GREEN).toInt(),
                     event.value(STATE_COLOR_BLUE).toInt());

        for (int i=0; i<dates.size(); ++i) {
            const QJsonObject date = dates.at(i).toObject();

            try {
                 qDebug()<< "EventItem::setEvent date 1" << date.value(STATE_NAME).toString()<<date.value(STATE_DATE_VALID).toBool();
                qDebug()<< "EventItem::setEvent date mUUID" << date.value(STATE_DATE_UUID).toString();
                DateItem* dateItem = new DateItem((EventsScene*) (mScene), date, color, settings);
                dateItem->setParentItem(this);
                dateItem->setGreyedOut(mGreyedOut);

                QPointF pos(0,
                            boundingRect().y() +
                            mTitleHeight +
                            AbstractItem::mBorderWidth +
                            1*AbstractItem::mEltsMargin +
                            i * (mEltsHeight + AbstractItem::mEltsMargin));
                dateItem->setPos(pos);
                dateItem->setOriginalPos(pos);
                dateItem = nullptr;
            }
            catch(QException &e){
                QMessageBox message(QMessageBox::Critical,
                                    qApp->applicationName() + " " + qApp->applicationVersion(),
                                    QString("EventItem::setEvent() Error : %1").arg(e.what()),
                                    QMessageBox::Ok,
                                    qApp->activeWindow());
                message.exec();
            }
        }
    }

    mData = event;
    mSettings = settings;

    // ----------------------------------------------
    //  Repaint based on mEvent
    // ----------------------------------------------
    //update(); //Done by prepareGeometryChange() at the beging of the function
}

/**
 ** @brief looking for selected dateItem in the QGraphicsScene
 */
bool EventItem::withSelectedDate() const
{
    const QList<QGraphicsItem*> datesItemsList = childItems();
    foreach (const QGraphicsItem* date, datesItemsList) {
        if (date->isSelected())
            return true;
    }
    return false;
}

void EventItem::setGreyedOut(const bool greyedOut)
{
    mGreyedOut = greyedOut;
    QList<QGraphicsItem*> children = childItems();
    for (int i=0; i<children.size(); ++i)
        static_cast<DateItem*>(children.at(i))->setGreyedOut(greyedOut);
    update();
}

void EventItem::updateGreyedOut()
{
    mGreyedOut = true;
    const QJsonObject state = mScene->getProject()->state();
    const QJsonArray phases = state.value(STATE_PHASES).toArray();
    QStringList selectedPhasesIds;

    for (int i=0; i<phases.size(); ++i) {
        QJsonObject phase = phases.at(i).toObject();
        const bool isSelected = phase.value(STATE_IS_SELECTED).toBool();
        if (isSelected)
            selectedPhasesIds.append(QString::number(phase.value(STATE_ID).toInt()));
    }
    if (selectedPhasesIds.size() == 0)
        mGreyedOut = false;

    else {
        const QString eventPhasesIdsStr = mData.value(STATE_EVENT_PHASE_IDS).toString();
        const QStringList eventPhasesIds = eventPhasesIdsStr.split(",");
        for (auto && ids : selectedPhasesIds) {
            if (eventPhasesIds.contains(ids)) {
                mGreyedOut = false;
                break;
            }
        }
    }
    setOpacity(mGreyedOut ? 0.3 : 1);
}

void EventItem::setDatesVisible(bool visible)
{
    QList<QGraphicsItem*> dateItems = childItems();
    for (auto &&item : dateItems)
        item->setVisible(visible);

}

// Events
void EventItem::updateItemPosition(const QPointF& pos)
{
    qDebug()<<"EventItem::updateItemPosition() pos="<<pos;
    mData[STATE_ITEM_X] = double (pos.x());
    mData[STATE_ITEM_Y] = double (pos.y());
}

void EventItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    handleDrop(e);
}

/**
 * @brief EventItem::handleDrop this function move a data from CSV file to an EventItem
 * @param e
 */
void EventItem::handleDrop(QGraphicsSceneDragDropEvent* e)
{
    e->acceptProposedAction();
    QJsonObject event = mData;
    EventsScene* scene = dynamic_cast<EventsScene*>(mScene);
    Project* project = scene->getProject();
    QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
    QList<QPair<QString, Date>> datesDragged = scene->decodeDataDrop(e);

    for (int i=0; i<datesDragged.size(); ++i) {
        QJsonObject date = datesDragged.at(i).second.toJson();
        date[STATE_ID] = project->getUnusedDateId(dates);
        if (date[STATE_NAME].toString() == "")
            date[STATE_NAME] = "No Name " + QString::number(date[STATE_ID].toInt());
        dates.append(date);
    }
    event[STATE_EVENT_DATES] = dates;

    project->updateEvent(event, QObject::tr("Dates added to event (CSV drag)"));
    scene->updateStateSelectionFromItem();
    scene->sendUpdateProject("Item selected", true, false); //  bool notify = true, bool storeUndoCommand = false
}

void EventItem::redrawEvent()
{
    prepareGeometryChange();
    // Set DateItems positions
    const QList<QGraphicsItem*> datesItemsList = childItems();
    // ----------------------------------------------
    //  Calculate item size
    // ----------------------------------------------
    const QJsonArray dates = getEvent().value(STATE_EVENT_DATES).toArray();
    int h = mTitleHeight + mPhasesHeight +1*AbstractItem::mBorderWidth + 1*AbstractItem::mEltsMargin;

    const int count = dates.size();
    if (count > 0)
        h += count * (mEltsHeight + AbstractItem::mEltsMargin);
    else
        h +=AbstractItem:: mEltsMargin + mEltsHeight;

#ifdef DEBUG
    if (count != datesItemsList.size())
        qDebug()<<"EventItem::redrawEvent()";
#endif

    mSize.setHeight(h);
    int i (0);
    for (QGraphicsItem* item: datesItemsList) {

        DateItem* dateItem = dynamic_cast<DateItem*>(item);
        if (dateItem) {
            dateItem->setGreyedOut(mGreyedOut);
            QPointF pos(0,
                        boundingRect().y() +  mTitleHeight +
                        1*AbstractItem::mEltsMargin +
                        i * (mEltsHeight + AbstractItem::mEltsMargin));
            dateItem->setPos(pos);
            dateItem->setOriginalPos(pos);
            ++i;
        }
    }

  //  update(); //Done by prepareGeometryChange() at the beging of the function
}

void EventItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect = boundingRect();

    QColor eventColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
                               mData.value(STATE_COLOR_GREEN).toInt(),
                               mData.value(STATE_COLOR_BLUE).toInt());
    // Phases
    const QJsonArray phases = getPhases();
    QRectF phasesRect(rect.x(), rect.y() + rect.height() - mPhasesHeight, rect.width(), mPhasesHeight);
    phasesRect.adjust(1, 1, -1, -1);

    const int numPhases ( phases.size());
    const qreal w = phasesRect.width()/numPhases;

    if (mGreyedOut) //setting with setGreyedOut() just above
        painter->setOpacity(0.35);
    else
        painter->setOpacity(1.);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(eventColor));
    painter->drawRect(rect);

   QFont font (qApp->font());
   //font.setPointSizeF(12.);
   font.setPixelSize(12);

    font.setStyle(QFont::StyleNormal);
    font.setBold(false);
    font.setItalic(false);
    if (numPhases == 0) {
        QString noPhase = tr("No Phase");
       // QFont ftAdapt = AbstractItem::adjustFont(font, noPhase, phasesRect);
        painter->setFont(font);//ftAdapt);
        painter->fillRect(phasesRect, QColor(0, 0, 0, 180));
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(phasesRect, Qt::AlignCenter, noPhase );

    } else {
        for (int i =0; i<numPhases; ++i) {
            const QJsonObject phase = phases.at(i).toObject();

            const QColor c(phase.value(STATE_COLOR_RED).toInt(),
                     phase.value(STATE_COLOR_GREEN).toInt(),
                     phase.value(STATE_COLOR_BLUE).toInt());
            painter->setPen(c);
            painter->setBrush(c);
            painter->drawRect(int (phasesRect.x() + i*w), int (phasesRect.y()), int(w), int (phasesRect.height()));
        }
    }

    //item box
    painter->setPen(QColor(0, 0, 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(phasesRect);

    // Name
   QRectF tr(rect.x() +AbstractItem:: mBorderWidth + 2*AbstractItem::mEltsMargin ,
              rect.y() + AbstractItem::mBorderWidth, // + AbstractItem::mEltsMargin,
              rect.width() - 2*AbstractItem::mBorderWidth - 4*AbstractItem::mEltsMargin,
              mTitleHeight);

    font.setPixelSize(14);
    font.setStyle(QFont::StyleNormal);
    font.setBold(true);
    font.setItalic(false);

    QString name = mData.value(STATE_NAME).toString();

     //QFont ftAdapt = AbstractItem::adjustFont(font, name, tr);
    painter->setFont(font);//ftAdapt);

    QFontMetrics metrics(font);//ftAdapt);
    name = metrics.elidedText(name, Qt::ElideRight, int (tr.width() - 5 ));

    const QColor frontColor = getContrastedColor(eventColor);
    painter->setPen(frontColor);
    painter->drawText(tr, Qt::AlignCenter, name);

    // restore Opacity from GreyedOut
    painter->setOpacity(1.);

    // Border
    painter->setBrush(Qt::NoBrush);
    if (mMergeable) {
        painter->setPen(QPen(Qt::white, 5.));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));

        painter->setPen(QPen(Painting::mainColorLight, 3., Qt::DashLine));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));

    } else if (isSelected() || withSelectedDate()) {
        painter->setPen(QPen(Qt::white, 5.));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));

        painter->setPen(QPen(Qt::red, 3.));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
    }

    painter->restore();
}

QJsonArray EventItem::getPhases() const
{
    QJsonObject state = mScene->getProject()->state();
    const QJsonArray allPhases = state.value(STATE_PHASES).toArray();

    const QString eventPhaseIdsStr = mData.value(STATE_EVENT_PHASE_IDS).toString();
    const QStringList eventPhaseIds = eventPhaseIdsStr.split(",");

    QJsonArray phases = QJsonArray();
    for (int i=0; i<allPhases.size(); ++i) {
        QJsonObject phase = allPhases.at(i).toObject();
        QString phaseId = QString::number(phase.value(STATE_ID).toInt());
        if (eventPhaseIds.contains(phaseId))
            phases.append(phase);
    }
    return phases;
}

// Geometry
QRectF EventItem::boundingRect() const
{
  return QRectF(-mSize.width()/2, -mSize.height()/2, mSize.width(), mSize.height());
}
