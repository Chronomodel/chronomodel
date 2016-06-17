#ifndef EventsScene_H
#define EventsScene_H

#include "AbstractScene.h"
#include "EventItem.h"
#include "ProjectSettings.h"


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

    EventItem* currentEvent() const;

public slots:
    void clean();
    void phasesSelected();

    void createSceneFromState();
    void updateSceneFromState();
    //void updateSelection(bool sendNotification = true, bool force = false);
    void updateStateSelectionFromItem();
    void updateHelp();
    
    //void updateSelectedEventsFromPhases();
    //void updateGreyedOutEvents(const QMap<int, bool>& eyedPhases);

public:
    void itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    bool itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    
    virtual void itemEntered(AbstractItem* eventItem, QGraphicsSceneHoverEvent* e);
    void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e);
    
    void dateMoved(DateItem* dateItem, QGraphicsSceneMouseEvent* e);
    void dateReleased(DateItem* dateItem, QGraphicsSceneMouseEvent* e);
    
    QList<Date> decodeDataDrop(QGraphicsSceneDragDropEvent* e);
    
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
    
    AbstractItem* collidingItem(QGraphicsItem* item);
    AbstractItem* currentItem() ;

    void setCurrentItem(QGraphicsItem* item);
    
    void deleteSelectedItems();
    bool constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo);
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
    
private:
    HelpWidget* mHelpView;
    QTimer* mHelpTimer;
    ProjectSettings mSettings;
    
    QGraphicsItemAnimation* mDatesAnim;
    QTimeLine* mDatesAnimTimer;
    //bool mShowAllThumbs;

};

#endif
