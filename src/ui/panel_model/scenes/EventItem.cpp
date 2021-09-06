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
#include "CurveSettings.h"
#include "qlocale.h"

EventItem::EventItem(EventsScene* scene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent):AbstractItem(scene, parent),
    mWithSelectedPhase(false),
    mShowAllThumbs(true),
    mPhasesHeight (20)
{
    mTitleHeight = 20;
    mEltsHeight =  DateItem::mTitleHeight +  DateItem::mEltsHeight ;



    QFont font;
    //font.setPointSizeF(12.);
    font.setPixelSize(12);

    font.setStyle(QFont::StyleNormal);
    font.setBold(false);
    font.setItalic(false);

    const QFontMetricsF fm (font);

    mCurveLineHeight =  fm.height();

    EventItem::setEvent(event, settings);
    mScene = static_cast<AbstractScene*>(scene);

}

EventItem::~EventItem()
{
    QList<QGraphicsItem*> dateItems = childItems();
    int NItems = childItems().size()-1;
    for (int i = NItems; i >= 0; --i) {
        mScene->removeItem(dateItems[i]);
        dateItems[i]->setParentItem(nullptr);
        delete [] dateItems[i];
        //dateItems.removeAt(i);
    }
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

    if (itemScene->selectedItems().size() < 2 ) {
        if (!itemScene->itemClicked(this, e))
            setZValue(2.);
        else
            itemScene->mTempArrow->setFrom(pos().x(), pos().y());
    }

    QGraphicsObject::mousePressEvent(e);
}

//Event Managment
QJsonObject& EventItem::getData()
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
    setPos(event.value(STATE_ITEM_X).toDouble(), event.value(STATE_ITEM_Y).toDouble());
    
    // ----------------------------------------------
    //  Check if item should be greyed out
    // ----------------------------------------------
    const QJsonArray phases = getPhases();
    mWithSelectedPhase = false;

    for (auto&& phase : phases) {
        if (phase.toObject().value(STATE_IS_SELECTED).toBool()) {
            mWithSelectedPhase = true;
        }
    }
    const bool noHide = dynamic_cast<EventsScene*>(mScene)->showAllThumbs();
    EventItem::setGreyedOut(!mWithSelectedPhase && !noHide);
    
    // ----------------------------------------------
    //  Dates
    // ----------------------------------------------
    const QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
    if (event.value(STATE_EVENT_DATES).toArray() != mData.value(STATE_EVENT_DATES).toArray() || mSettings != settings) {
        // ----------------------------------------------
        //  Delete Date Items
        // ----------------------------------------------
        QList<QGraphicsItem*> dateItems = childItems();
        int NItems = childItems().size()-1;
        for (int i = NItems; i >= 0; --i) {
            mScene->removeItem(dateItems[i]);
            dateItems[i]->setParentItem(nullptr);
            //delete [] dateItems[i];
            dateItems.removeAt(i);
        }
        dateItems.clear();
//        dateItems = childItems();
//qDebug()<<dateItems.size();
        mData[STATE_EVENT_DATES] = QJsonArray();
        // ----------------------------------------------
        //  Re-create Date Items
        // ----------------------------------------------
        const QColor color(event.value(STATE_COLOR_RED).toInt(),
            event.value(STATE_COLOR_GREEN).toInt(),
            event.value(STATE_COLOR_BLUE).toInt());

 /*       QProgressDialog *progress = new QProgressDialog(QTranslator::tr("Calibration generation") + " : "+ event.value(STATE_NAME).toString(), QTranslator::tr("Wait") , 1, 10);
       progress->blockSignals(true);
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(nullptr);
        progress->setMinimumDuration(0);
        progress->setMinimum(0);
        progress->setMaximum(dates.size());
        progress->setMinimumWidth(int (progress->fontMetrics().boundingRect(progress->labelText()).width() * 1.5));
*/
        for (int i = 0; i < dates.size(); ++i) {

//           progress->setValue(i);
            const QJsonObject date = dates.at(i).toObject();

            try {
                DateItem* dateItem = new DateItem((EventsScene*) (mScene), date, color, settings);
                dateItem->setParentItem(this);
                dateItem->setGreyedOut(mGreyedOut);
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

//progress->deleteLater();
    }

    mData = event;
    mSettings = settings;

//QList<QGraphicsItem*>    dateItems = childItems();
//qDebug()<<dateItems.size();


    resizeEventItem();
    repositionDateItems();

    // ----------------------------------------------
    //  Repaint
    // ----------------------------------------------
    // update(); // Done by prepareGeometryChange() at the begining of the function
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
    for (int i = 0; i < children.size(); ++i){
        static_cast<DateItem*>(children.at(i))->setGreyedOut(greyedOut);
    }
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
    for (auto&& item : dateItems)
        item->setVisible(visible);

}

// Events
void EventItem::updateItemPosition(const QPointF& pos)
{
    mData[STATE_ITEM_X] = double (pos.x());
    mData[STATE_ITEM_Y] = double (pos.y());
}

void EventItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    handleDrop(e);
}

/**
 * @brief EventItem::handleDrop this function move one or several data from CSV file to one EventItem
 * We don't have to modify the Curve parameter of the Event
 * @param e
 */
void EventItem::handleDrop(QGraphicsSceneDragDropEvent* e)
{
    e->acceptProposedAction();
    QJsonObject event = mData;
    EventsScene* scene = dynamic_cast<EventsScene*>(mScene);
    Project* project = scene->getProject();
    QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
    
    QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>> droppedData = scene->decodeDataDrop(e);
    QList<QPair<QString, Date>> datesDragged = droppedData.first;
    //QList<QMap<QString, double>> curveData = droppedData.second;

    for (int i = 0; i < datesDragged.size(); ++i) {
        QJsonObject date = datesDragged.at(i).second.toJson();
        date[STATE_ID] = project->getUnusedDateId(dates);
        if (date[STATE_NAME].toString() == "")
            date[STATE_NAME] = "No Name " + QString::number(date[STATE_ID].toInt());
        dates.append(date);
    }
    event[STATE_EVENT_DATES] = dates;
    
    //Event::setCurveCsvDataToJsonEvent(event, curveData.at(0));

    project->updateEvent(event, QObject::tr("Dates added to event (CSV drag)"));
    scene->updateStateSelectionFromItem();
    scene->sendUpdateProject("Item selected", true, false); //  bool notify = true, bool storeUndoCommand = false
}

void EventItem::redrawEvent()
{
    resizeEventItem();
    repositionDateItems();
}

void EventItem::resizeEventItem()
{
    prepareGeometryChange();
    
    qreal y =  mTitleHeight + AbstractItem::mEltsMargin;
    qreal h = mEltsHeight + AbstractItem::mEltsMargin;

    qreal eventHeight = y + (childItems().count() * h);
    //qDebug() << "resizeEventItem dates count : " << dates.count();

    QJsonObject state = mScene->getProject()->mState;
    CurveSettings curveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());

    const int nbLines = getNumberCurveLines(curveSettings);

    mCurveTextHeight = (nbLines>0 ? nbLines*mCurveLineHeight: 0);

    eventHeight += mCurveTextHeight + mPhasesHeight + (nbLines>0 ? 3*AbstractItem::mEltsMargin : 2*AbstractItem::mEltsMargin);
    
    mSize = QSize(AbstractItem::mItemWidth, eventHeight);
}

void EventItem::repositionDateItems()
{
    const QList<QGraphicsItem*> datesItemsList = childItems();
    
    int i = 0;
    qreal y = boundingRect().y() + mTitleHeight + AbstractItem::mEltsMargin;
    qreal h = mEltsHeight + AbstractItem::mEltsMargin;
    
    for (QGraphicsItem* item: datesItemsList) {
        DateItem* dateItem = dynamic_cast<DateItem*>(item);
        if (dateItem) {
            QPointF pos(0, y + i * h);
            dateItem->setPos(pos);
            dateItem->setOriginalPos(pos);
            ++i;
        }
    }
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

    // Item background : color of the event
    // Use the opacity to "grey out"
    painter->setOpacity(mGreyedOut ? 0.35 : 1.);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(eventColor));
    painter->drawRect(rect);

    QFont font (qApp->font());
    font.setPixelSize(12);

    font.setStyle(QFont::StyleNormal);
    font.setBold(false);
    font.setItalic(false);
    
    QJsonObject state = mScene->getProject()->mState;
    CurveSettings curveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());

    const qreal vMarge = AbstractItem::mEltsMargin;
    const qreal hMarge = AbstractItem::mEltsMargin;

    QRectF curveRect(rect.x() + hMarge, rect.y() + rect.height() - mCurveTextHeight - mPhasesHeight - 2*vMarge, rect.width() - 2*hMarge, mCurveTextHeight);

    paintBoxCurveParameter(painter, curveRect, curveSettings);

    // Phases
    QRectF phasesRect(rect.x() + hMarge , rect.y() + rect.height() - mPhasesHeight -vMarge, rect.width() - 2*hMarge, mPhasesHeight);
    paintBoxPhases(painter, phasesRect)   ;

    // Name
    QRectF tr(rect.x() +AbstractItem:: mBorderWidth + 2*AbstractItem::mEltsMargin ,
        rect.y() + AbstractItem::mBorderWidth,
        rect.width() - 2*AbstractItem::mBorderWidth - 4*AbstractItem::mEltsMargin,
        mTitleHeight);

    font.setPixelSize(14);
    font.setStyle(QFont::StyleNormal);
    font.setBold(true);
    font.setItalic(false);

    QString name = mData.value(STATE_NAME).toString();

    painter->setFont(font);

    QFontMetrics metrics(font);
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
    for (int i = 0; i < allPhases.size(); ++i) {
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

int EventItem::getNumberCurveLines(CurveSettings& cs) const
{
    switch (cs.mProcessType) {
    case CurveSettings::eProcessTypeNone:
        return 0;
        break;
    case CurveSettings::eProcessTypeUnivarie:
        return 1;
        break;
    case CurveSettings::eProcessTypeSpherique:
        return 1;
        break;
    case CurveSettings::eProcessTypeVectoriel:
        return 2;
        break;
    case CurveSettings::eProcessType3D:
        return 2;
        break;
    }

}

void EventItem::paintBoxCurveParameter (QPainter* painter, QRectF rectBox, CurveSettings& cs )
{
    const int nbLines = getNumberCurveLines(cs);
    if (nbLines>0) {

        QPen pen = QPen();
        pen.setColor(CURVE_COLOR_BORDER);
        pen.setWidth(1);
        painter->setPen(pen);
        painter->setBrush(CURVE_COLOR_BACK);
        painter->drawRoundedRect(rectBox, 4, 4);

        const qreal lineX = rectBox.x();
        const qreal lineY = rectBox.y();

        const qreal lineW = rectBox.width();
        QFont font;
        font.setPixelSize(12);

        font.setStyle(QFont::StyleNormal);
        font.setBold(false);
        font.setItalic(false);
        painter->setFont(font);
        painter->setPen(CURVE_COLOR_TEXT);

        if ( cs.mProcessType == CurveSettings::eProcessType3D) {
            QString text1;

            text1 += "X = ";
            text1 += QLocale().toString (mData.value(STATE_EVENT_Y_INC).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_S_INC).toDouble());

            text1 += " Y = ";
            text1 += QLocale().toString(mData.value(STATE_EVENT_Y_DEC).toDouble());

            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);

            QString text2 = "Z = ";
            text2 += QLocale().toString(mData.value(STATE_EVENT_Y_INT).toDouble());
            text2 += " ± " + QLocale().toString(mData.value(STATE_EVENT_S_INT).toDouble());
            painter->drawText(QRectF(lineX, lineY + mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text2);

        } else {
            if (cs.showInclination()) {
                QString text1 = "Inc = ";
                text1 += QLocale().toString (mData.value(STATE_EVENT_Y_INC).toDouble());
                text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_S_INC).toDouble());

                if (cs.showDeclination()) {
                    text1 += " Dec = ";
                    text1 += QLocale().toString(mData.value(STATE_EVENT_Y_DEC).toDouble());
                }

                painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);
            }

            if (cs.showIntensity()) {
                QString text2 = cs.intensityLabel();
                text2 += " = ";
                text2 += QLocale().toString(mData.value(STATE_EVENT_Y_INT).toDouble());
                text2 += " ± " + QLocale().toString(mData.value(STATE_EVENT_S_INT).toDouble());
                painter->drawText(QRectF(lineX, lineY + (cs.showInclination() ? 1 : 0) * mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text2);
            }
        }

    }


}

void EventItem::paintBoxPhases (QPainter *painter, QRectF rectBox)
{
    QFont font, memoFont;
    memoFont = painter->font();

    font.setStyle(QFont::StyleNormal);
    font.setBold(false);
    font.setItalic(false);
    painter->setFont(font);
    const QJsonArray phases = getPhases();
    const int numPhases = phases.size();

    if (numPhases == 0) {

        //painter->fillRect(phasesRect, QColor(0, 0, 0, 180));
      //  painter->setBrush(QColor(0, 0, 0, 180));
      //  painter->drawRoundedRect(phasesRect, 4, 4);
        painter->setPen(QColor(200, 200, 200));

        QString noPhase = tr("No Phase");
        // QFont ftAdapt = AbstractItem::adjustFont(font, noPhase, phasesRect);
       // painter->setFont(font);//ftAdapt);
        painter->drawText(rectBox, Qt::AlignCenter, noPhase);

    } else {
        const qreal w = (rectBox.width() - (numPhases-1)*mEltsMargin) / numPhases;
        qreal dx = 0;
        for (int i = 0; i < numPhases; ++i) {
            dx = i*(w + mEltsMargin);
            const QJsonObject phase = phases.at(i).toObject();

            const QColor c(phase.value(STATE_COLOR_RED).toInt(),
                           phase.value(STATE_COLOR_GREEN).toInt(),
                           phase.value(STATE_COLOR_BLUE).toInt());
            painter->setPen(QColor(0, 0, 0));
            painter->setBrush(c);
            painter->drawRoundedRect( (rectBox.x() + dx),  (rectBox.y()), (w),  (rectBox.height()), 4, 4);

        }
    }

    //item box
/*    painter->setPen(QColor(0, 0, 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(phasesRect, 4, 4);*/

    painter->setFont(memoFont);

}
