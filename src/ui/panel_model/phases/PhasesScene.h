#ifndef PhasesScene_H
#define PhasesScene_H

#include <QGraphicsScene>

class QGraphicsScene;
class QGraphicsItem;
class QLabel;

class Phase;
class PhaseConstraint;
class PhasesItem;
class PhasesSceneArrowItem;
class PhasesSceneArrowTmpItem;


class PhasesScene: public QGraphicsScene
{
    Q_OBJECT
public:
    PhasesScene(QGraphicsView* view, QObject* parent = 0);
    ~PhasesScene();
    
    void sendUpdateProject(const QString& reason, bool notify, bool async);
    
public slots:
    void updateProject();
    void updateSelection();

public:
    /*void phaseClicked(PhasesItem* phaseItem, QGraphicsSceneMouseEvent* e);
    void phaseDoubleClicked(PhasesItem* phaseItem, QGraphicsSceneMouseEvent* e);
    void phaseEntered(PhasesItem* phaseItem, QGraphicsSceneHoverEvent* e);
    void phaseLeaved(PhasesItem* phaseItem, QGraphicsSceneHoverEvent* e);
    void phaseMoved(PhasesItem* phaseItem, QGraphicsSceneMouseEvent* e);
    
    void constraintDoubleClicked(PhasesSceneArrowItem* item, QGraphicsSceneMouseEvent* e);
    void updateConstraintsPos();
    
public slots:
    void createPhase(Phase* phase);
    void deletePhase(Phase* phase);
    void updatePhase(Phase* phase);
    void setCurrentPhase(Phase* phase);
    
    void createPhaseConstraint(PhaseConstraint* constraint);
    void deletePhaseConstraint(PhaseConstraint* constraint);
    
    void updateCheckedPhases();
    
protected:
    void keyPressEvent(QKeyEvent* keyEvent);
    void keyReleaseEvent(QKeyEvent* keyEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);*/
    
    PhasesItem* currentItem();
    
private:
    QGraphicsView* mView;
    
    QList<PhasesItem*> mItems;
    QList<PhasesSceneArrowItem*> mConstraintItems;
    PhasesSceneArrowTmpItem* mTempArrow;
    
    QLabel* mHelpView;
    
    bool mDrawingArrow;
    bool mUpdatingItems;
};

#endif
