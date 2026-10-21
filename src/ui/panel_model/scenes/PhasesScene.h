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

#ifndef PHASESSCENE_H
#define PHASESSCENE_H

#include "AbstractScene.h"

class PhaseItem;

class PhasesScene: public AbstractScene
{
    Q_OBJECT
public:
    PhasesScene(QGraphicsView* view, QObject* parent = nullptr);
    virtual ~PhasesScene();

    //void sendUpdateProject(const QString& reason, bool notify, bool async);
    void sendUpdateProject(const Project::ReasonId id, bool notify, bool async) override;

    PhaseItem* currentPhase() const;
     void createSceneFromState();

signals:
    void selectionUpdated(); // connected to evenScene to hide or show the Events' phase
    void noSelection();
    void phasesAreSelected();

public slots:
    void clean() override;
    void updateSceneFromState();
    void updateStateSelectionFromItem();

    void noHide();
    void eventsSelected();

   void deleteSelectedItems() override;

public:
    bool itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e) override;

    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e) override;

    void itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent*) override;

    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent*) override;
    void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent*) override;


    void adaptItemsForZoom(const double prop);

    void setShowAllEvents(const bool show);


protected:
    AbstractItem* collidingItem(const QGraphicsItem* item) override;
    AbstractItem* currentItem() override;
    void setCurrentItem(QGraphicsItem* item);


    bool constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo) override;
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo) override;
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo) override;


};

#endif
