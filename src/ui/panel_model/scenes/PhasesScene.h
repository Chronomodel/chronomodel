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

signals:
    void selectionChanged();
    
public slots:
    void clean();
    void updateProject();
    void updateSelection(bool sendNotif = true, bool forced = false);
    void updateCheckedPhases();
    
public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    
    void updateEyedPhases();
    
    void adaptItemsForZoom(double prop);
    
protected:
    AbstractItem* collidingItem(QGraphicsItem* item);
    AbstractItem* currentItem();
    void setCurrentItem(QGraphicsItem* item);

    void deleteSelectedItems();
    bool constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo);
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo);
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo);
 private:
    Project* mProject;
};

#endif
