/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#ifndef ABSTRACTITEM_H
#define ABSTRACTITEM_H

#include "AbstractScene.h"

#include <QGraphicsObject>
#include <QJsonObject>

class AbstractItem : public QGraphicsObject
{
public:
    AbstractItem(AbstractScene* scene, QGraphicsItem* parent = nullptr);
    virtual ~AbstractItem();

    void setMergeable(bool mergeable, bool shouldRepaint = true);
    virtual void setGreyedOut(const bool greyedOut);

    virtual void updateItemPosition(const QPointF& pos);
    void setSelectedInData(const bool selected);
    void setCurrentInData(const bool current);
    QJsonObject& getData() {return mData;};

    static QFont adjustFont(const QFont &ft, const QString &str, const QRectF &r);

    virtual QSizeF sizeF() {return mSize;};
    virtual QRectF rectF() const;
    virtual QRectF boundingRect() const;

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    static int mBorderWidth;
    static int mEltsMargin;
    static int mItemWidth;
    static int mTitleHeight;
    QSizeF mSize;

    QJsonObject mData;
    AbstractScene* mScene;

    int mEltsHeight;

    bool mMoving; // used in AbstractScene::itemReleased() to merge item like eventItem and phaseItem
    bool mMergeable;
    bool mGreyedOut;

};

#endif
