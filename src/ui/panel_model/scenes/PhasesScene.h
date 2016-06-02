#ifndef PhasesScene_H
#define PhasesScene_H

#include "AbstractScene.h"

class PhaseItem;


class PhasesScene: public AbstractScene
{
    Q_OBJECT
public:
    PhasesScene(QGraphicsView* view, QObject* parent = 0);
    ~PhasesScene();
    
    void sendUpdateProject(const QString& reason, bool notify, bool async);
    PhaseItem* currentPhase() const;
signals:
    void selectionChanged();
    
public slots:
    void clean();
    void updateScene();
    void updateSelection(bool sendNotif = true, bool forced = false);
    void updateCheckedPhases();

    void noHide();
    void eventsSelected();
    
public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    bool itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);

    void itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent* e);
    void updateEyedPhases();
    
    void adaptItemsForZoom(double prop);

    void setShowAllEvents(const bool show) { mShowAllEvents = show;}
    bool showAllEvents() const { return mShowAllEvents;}
    
protected:
    AbstractItem* collidingItem(QGraphicsItem* item);
    AbstractItem* currentItem();
    void setCurrentItem(QGraphicsItem* item);

    void deleteSelectedItems();
    bool constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo);
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo);
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo);

private:
    bool mShowAllEvents;

};

#endif
