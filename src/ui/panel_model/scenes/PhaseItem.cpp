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

#include "PhaseItem.h"

#include "PhasesScene.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include "Project.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"

#include <QtWidgets>
#include <QLocale>


int PhaseItem::mTitleHeight(25);
int PhaseItem::mEltsHeight(25);

PhaseItem::PhaseItem(AbstractScene* scene, const QJsonObject& phase, QGraphicsItem* parent):
    AbstractItem(scene, parent),
    mControlsVisible(false),
    mControlsEnabled(false),
    matLeastOneEventSelected(false),
    mOneEventSelectedOnScene(false)
{

    inPix = new QPixmap(":insert_event.png");
    exPix = new QPixmap(":extract_event.png");



    setPhase(phase);
}

PhaseItem::~PhaseItem()
{
    delete inPix;
    inPix = nullptr;
    
    delete exPix;
    exPix = nullptr;
}



void PhaseItem::setPhase(const QJsonObject& phase)
{
    prepareGeometryChange();
    mData = phase;

    setSelected(mData.value(STATE_IS_SELECTED).toBool());// || mData.value(STATE_IS_CURRENT).toBool() );
    setPos(mData.value(STATE_ITEM_X).toDouble(), mData.value(STATE_ITEM_Y).toDouble());

    // ----------------------------------------------------
    //  Calculate item size
    // ----------------------------------------------------
    const qreal w = AbstractItem::mItemWidth;
    qreal h = mTitleHeight + 2*mBorderWidth + 2*AbstractItem::mEltsMargin;

    const QJsonArray events = getEvents();
    if (events.size() > 0)
        h += events.size() * (mEltsHeight + AbstractItem::mEltsMargin);


    const QString tauStr = getTauString();
    if (!tauStr.isEmpty())
        h += mEltsMargin + mEltsHeight;

    mSize = QSizeF(w, h);

    update();
}


void PhaseItem::setControlsEnabled(const bool enabled)
{
    mControlsEnabled = enabled;
}

void PhaseItem::setControlsVisible(const bool visible)
{
    mControlsVisible = visible;
}

// Mouse events

void PhaseItem::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
    setControlsEnabled(true);
    AbstractItem::hoverEnterEvent(e);
}

void PhaseItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    setControlsEnabled(false);
    AbstractItem::hoverLeaveEvent(e);
}

void PhaseItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    // -------------------------------------------------
    // 1️⃣ Gestion des contrôles visibles (insert / extract)
    // -------------------------------------------------
    if (mControlsVisible) {
        const QPointF p = e->pos();

        if (insertRect().contains(p)) {
            e->accept();
            getProject_ptr()->updatePhaseEvents(
                mData.value(STATE_ID).toInt(),
                Project::InsertEventsToPhase);
            return;
        }

        if (matLeastOneEventSelected && extractRect().contains(p)) {
            e->accept();
            getProject_ptr()->updatePhaseEvents(
                mData.value(STATE_ID).toInt(),
                Project::ExtractEventsFromPhase);
            return;
        }
    }

    // -------------------------------------------------
    // 2️⃣ Cast sécurisé vers la scène spécifique
    // -------------------------------------------------
    PhasesScene* const itemScene = qobject_cast<PhasesScene*>(mScene);
    if (!itemScene) {
        // Pas la bonne scène → on laisse la classe de base gérer l’événement
        QGraphicsObject::mousePressEvent(e);
        return;
    }

    // -------------------------------------------------
    // 3️⃣ Interaction avec la scène (sélection / flèche)
    // -------------------------------------------------
    if (itemScene->selectedItems().size() < 2) {
        if (!itemScene->itemClicked(this, e)) {
            setZValue(2.0);                 // mettre l’item au premier plan
        } else {
            // on prépare la flèche temporaire
            itemScene->mTempArrow->setFrom(pos().x(), pos().y());
        }
    }

    // -------------------------------------------------
    // 4️⃣ Propagation de l’événement à la hiérarchie Qt
    // -------------------------------------------------
    QGraphicsObject::mousePressEvent(e);   // appel unique à la classe de base
}


void PhaseItem::redrawPhase()
{
    update();
}

bool sortEvents(QPair<int, int> e1, QPair<int, int> e2)
{
    return (e1.second < e2.second);
}

void PhaseItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* , QWidget* )
{
    // QPainter::Antialiasing is effective on native shape = circle, square, line

    painter->setRenderHints(painter->renderHints() | QPainter::SmoothPixmapTransform | QPainter::Antialiasing );

    const QRectF rect = rectF();//boundingRect();
    int rounded = 10;

    const QColor phaseColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
                               mData.value(STATE_COLOR_GREEN).toInt(),
                               mData.value(STATE_COLOR_BLUE).toInt());
    const QColor fontColor = getContrastedColor(phaseColor);

    QFont font (qApp->font().family(), 14);
    //font.setPixelSize(14);

    // Draw then container
    painter->setPen(Qt::NoPen);
    painter->setBrush(phaseColor);
    painter->drawRoundedRect(rect, rounded, rounded);

    //
    // Events
    QRectF r(rect.x() + mBorderWidth + AbstractItem::mEltsMargin,
             rect.y() + mBorderWidth + AbstractItem::mEltsMargin + mTitleHeight + AbstractItem::mEltsMargin,
             rect.width() - 2 * (AbstractItem::mEltsMargin + mBorderWidth),
             mEltsHeight);

    const QJsonArray events = getEvents();
    double dy = AbstractItem::mEltsMargin + mEltsHeight;

    const bool showAlldata = mScene->showAllThumbs();
    matLeastOneEventSelected = false;
    mOneEventSelectedOnScene = false;
    
    const QJsonObject &state = MainWindow::getInstance()->getProject()->state();
    const QJsonArray &allEvents = state.value(STATE_EVENTS).toArray();

    for (auto ev :allEvents) {
       const QJsonObject &event = ev.toObject();
        const bool isSelected = event.value(STATE_IS_SELECTED).toBool();//( event.value(STATE_IS_SELECTED).toBool() || event.value(STATE_IS_CURRENT).toBool() );
       mOneEventSelectedOnScene = (mOneEventSelectedOnScene || isSelected);
    }
    
    painter->setFont(font);

    QList< QPair<int, double>> sortedEvents;
    for (int i = 0; i < events.size(); ++i){
        sortedEvents.append(qMakePair(i, events.at(i).toObject().value(STATE_ITEM_Y).toDouble()));
    }
    std::sort(sortedEvents.begin(), sortedEvents.end(), sortEvents);


   for (int j = 0; j < sortedEvents.size(); ++j) {
       int i = sortedEvents[j].first;
       const QJsonObject &event = events.at(i).toObject();
       const QColor eventColor(event.value(STATE_COLOR_RED).toInt(),
                               event.value(STATE_COLOR_GREEN).toInt(),
                               event.value(STATE_COLOR_BLUE).toInt());

       const bool isSelected = event.value(STATE_IS_SELECTED).toBool();//( event.value(STATE_IS_SELECTED).toBool() || event.value(STATE_IS_CURRENT).toBool() );
       const bool isBound = event.value(STATE_EVENT_TYPE).toInt() == Event::eBound;
       if (j > 0)
           r.adjust(0, dy, 0, dy);

       painter->setPen(getContrastedColor(eventColor));

       painter->setFont(font);

       painter->save();

       painter->setBrush(eventColor);
       if (isSelected)
           matLeastOneEventSelected = true;

       painter->setPen(QPen(getContrastedColor(eventColor), 1));

       if (!isSelected & !showAlldata) {
           painter->setOpacity(0.2);
       }

       if (isBound)
           painter->drawRoundedRect(r, r.width()/7, r.height()/2);
       else
           painter->drawRoundedRect(r, 1, 1);

       painter->setPen(QPen(getContrastedColor(eventColor), 1));

       if (mControlsVisible) {
           const QFontMetrics fmAdjust (font);
           const QString eventName = fmAdjust.elidedText(event.value(STATE_NAME).toString(), Qt::ElideRight, int (r.width() - 5));
           painter->drawText(r, Qt::AlignCenter, eventName);
       }

       painter->restore();
    }


    if (mControlsVisible && mControlsEnabled) {
        // insert button
        const QRectF inRect = insertRect();
        // extract button
        const QRectF exRect = extractRect();

        // [4] l'ancien code (QTransform::fromScale est statique, résultat ignoré) faisait deux
        // transformed() à l'identité à chaque repaint ; drawPixmap(rect, pix, pix.rect())
        // met déjà à l'échelle, avec SmoothPixmapTransform activé plus haut.
        const QPixmap& inPix2 = *inPix;
        const QPixmap& exPix2 = *exPix;
        painter->setOpacity(1);
        painter->fillRect(inRect, Qt::black);
        painter->fillRect(exRect, Qt::black);
        
        painter->drawRect(inRect);
        painter->drawRect(exRect);
        
        if (mOneEventSelectedOnScene) {
            painter->setOpacity(1);
            painter->fillRect(inRect, Qt::black);
            painter->drawPixmap(inRect, inPix2, inPix2.rect());
        } else {
            painter->setOpacity(0.5);
            painter->fillRect(inRect, Qt::black);
            painter->drawPixmap(inRect, inPix2, inPix2.rect());
        }
     
        
        if (matLeastOneEventSelected){
            painter->setOpacity(1);
            painter->fillRect(exRect, Qt::black);
            painter->drawPixmap(exRect, exPix2, exPix2.rect());
          
        } else {
            painter->setOpacity(0.5);
            painter->fillRect(exRect, Qt::black);
            painter->drawPixmap(exRect, exPix2, exPix2.rect());
        
        }
      

    } else {
        // Phase Name
        const QRectF tr(rect.x() + mBorderWidth + mEltsMargin,
                  rect.y() + mBorderWidth,
                  rect.width() - 2*(mBorderWidth+ mEltsMargin),
                  mTitleHeight);

        font.setPixelSize(16);
          font.setBold(true);
        painter->setFont(font);

        QString name = mData.value(STATE_NAME).toString();

        QFontMetrics fmName (font);

        name = fmName.elidedText(name, Qt::ElideRight, int(tr.width() - 5));
        painter->setPen(fontColor);
        painter->drawText(tr, Qt::AlignCenter, name);
    }

    // Change font
    font.setPixelSize(12);
    painter->setFont(font);

    // Type (duration tau)
    const QString tauStr = getTauString();
    if (!tauStr.isEmpty()) {
        const QRectF tpr(rect.x() + mBorderWidth + mEltsMargin,
                   rect.y() + rect.height() - mBorderWidth - mEltsHeight - mEltsMargin,
                   rect.width() - 2*(mBorderWidth + mEltsMargin),
                   mEltsHeight);

        painter->setPen(Qt::black);
        painter->setBrush(Qt::white);
        painter->drawRect(tpr);

        painter->drawText(tpr, Qt::AlignCenter, tauStr);
    }


    // Border
    painter->setBrush(Qt::NoBrush);
    if (isSelected()) {
        painter->setPen(QPen(Qt::white, 5.));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), rounded, rounded);

        painter->setPen(QPen(Qt::red, 3.));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), rounded, rounded);
    }
}

QJsonArray PhaseItem::getEvents() const
{
    const QString phaseId = QString::number(mData.value(STATE_ID).toInt());
    const QJsonObject &state = MainWindow::getInstance()->getState();
    const QJsonArray &allEvents = state.value(STATE_EVENTS).toArray();
    QJsonArray events;
    for (const auto ev : allEvents) {
        const QJsonObject& event = ev.toObject();
        const QString phasesIdsStr = event.value(STATE_EVENT_PHASE_IDS).toString();
        QStringList phasesIds = phasesIdsStr.split(",");
        if (phasesIds.contains(phaseId))
            events.append(event);
    }
    return events;
}

QString PhaseItem::getTauString() const
{
    QString tauStr;
    const Phase::TauType type = Phase::TauType (mData.value(STATE_PHASE_TAU_TYPE).toInt());
    if (type == Phase::eTauFixed)
        tauStr += tr("Duration ≤ %1").arg(QLocale().toString(mData.value(STATE_PHASE_TAU_FIXED).toDouble()));

    else if (type == Phase::eZOnly)
        tauStr += tr("Uniform Span");

    return tauStr;
}

QRectF PhaseItem::checkRect() const
{
   return QRectF (rectF().x() + mBorderWidth + mEltsMargin,
                 rectF().y() + mBorderWidth + mEltsMargin,
                 mTitleHeight,
                 mTitleHeight);

}

QRectF PhaseItem::extractRect() const
{
    return QRectF (rectF().x() + mBorderWidth + mEltsMargin + mEltsMargin + mTitleHeight,
                  rectF().y() + mBorderWidth + mEltsMargin,
                  mTitleHeight,
                  mTitleHeight);

}

QRectF PhaseItem::insertRect() const
{
    return QRectF (rectF().x() + mBorderWidth + mEltsMargin,
                  rectF().y() + mBorderWidth + mEltsMargin,
                  mTitleHeight,
                  mTitleHeight);

}
