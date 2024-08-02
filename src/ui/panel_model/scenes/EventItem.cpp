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

#include "EventItem.h"

#include "Date.h"
#include "Painting.h"
#include "DateItem.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "Project.h"
#include "ArrowTmpItem.h"
#include "CurveSettings.h"
#include "qlocale.h"

EventItem::EventItem(EventsScene* scene, const QJsonObject &event, const QJsonObject &StudyPeriodSettings, QGraphicsItem* parent):
    AbstractItem(scene, parent),
    mStudyPeriodSettings(StudyPeriodSettings),
    mWithSelectedPhase (false),
    mThumbVisible (true),
    mNodeSkin (7.),
    mPhasesHeight (20)
{
    mScene = static_cast<AbstractScene*>(scene);
    mEltsHeight =  DateItem::mTitleHeight +  DateItem::mEltsHeight ;
    mGreyedOut = false;

    QFont font;
    font.setPixelSize(12);

    font.setStyle(QFont::StyleNormal);
    font.setBold(false);
    font.setItalic(false);

    const QFontMetricsF fm (font);

    mCurveLineHeight =  fm.height();

    if (Event::Type (event.value(STATE_EVENT_TYPE).toInt()) == Event::eDefault )
        EventItem::setEvent(event, StudyPeriodSettings);

}

EventItem::~EventItem()
{
    mData = QJsonObject();
    remove_dateItems();
}

void EventItem::remove_dateItems()
{
    QList<QGraphicsItem*> dateItems = childItems();
    const auto dateItemsSize = dateItems.size() - 1;
    for (auto j = dateItemsSize; j >= 0; --j) {
        if (mScene->getItemsList().contains(dateItems.first()))
            mScene->removeItem(dateItems.first());

        delete dateItems.first();
        dateItems.removeFirst();

    }
    dateItems.clear();
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

#pragma mark Event Managment

void EventItem::setEvent(const QJsonObject &event, const QJsonObject &StudyPeriodSettings)
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
    if (event.value(STATE_EVENT_DATES).toArray() != mData.value(STATE_EVENT_DATES).toArray() || mStudyPeriodSettings != StudyPeriodSettings) {
        // ----------------------------------------------
        //  Delete Date Items
        // ----------------------------------------------
        remove_dateItems();

        mData[STATE_EVENT_DATES] = QJsonArray();
        // ----------------------------------------------
        //  Re-create Date Items
        // ----------------------------------------------
        const QColor color(event.value(STATE_COLOR_RED).toInt(),
            event.value(STATE_COLOR_GREEN).toInt(),
            event.value(STATE_COLOR_BLUE).toInt());

        for (int i = 0; i < dates.size(); ++i) {
            const QJsonObject date = dates.at(i).toObject();

            try {
                DateItem* dateItem = new DateItem((EventsScene*) (mScene), date, color, StudyPeriodSettings);
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

    }

    mData = event;
    mStudyPeriodSettings = StudyPeriodSettings;

    resizeEventItem();
    repositionDateItems();
    update();
}

bool EventItem::isCurveNode() const
{
    const QJsonObject &state = mScene->getProject()->mState;
    const CurveSettings curveSettings (state.value(STATE_CURVE).toObject());
    const bool withNode = (curveSettings.mLambdaSplineType != CurveSettings::eInterpolation)
                          && (curveSettings.mVarianceType == CurveSettings::eModeBayesian);

    return (Event::PointType (mData.value(STATE_EVENT_POINT_TYPE).toInt()) == Event::eNode) && withNode ;
}

/**
 ** @brief looking for selected dateItem in the QGraphicsScene
 */
bool EventItem::withSelectedDate() const
{
    const QList<QGraphicsItem*>& datesItemsList = childItems();
    for (const QGraphicsItem* date : datesItemsList) {
        if (date && date->isSelected())
            return true;
    }
    return false;
}

void EventItem::setGreyedOut(const bool greyedOut)
{
    mGreyedOut = greyedOut;
    QList<QGraphicsItem*> children = childItems();
    for (auto &c : children){
        static_cast<DateItem*>(c)->setGreyedOut(greyedOut);
    }
    update();
}

void EventItem::updateGreyedOut()
{
    const QJsonArray &phases = mScene->getState().value(STATE_PHASES).toArray();
    mGreyedOut = true;

    QStringList selectedPhasesIds;

    for (const auto &&p : phases) {
        QJsonObject phase = p.toObject();
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

    QList<QGraphicsItem*> dateItems = childItems();
    for (auto&& item : dateItems) {
        DateItem *di = (DateItem*) item;
        di->setGreyedOut(mGreyedOut);
    }
}

void EventItem::setDatesVisible(bool visible)
{
    mThumbVisible = visible;
    QList<QGraphicsItem*> dateItems = childItems();
    for (auto&& item : dateItems)
        item->setVisible(visible);

}

#pragma mark Events

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
    std::shared_ptr<Project> project = scene->getProject();
    QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
    
    QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>> droppedData = scene->decodeDataDrop(e);
    const QList<QPair<QString, Date>> &datesDragged = droppedData.first;

    for (int i = 0; i < datesDragged.size(); ++i) {
        QJsonObject date = datesDragged.at(i).second.toJson();
        if (date.value(STATE_DATE_PLUGIN_ID)  == -1) // it's invalid or a bound
            continue;
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
    resizeEventItem();
    repositionDateItems();
}

void EventItem::resizeEventItem()
{
    prepareGeometryChange();
    
    const qreal y = mTitleHeight + AbstractItem::mEltsMargin;
    const qreal h = mEltsHeight + AbstractItem::mEltsMargin;

    qreal eventHeight = y + (childItems().count() * h);
    //qDebug() << "resizeEventItem dates count : " << dates.count();

    const QJsonObject &state = mScene->getProject()->mState;
    const CurveSettings &curveSettings (state.value(STATE_CURVE).toObject());

    const int nbLines = getNumberCurveLines(curveSettings);

    mCurveTextHeight = (nbLines>0 ? nbLines*mCurveLineHeight: 0);

    eventHeight += mCurveTextHeight + mPhasesHeight + (nbLines>0 ? 3*AbstractItem::mEltsMargin : 2*AbstractItem::mEltsMargin);
    
    eventHeight += (isCurveNode()? 2*mNodeSkin + 4.: 0.);

    mSize = QSize(AbstractItem::mItemWidth + (isCurveNode()? 2*mNodeSkin + 4.: 0.), eventHeight);
 }

void EventItem::repositionDateItems()
{
    const QList<QGraphicsItem*> datesItemsList = childItems();
    
    int i = 0;
    const QRectF rectTotal = rectF();//QRectF(-mSize.width()/2, -mSize.height()/2, mSize.width(), mSize.height());
    const QRectF rect = isCurveNode() ? rectTotal.adjusted(mNodeSkin + 1., mNodeSkin + 1., -mNodeSkin -1., -mNodeSkin - 1.) : rectTotal;

    qreal y = rect.y() + mTitleHeight + AbstractItem::mEltsMargin;
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

    const QRectF rectTotal = QRectF(-mSize.width()/2, -mSize.height()/2, mSize.width(), mSize.height());
    const QRectF rect = isCurveNode() ? rectTotal.adjusted(mNodeSkin + 1., mNodeSkin + 1., -mNodeSkin - 1., -mNodeSkin - 1.) : rectTotal;

    QColor eventColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
       mData.value(STATE_COLOR_GREEN).toInt(),
       mData.value(STATE_COLOR_BLUE).toInt());

    // Item background : color of the event
    // Use the opacity to "grey out"
    painter->setOpacity(mGreyedOut ? 0.35 : 1.);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(eventColor));
    painter->drawRect(rect);

    QFont font (qApp->font().family(), 12.);
    //font.setPixelSize(12);

    font.setStyle(QFont::StyleNormal);
    font.setBold(false);
    font.setItalic(false);
    
    QJsonObject state = mScene->getProject()->mState;
    CurveSettings curveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());

    const qreal vMarge = AbstractItem::mEltsMargin;
    const qreal hMarge = AbstractItem::mEltsMargin;

    QRectF curveRect(rect.x() + hMarge, rect.y() + rect.height() - mCurveTextHeight - mPhasesHeight - 2*vMarge, rect.width() - 2*hMarge, mCurveTextHeight);
    if (mThumbVisible)
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

    // it's a curve Node
    painter->setBrush(Qt::NoBrush);
    if (isCurveNode() && curveRect.height()>0) {
        painter->setPen(QPen(Painting::mainColorDark, 2.5));
        painter->drawRoundedRect(rectTotal, 4, 4);
    }

    // Border
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
    const QJsonArray allPhases = mScene->getState().value(STATE_PHASES).toArray();

    const QString eventPhaseIdsStr = mData.value(STATE_EVENT_PHASE_IDS).toString();
    const QStringList eventPhaseIds = eventPhaseIdsStr.split(",");

    QJsonArray phases = QJsonArray();
    for (const auto &&p : allPhases) {
        const QJsonObject &phase = p.toObject();
        QString phaseId = QString::number(phase.value(STATE_ID).toInt());
        if (eventPhaseIds.contains(phaseId))
            phases.append(phase);
    }
    return phases;
}

#pragma mark Geometry

int EventItem::getNumberCurveLines(const CurveSettings &cs) const
{
    switch (cs.mProcessType) {
    case CurveSettings::eProcess_None:
        return 0;
        break;

    case CurveSettings::eProcess_Univariate:
    case CurveSettings::eProcess_Depth:
    case CurveSettings::eProcess_Inclination:
    case CurveSettings::eProcess_Field:
    case CurveSettings::eProcess_Spherical:
        return 1;
        break;

    case CurveSettings::eProcess_Declination:
    case CurveSettings::eProcess_Vector:
    case CurveSettings::eProcess_Unknwon_Dec:
    case CurveSettings::eProcess_2D:
        return 2;
        break;

    case CurveSettings::eProcess_3D:
        return 3;
        break;
    }
    return -1;
}

void EventItem::paintBoxCurveParameter (QPainter* painter, const QRectF &rectBox, const CurveSettings &cs )
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

        QString text1, text2, text3;

        switch (cs.mProcessType) {
        case CurveSettings::eProcess_3D :
            text1 = cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble());
            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);

            text2 = cs.Y_short_name() + " = " + QLocale().toString(mData.value(STATE_EVENT_Y_DEC).toDouble());
            text2 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SY).toDouble());

            painter->drawText(QRectF(lineX, lineY + mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text2);

            text3 = cs.Z_short_name() + " = " + QLocale().toString(mData.value(STATE_EVENT_Z_F).toDouble());
            text3 += " ± " + QLocale().toString(mData.value(STATE_EVENT_SZ_SF).toDouble());
            painter->drawText(QRectF(lineX, lineY + 2*mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text3);
            break;

        case CurveSettings::eProcess_2D :
            text1 = cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble(), 'g', 3);
            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);

            text2 = cs.Y_short_name() + " = " + QLocale().toString(mData.value(STATE_EVENT_Y_DEC).toDouble());
            text2 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SY).toDouble());
            painter->drawText(QRectF(lineX, lineY + mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text2);
            break;

        case CurveSettings::eProcess_Spherical :
            text1 =  cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble(), 'g', 3);

            text1 += " "+ cs.Y_short_name() + " = ";
            text1 += QLocale().toString(mData.value(STATE_EVENT_Y_DEC).toDouble());

            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);
            break;
        case CurveSettings::eProcess_Unknwon_Dec :
            text1 =  cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble(), 'g', 3);
            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);

            text2 = cs.Y_short_name() + " = " + QLocale().toString(mData.value(STATE_EVENT_Z_F).toDouble());
            text2 += " ± " + QLocale().toString(mData.value(STATE_EVENT_SZ_SF).toDouble());
            painter->drawText(QRectF(lineX, lineY + mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text2);
            break;

        case CurveSettings::eProcess_Vector :
            text1 =  cs.X_short_name() + " = ";
            text1 += QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble(), 'g', 3);

            text1 += " "+ cs.Y_short_name()+ " = ";
            text1 += QLocale().toString(mData.value(STATE_EVENT_Y_DEC).toDouble());

            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);

            text2 = cs.Z_short_name() + " = ";
            text2 += QLocale().toString(mData.value(STATE_EVENT_Z_F).toDouble());
            text2 += " ± " + QLocale().toString(mData.value(STATE_EVENT_SZ_SF).toDouble());
            painter->drawText(QRectF(lineX, lineY + mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text2);
            break;

        case CurveSettings::eProcess_Univariate :
            text1 = cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble());
            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);
            break;

        case CurveSettings::eProcess_Depth :
            text1 = cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble());
            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);
            break;

        case CurveSettings::eProcess_Inclination :
            text1 = cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble());
            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);
            break;

        case CurveSettings::eProcess_Declination :
            text1 = cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble());

            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);

            text2 += QObject::tr("Inc") + " = "+ QLocale().toString (mData.value(STATE_EVENT_X_INC_DEPTH).toDouble()) + " ± " + QLocale().toString (mData.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble());

            painter->drawText(QRectF(lineX, lineY + mCurveLineHeight, lineW, mCurveLineHeight), Qt::AlignCenter, text2);
            break;

        case CurveSettings::eProcess_Field :
            text1 = cs.X_short_name() + " = " + QLocale().toString (mData.value(STATE_EVENT_Z_F).toDouble());
            text1 += " ± " + QLocale().toString (mData.value(STATE_EVENT_SZ_SF).toDouble());
            painter->drawText(QRectF(lineX, lineY, lineW, mCurveLineHeight), Qt::AlignCenter, text1);
            break;

        case CurveSettings::eProcess_None :
        default:
            break;
        }

    }


}

void EventItem::paintBoxPhases (QPainter *painter, const QRectF &rectBox)
{
    QFont font, memoFont;
    memoFont = painter->font();

    font.setStyle(QFont::StyleNormal);
    font.setBold(false);
    font.setItalic(false);
    painter->setFont(font);
    const QJsonArray phases = getPhases();
    const auto numPhases = phases.size();

    if (numPhases == 0) {
        const QColor eventColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
                                   mData.value(STATE_COLOR_GREEN).toInt(),
                                   mData.value(STATE_COLOR_BLUE).toInt());
        painter->setPen(getContrastedColor(eventColor));
        painter->drawText(rectBox, Qt::AlignCenter, tr("No Phase"));

    } else {
        const qreal w = (rectBox.width() - (numPhases-1)*mEltsMargin) / numPhases;
        qreal dx = 0;
        for (int i = 0; i < numPhases; ++i) {
            dx = i*(w + mEltsMargin);
            const QJsonObject phase = phases.at(i).toObject();
            painter->setPen(QColor(0, 0, 0));

            const QColor c(phase.value(STATE_COLOR_RED).toInt(),
                           phase.value(STATE_COLOR_GREEN).toInt(),
                           phase.value(STATE_COLOR_BLUE).toInt());

            painter->setBrush(c);
            painter->drawRoundedRect( (rectBox.x() + dx),  (rectBox.y()), (w),  (rectBox.height()), 4, 4);

        }
    }
   painter->setFont(memoFont);

}
