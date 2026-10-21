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

#ifndef EVENTSSCENE_H
#define EVENTSSCENE_H

#include "AbstractScene.h"
#include "EventItem.h"
#include "Date.h"
#include "StudyPeriodSettings.h"
#include "CurveSettings.h"

class QGraphicsItemAnimation;
class QTimeLine;

class DateItem;

class HelpWidget;


class EventsScene: public AbstractScene
{
    Q_OBJECT
public:
    EventsScene(QGraphicsView* view, QObject* parent = nullptr);
    virtual ~EventsScene();

    //void sendUpdateProject(const QString& reason, bool notify, bool async);
    void sendUpdateProject(const Project::ReasonId id, bool notify, bool async) override;

    HelpWidget* getHelpView();
    void showHelp(bool show);

    EventItem* currentEvent() const;

public slots:
    void clean() override;
    void phasesSelected();

    void createSceneFromState();
    void updateSceneFromState();

    void updateStateSelectionFromItem();
    void updateHelp();

    void deleteSelectedItems() override;

public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e) override;
    bool itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e) override;

    void itemEntered(AbstractItem* eventItem, QGraphicsSceneHoverEvent* e) override;
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e) override;
    void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e) override;

    void dateMoved(const DateItem* dateItem);
    EventItem *dateReleased(DateItem* dateItem);

    QList<Date> decodeDataDrop_old(QGraphicsSceneDragDropEvent* e);
    QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>> decodeDataDrop(QGraphicsSceneDragDropEvent* e); // = Qlist<QPair<Event'name, data>>

    void adaptItemsForZoom(const double prop);

    void centerOnEvent(int eventId);

    void noHide();
    void setShowAllThumbs(const bool show);


protected:
    void keyPressEvent(QKeyEvent* keyEvent) override;
    void keyReleaseEvent(QKeyEvent* keyEvent) override;
    void dropEvent(QGraphicsSceneDragDropEvent* e) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* e) override;

    AbstractItem* collidingItem(const QGraphicsItem *item) override;
    AbstractItem* currentItem() override;

    void setCurrentItem(QGraphicsItem* item);

    bool constraintAllowed(AbstractItem *itemFrom, AbstractItem *itemTo) override;
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo) override;
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo) override;
    EventItem* findEventItemWithJsonId(const int id);

signals:
    void csvDataLineDropAccepted(QList<int> rows);
    void csvDataLineDropRejected(QList<int> rows);
    void eventClicked();
    void eventDoubleClicked(AbstractItem* item);
    void noSelection();
    void eventsAreSelected();
    void eventsAreModified(const Project::ReasonId id, bool notify, bool storeUndoCommand);

private:
    HelpWidget* mHelpView;
    QTimer* mHelpTimer;
    
    StudyPeriodSettings mSettings;
    CurveSettings mCurveSettings;

};

#endif
