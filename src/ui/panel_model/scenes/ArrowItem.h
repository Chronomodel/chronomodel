/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#ifndef ARROWITEM_H
#define ARROWITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QJsonObject>

class AbstractScene;
class AbstractItem;
class EventItem;
class PhaseItem;

class ArrowItem: public QGraphicsItem
{

    Q_PROPERTY(QJsonObject mData READ data WRITE setData)
public:
    enum Type {
        eEvent = 0,
        ePhase = 1
    };

    ArrowItem(AbstractScene* scene, Type type, const QJsonObject& constraint, QGraphicsItem* parent = nullptr);
    virtual ~ArrowItem();

    void updatePosition();
    void setFrom(const double x, const double y);
    void setTo(const double x, const double y);

    void setGreyedOut(bool greyedOut);

    QRectF boundingRect() const;
    QPainterPath shape() const;

    QPointF contactPos(const double theta, AbstractItem *e);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);

    QRectF getBubbleRect(const QString& text = QString()) const;
    QSizeF getBubbleSize(const QString& text = QString()) const;
    QString getBubbleText() const;

    void setData(const QJsonObject& c);
    QJsonObject& data();

    EventItem* findEventItemWithJsonId(const int id);
    PhaseItem* findPhaseItemWithJsonId(const int id);

public:
    Type mType;
    QJsonObject mData;
    AbstractScene* mScene;

    QPointF mStart;
    QPointF mEnd;

    QPointF mStartContact;
    QPointF mEndContact;

    qreal mBubbleWidth;
    qreal mBubbleHeight;

    bool mEditing;
    bool mShowDelete;

    bool mGreyedOut;
};

#endif
