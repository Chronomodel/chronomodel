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

    void sendUpdateProject(const QString& reason, bool notify, bool async);
    PhaseItem* currentPhase() const;
     void createSceneFromState();

signals:
    void selectionUpdated(); // connected to evenScene to hide or show the Events' phase
    void noSelection();
    void phasesAreSelected();

public slots:
    void clean();
    void updateSceneFromState();
    void updateStateSelectionFromItem();

    void noHide();
    void eventsSelected();

   virtual  void deleteSelectedItems();

public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent*);
    void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent*);
    bool itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent*);

    void itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent*);
    //void updateEyedPhases();

    void adaptItemsForZoom(const double prop);

    void setShowAllEvents(const bool show);
   // bool showAllEvents() const { return mShowAllEvents;}

protected:
    AbstractItem* collidingItem(const QGraphicsItem* item);
    AbstractItem* currentItem();
    void setCurrentItem(QGraphicsItem* item);


    bool constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo);
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo);
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo);

private:
   // bool mShowAllEvents;

};

#endif
