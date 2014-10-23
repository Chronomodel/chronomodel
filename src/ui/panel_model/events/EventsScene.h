#ifndef EventsScene_H
#define EventsScene_H

#include <QGraphicsScene>

class QGraphicsScene;
class QGraphicsItem;
class QLabel;
class QGraphicsItemAnimation;
class QTimeLine;

class Event;
class EventConstraint;
class EventItem;
class EventsSceneArrowItem;
class EventsSceneArrowTmpItem;
class DateItem;
class Date;
class HelpWidget;


class EventsScene: public QGraphicsScene
{
    Q_OBJECT
public:
    EventsScene(QGraphicsView* view, QObject* parent = 0);
    ~EventsScene();
    
    void sendUpdateProject(const QString& reason, bool notify, bool async);
    
    HelpWidget* getHelpView();
    
public slots:
    void updateProject();
    void updateHelp();

public:
    void eventClicked(EventItem* eventItem, QGraphicsSceneMouseEvent* e);
    void eventDoubleClicked(EventItem* eventItem, QGraphicsSceneMouseEvent* e);
    void eventEntered(EventItem* eventItem, QGraphicsSceneHoverEvent* e);
    void eventLeaved(EventItem* eventItem, QGraphicsSceneHoverEvent* e);
    void eventMoved(EventItem* eventItem, QGraphicsSceneMouseEvent* e);
    void eventReleased(EventItem* eventItem, QGraphicsSceneMouseEvent* e);
    
    void dateMoved(DateItem* dateItem, QGraphicsSceneMouseEvent* e);
    void dateReleased(DateItem* dateItem, QGraphicsSceneMouseEvent* e);
    
    void constraintDoubleClicked(EventsSceneArrowItem* item, QGraphicsSceneMouseEvent* e);
    void updateConstraintsPos();
    
    QList<Date> decodeDataDrop(QGraphicsSceneDragDropEvent* e);
    
public slots:
    void createEventConstraint(EventConstraint* constraint);
    void deleteEventConstraint(EventConstraint* constraint);
    
protected:
    void keyPressEvent(QKeyEvent* keyEvent);
    void keyReleaseEvent(QKeyEvent* keyEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    void dropEvent(QGraphicsSceneDragDropEvent* e);
    void dragMoveEvent(QGraphicsSceneDragDropEvent* e);
    
    EventItem* collidingItem(QGraphicsItem* item);
    EventItem* currentItem();
    
private slots:
    void updateSelection();
    
signals:
    void eventDoubleClicked(Event* event);
    void csvDataLineDropAccepted(QList<int> rows);
    
private:
    QGraphicsView* mView;
    
    HelpWidget* mHelpView;
    QTimer* mHelpTimer;
    
    QList<EventItem*> mItems;
    QList<EventsSceneArrowItem*> mConstraintItems;
    EventsSceneArrowTmpItem* mTempArrow;
    
    bool mDrawingArrow;
    bool mUpdatingItems;
    
    QGraphicsItemAnimation* mDatesAnim;
    QTimeLine* mDatesAnimTimer;
};

#endif
