#ifndef EventsScene_H
#define EventsScene_H

#include "AbstractScene.h"
#include "EventItem.h"
#include "ProjectSettings.h"
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
    QList<QPair<QString, Date>> decodeDataDrop(QGraphicsSceneDragDropEvent* e); // = Qlist<QPair<Event'name, data>>
    
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
    
//    QGraphicsItemAnimation* mDatesAnim;
//    QTimeLine* mDatesAnimTimer;
    //bool mShowAllThumbs;

};

#endif
