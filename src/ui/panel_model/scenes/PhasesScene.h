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
    
    void updateCheckedPhases();
    
public slots:
    void updateProject();
    void updateSelection();
    
public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    
protected:
    AbstractItem* collidingItem(QGraphicsItem* item);
    AbstractItem* currentItem();
    
    void deleteSelectedItems();
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo);
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo);
};

#endif
