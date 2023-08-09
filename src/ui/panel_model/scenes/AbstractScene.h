/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#ifndef ABSTRACTSCENE_H
#define ABSTRACTSCENE_H

#include <QGraphicsScene>

class AbstractItem;
class ArrowItem;
class ArrowTmpItem;
class Project;


class AbstractScene: public QGraphicsScene
{
    Q_OBJECT
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
    qreal mDeltaGrid;

public:
    bool mDrawingArrow;
    ArrowTmpItem* mTempArrow;
    bool mSelectKeyIsDown; // used to add item in selection
    bool mShowGrid;

    AbstractScene(QGraphicsView* view, QObject* parent = nullptr);
    virtual ~AbstractScene();

    QRectF specialItemsBoundingRect(QRectF r = QRectF()) const;
    void adjustSceneRect();

    void setProject(Project *project);
    Project* getProject() const;

    QList<AbstractItem*> getItemsList() const  {return mItems;}
    bool showAllThumbs() const { return mShowAllThumbs;}


    qreal deltaGrid() const {return mDeltaGrid;}

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



signals:
    void projectUpdated();

};

#endif
