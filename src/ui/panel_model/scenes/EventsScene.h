#ifndef EventsScene_H
#define EventsScene_H

#include "AbstractScene.h"

class QGraphicsItemAnimation;
class QTimeLine;

class DateItem;
class Date;
class HelpWidget;


class EventsScene: public AbstractScene
{
    Q_OBJECT
public:
    EventsScene(QGraphicsView* view, QObject* parent = 0);
    ~EventsScene();
    
    void sendUpdateProject(const QString& reason, bool notify, bool async);
    
    HelpWidget* getHelpView();
    void showHelp(bool show);
    
public slots:
    void updateProject();
    void updateSelection();
    void updateHelp();
    
    void updateSelectedEventsFromPhases();
    void updateGreyedOutEvents(const QMap<int, bool>& eyedPhases);

public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    
    void dateMoved(DateItem* dateItem, QGraphicsSceneMouseEvent* e);
    void dateReleased(DateItem* dateItem, QGraphicsSceneMouseEvent* e);
    
    QList<Date> decodeDataDrop(QGraphicsSceneDragDropEvent* e);
    
protected:
    void dropEvent(QGraphicsSceneDragDropEvent* e);
    void dragMoveEvent(QGraphicsSceneDragDropEvent* e);
    
    AbstractItem* collidingItem(QGraphicsItem* item);
    AbstractItem* currentItem();
    
    void deleteSelectedItems();
    void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo);
    void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo);
    
private slots:
    void setSelectedPhase(const QJsonObject& phase);
    
signals:
    void csvDataLineDropAccepted(QList<int> rows);
    void eventDoubleClicked();
    
private:
    HelpWidget* mHelpView;
    QTimer* mHelpTimer;
    
    QGraphicsItemAnimation* mDatesAnim;
    QTimeLine* mDatesAnimTimer;
};

#endif
