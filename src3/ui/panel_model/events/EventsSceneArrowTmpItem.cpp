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

#include "EventsSceneArrowTmpItem.h"
#include <QtWidgets>


EventsSceneArrowTmpItem::EventsSceneArrowTmpItem(EventsScene* EventsScene, QGraphicsItem* parent):QGraphicsItem(parent),
mEventsScene(EventsScene),
mXFrom(0),
mYFrom(0),
mXTo(0),
mYTo(0),
mState(eNormal),
mLocked(false)
{
    setZValue(-1.);
}

void EventsSceneArrowTmpItem::setFrom(double x, double y)
{
    mXFrom = x;
    mYFrom = y;
    if(scene())
    {
        scene()->update();
    }
}

void EventsSceneArrowTmpItem::setTo(double x, double y)
{
    if(!mLocked)
    {
        mXTo = x;
        mYTo = y;
        if(scene())
        {
            scene()->update();
        }
    }
}

void EventsSceneArrowTmpItem::setState(const State state)
{
    mState = state;
    update();
}

void EventsSceneArrowTmpItem::setLocked(bool locked)
{
    mLocked = locked;
}

QRectF EventsSceneArrowTmpItem::boundingRect() const
{
    double x = qMin(mXFrom, mXTo);
    double y = qMin(mYFrom, mYTo);
    double w = qAbs(mXTo - mXFrom);
    double h = qAbs(mYTo - mYFrom);

    return QRectF(x, y, w, h);
}

void EventsSceneArrowTmpItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRectF rect = boundingRect();

    //painter->fillRect(rect, QColor(255, 0, 0, 30));

    painter->setRenderHint(QPainter::Antialiasing);
    int penWidth = 2;
    QColor color = Qt::black;

    switch(mState)
    {
        case eNormal:
            color = Qt::black;
            break;
        case eAllowed:
            color = QColor(77, 180, 62);
            break;
        case eForbidden:
            color = Qt::red;
            break;
        default:
            break;
    }
    painter->setPen(QPen(color, penWidth, Qt::DashLine));
    painter->drawLine(mXFrom, mYFrom, mXTo, mYTo);

    // arrows

    float angle_rad = atanf(rect.width() / rect.height());
    float angle_deg = angle_rad * 180. / M_PI;

    QPainterPath path;
    int arrow_w = 10;
    int arrow_l = 15;
    path.moveTo(-arrow_w/2, arrow_l/2);
    path.lineTo(arrow_w/2, arrow_l/2);
    path.lineTo(0, -arrow_l/2);
    path.closeSubpath();

    float posX = rect.width()/2;
    float posY = rect.height()/2;

    if(mXFrom < mXTo && mYFrom > mYTo)
    {
        painter->save();
        painter->translate(rect.x() + posX, rect.y() + posY);
        painter->rotate(angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    else if(mXFrom < mXTo && mYFrom < mYTo)
    {
        painter->save();
        painter->translate(rect.x() + posX, rect.y() + posY);
        painter->rotate(180 - angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    else if(mXFrom > mXTo && mYFrom < mYTo)
    {
        painter->save();
        painter->translate(rect.x() + posX, rect.y() + posY);
        painter->rotate(180 + angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    else if(mXFrom > mXTo && mYFrom > mYTo)
    {
        painter->save();
        painter->translate(rect.x() + rect.width()/2, rect.y() + rect.height()/2);
        painter->rotate(-angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }

    // Message

    switch(mState)
    {
        case eAllowed:
        case eForbidden:
        {
            float w = 40;
            float h = 40;
            QRectF r(rect.x() + (rect.width() - w)/2, rect.y() + (rect.height() - h)/2, w, h);
            painter->setBrush(Qt::white);
            painter->drawEllipse(r);

            if(mState == eAllowed)
            {
                painter->drawText(r, Qt::AlignCenter, "OK");
            }
            else
            {
                painter->setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap));
                painter->drawLine(r.x() + r.width()/4, r.y() + r.height()/4, r.x() + 3*r.width()/4, r.y() + 3*r.height()/4);
                painter->drawLine(r.x() + r.width()/4, r.y() + 3*r.height()/4, r.x() + 3*r.width()/4, r.y() + r.height()/4);
            }
        }
        default:
            break;
    }
}
