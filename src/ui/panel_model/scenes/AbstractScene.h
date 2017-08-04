#ifndef AbstractScene_H
#define AbstractScene_H

#include <QGraphicsScene>

class AbstractItem;
class ArrowItem;
class ArrowTmpItem;
class Project;


class AbstractScene: public QGraphicsScene
{
    Q_OBJECT
public:
    AbstractScene(QGraphicsView* view, QObject* parent = nullptr);
    ~AbstractScene();
    
    QRectF specialItemsBoundingRect(QRectF r = QRectF()) const;
    void adjustSceneRect();
    bool mDrawingArrow;
    ArrowTmpItem* mTempArrow;

    void setProject(Project *project);
    Project* getProject() const;

    QList<AbstractItem*> getItemsList() const  {return mItems;}
    bool showAllThumbs() const { return mShowAllThumbs;}

    bool mSelectKeyIsDown; // used to add item in selection
    bool mShowGrid;

public slots:
    void showGrid(bool show);

public:
    virtual void clean() = 0;
    
    virtual void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e) = 0;
    virtual void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e) = 0;
    
    virtual bool itemClicked(AbstractItem* eventItem, QGraphicsSceneMouseEvent* e);
    virtual void itemDoubleClicked(AbstractItem* eventItem, QGraphicsSceneMouseEvent* e);
    virtual void itemEntered(AbstractItem* eventItem, QGraphicsSceneHoverEvent* e);
    virtual void itemLeaved(AbstractItem* item, QGraphicsSceneHoverEvent* e);
    virtual void itemMoved(AbstractItem* item, QPointF newPos, bool merging);
    virtual void itemReleased(AbstractItem* item, QGraphicsSceneMouseEvent* e);
    
    virtual void sendUpdateProject(const QString& reason, bool notify, bool async) = 0;
    
    void updateConstraintsPos(AbstractItem* movedItem, const QPointF& newPos);

    //void setCurrentItem(QGraphicsItem *item);
    //void setCurrentItem(AbstractItem *item) { mCurrentItem = item;}
    
protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    
    virtual void keyPressEvent(QKeyEvent* keyEvent);
    virtual void keyReleaseEvent(QKeyEvent* keyEvent);
    
    virtual AbstractItem* currentItem() = 0;
    virtual AbstractItem* collidingItem(const QGraphicsItem* item) = 0;
    virtual void deleteSelectedItems() = 0;
    
    virtual bool constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo) = 0;
    virtual void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo) = 0;
    virtual void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo) = 0;
    
    void drawBackground(QPainter* painter, const QRectF& rect);

protected:
    Project* mProject;
    QGraphicsView* mView;
    QList<AbstractItem*> mItems;
    QList<ArrowItem*> mConstraintItems;
    
    bool mUpdatingItems;
    bool mAltIsDown;

    


    bool mShowAllThumbs;
    
    double mZoom;
    
    AbstractItem* mCurrentItem;

signals:
    void projectUpdated();

};

#endif
