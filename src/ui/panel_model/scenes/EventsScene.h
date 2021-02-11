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

#ifndef EVENTSSCENE_H
#define EVENTSSCENE_H

#include "AbstractScene.h"
#include "EventItem.h"
#include "ProjectSettings.h"
#include "ChronocurveSettings.h"

#include <QWheelEvent>

class QGraphicsItemAnimation;
class QTimeLine;

class DateItem;
class Date;
class HelpWidget;


class EventsScene: public AbstractScene
{
    Q_OBJECT
public:
    EventsScene(QGraphicsView* view, QObject* parent = nullptr);
    virtual ~EventsScene();

    void sendUpdateProject(const QString& reason, bool notify, bool async);

    HelpWidget* getHelpView();
    void showHelp(bool show);

    EventItem* currentEvent() const;

public slots:
    void clean();
    void phasesSelected();

    void createSceneFromState();
    void updateSceneFromState();
    //void updateSelection(bool sendNotification = true, bool force = false);
    void updateStateSelectionFromItem();
    void updateHelp();

   virtual  void deleteSelectedItems();
    //void updateSelectedEventsFromPhases();
    //void updateGreyedOutEvents(const QMap<int, bool>& eyedPhases);

public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    bool itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);

    virtual void itemEntered(AbstractItem* eventItem, QGraphicsSceneHoverEvent* e);
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);

    void dateMoved(const DateItem* dateItem);
    EventItem *dateReleased(DateItem *dateItem);

    QList<Date> decodeDataDrop_old(QGraphicsSceneDragDropEvent* e);
    QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>> decodeDataDrop(QGraphicsSceneDragDropEvent* e); // = Qlist<QPair<Event'name, data>>

    void adaptItemsForZoom(const double prop);

    void centerOnEvent(int eventId);

    void noHide();
    void setShowAllThumbs(const bool show);
    //bool showAllThumbs() const { return mShowAllThumbs;}

protected:
    virtual void keyPressEvent(QKeyEvent* keyEvent);
    virtual void keyReleaseEvent(QKeyEvent* keyEvent);
    void dropEvent(QGraphicsSceneDragDropEvent* e);
    void dragMoveEvent(QGraphicsSceneDragDropEvent* e);

    AbstractItem* collidingItem(const QGraphicsItem *item);
    AbstractItem* currentItem() ;

    void setCurrentItem(QGraphicsItem* item);

    bool constraintAllowed(AbstractItem *itemFrom, AbstractItem *itemTo);
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo);
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo);
    EventItem *findEventItemWithJsonId(const int id);

signals:
    void csvDataLineDropAccepted(QList<int> rows);
    void csvDataLineDropRejected(QList<int> rows);
    void eventClicked();
    void eventDoubleClicked();
    void noSelection();
    void eventsAreSelected();
    void eventsAreModified(const QString& reason, bool notify, bool storeUndoCommand);

private:
    HelpWidget* mHelpView;
    QTimer* mHelpTimer;
    
    ProjectSettings mSettings;
    ChronocurveSettings mChronocurveSettings;

//    QGraphicsItemAnimation* mDatesAnim;
//    QTimeLine* mDatesAnimTimer;
    //bool mShowAllThumbs;

};

#endif
