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

#include "ArrowItem.h"
#include "EventItem.h"
#include "EventConstraint.h"
#include "MainWindow.h"
#include "Project.h"
#include "Painting.h"
#include <QtWidgets>
#include <math.h>


ArrowItem::ArrowItem(AbstractScene* scene, Type type, const QJsonObject& constraint, QGraphicsItem* parent):QGraphicsItem(parent),
mType(type),
mScene(scene),
mXStart(0),
mYStart(0),
mXEnd(0.),
mYEnd(0.),
mBubbleWidth(10.),
mBubbleHeight(10.),
mEditing(false),
mShowDelete(false),
mGreyedOut(false)
{
    setZValue(-1.);
    setAcceptHoverEvents(true);
    setFlags(QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsFocusable |
            QGraphicsItem::ItemSendsScenePositionChanges |
            QGraphicsItem::ItemSendsGeometryChanges);

    setData(constraint);
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(30);
    shadow->setOffset(3, 3);
    setGraphicsEffect(shadow);
}

ArrowItem::~ArrowItem()
{
    mScene = nullptr;
}

QJsonObject& ArrowItem::data()
{
    return mData;
}

void ArrowItem::setData(const QJsonObject& c)
{
    mData = c;
    updatePosition();
}

void ArrowItem::setFrom(const double x, const double y)
{
    prepareGeometryChange();
    mXStart = x;
    mYStart = y;
}

void ArrowItem::setTo(const double x, const double y)
{
    prepareGeometryChange();
    mXEnd = x;
    mYEnd = y;
}

void ArrowItem::setGreyedOut(bool greyedOut)
{
    mGreyedOut = greyedOut;
    update();
}

void ArrowItem::updatePosition()
{
    prepareGeometryChange();

    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();

    const int fromId = mData.value(STATE_CONSTRAINT_BWD_ID).toInt();
    const int toId = mData.value(STATE_CONSTRAINT_FWD_ID).toInt();

    QJsonObject from;
    QJsonObject to;

    if (mType == eEvent) {
        const QJsonArray events = state.value(STATE_EVENTS).toArray();
        for (int i=0; i<events.size(); ++i) {
            const QJsonObject event = events.at(i).toObject();
            if (event.value(STATE_ID).toInt() == fromId)
                from = event;
            if (event.value(STATE_ID).toInt() == toId)
                to = event;
        }
    } else {
        const QJsonArray phases = state.value(STATE_PHASES).toArray();
        for (int i=0; i<phases.size(); ++i) {
            const QJsonObject phase = phases.at(i).toObject();
            if (phase.value(STATE_ID).toInt() == fromId)
                from = phase;
            if (phase.value(STATE_ID).toInt() == toId)
                to = phase;
        }
    }

    mXStart = from.value(STATE_ITEM_X).toDouble();
    mYStart = from.value(STATE_ITEM_Y).toDouble();

    mXEnd = to.value(STATE_ITEM_X).toDouble();
    mYEnd = to.value(STATE_ITEM_Y).toDouble();
}

void ArrowItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    //qDebug()<<"ArrowItem::mouseDoubleClickEvent";
    QGraphicsItem::mouseDoubleClickEvent(e);
    mScene->constraintDoubleClicked(this, e);
}

void ArrowItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
   //  qDebug()<<"ArrowItem::mousePressEvent";
    QGraphicsItem::mousePressEvent(e);
    const QRectF r = getBubbleRect(getBubbleText());
    if (r.contains(e->pos()))
        mScene->constraintClicked(this, e);

}

void ArrowItem::hoverMoveEvent(QGraphicsSceneHoverEvent* e)
{
    //qDebug()<<"ArrowItem::hoverMoveEvent----->";
    QGraphicsItem::hoverMoveEvent(e);
    prepareGeometryChange();

    const QRectF br = boundingRect();

    const bool shouldShowDelete = br.contains(e->pos());
    if (shouldShowDelete != mShowDelete) {
        mShowDelete = shouldShowDelete;
        update();
    }

}
void ArrowItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    QGraphicsItem::hoverLeaveEvent(e);
    prepareGeometryChange();
    if (mShowDelete) {
        mShowDelete = false;
        update();
    }
}

QRectF ArrowItem::boundingRect() const
{
    const QString text = getBubbleText();

    qreal x = std::min(mXStart, mXEnd);
    qreal y = std::min(mYStart, mYEnd);
    qreal w = std::abs(mXEnd - mXStart);
    qreal h = std::abs(mYEnd - mYStart);


    if (!text.isEmpty()) {
        const QSize s = getBubbleSize(text);
        const qreal xa = (mXStart + mXEnd - s.width())/2.;
        const qreal ya = (mYStart + mYEnd - s.height())/2.;

        x = std::min(x, xa);
        y = std::min(y, ya);
        w = std::max(w, qreal (s.width()));
        h = std::max(h, qreal (s.height()));

    } else { // the arrow size in the shape()
        const qreal xa = (mXStart + mXEnd - 15.)/2.;
        const qreal ya = (mYStart + mYEnd - 15.)/2.;

        x = std::min(x, xa);
        y = std::min(y, ya);

        w = std::max(w, 15.);
        h = std::max(h, 15.);

    }

    return QRectF(x, y, w, h);
}

QPainterPath ArrowItem::shape() const
{
    QPainterPath path;
    QRectF rect = boundingRect();
    const qreal shift = 15;

    if (mXStart < mXEnd && mYStart >= mYEnd) {
        path.moveTo(mXStart + shift, mYStart);
        path.lineTo(mXStart, mYStart - shift);
        path.lineTo(mXEnd - shift, mYEnd);

        path.lineTo(mXEnd, mYEnd + shift);

    } else if (mXStart < mXEnd && mYStart < mYEnd) {
        path.moveTo(mXStart + shift, mYStart);

        path.lineTo(mXStart, mYStart + shift);

        path.lineTo(mXEnd - shift, mYEnd);

        path.lineTo(mXEnd, mYEnd - shift);

    } else if (mXStart >= mXEnd && mYStart < mYEnd) {
        path.moveTo(mXStart - shift, mYStart);
        path.lineTo(mXStart, mYStart + shift);
        path.lineTo(mXEnd + shift, mYEnd);
        path.lineTo(mXEnd, mYEnd - shift);

    } else if (mXStart >= mXEnd && mYStart >= mYEnd) {
        path.moveTo(mXStart - shift, mYStart);
        path.lineTo(mXStart, mYStart - shift);
        path.lineTo(mXEnd + shift, mYEnd);
        path.lineTo(mXEnd, mYEnd + shift);

    } else
        path.addRect(rect);

    return path;
}

void ArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    const int penWidth = 2;
    QColor color = mEditing ? QColor(77, 180, 62) : QColor(0, 0, 0);
    //set the Arrow under the Event
    setZValue(-1);

    if (mShowDelete) {
        color = Qt::red;
        //set the Arrow in front of the Event, like this we can click on
        setZValue(1);
    } else if (mGreyedOut)
        color.setAlphaF(0.05);

    painter->setPen(QPen(color, penWidth, mEditing ? Qt::DashLine : Qt::SolidLine));
    painter->drawLine(QPointF(qreal(mXStart), qreal(mYStart)), QPointF(qreal(mXEnd), qreal(mYEnd)));

    // Bubble

    const QString bubbleText = getBubbleText();
    bool showMiddleArrow = true;

    QFont font (qApp->font().family(), 10, 50, false);

    if (mShowDelete) {
        showMiddleArrow = false;
        painter->setPen(Qt::white);
        painter->setBrush(Qt::red);
        font.setBold(true);
        font.setPointSizeF(18.);

    } else {
        painter->setPen(Qt::black);
        painter->setBrush(Qt::white);
        font.setBold(false);
        font.setPointSizeF(12.);
    }

    painter->setFont(font);
    if (!bubbleText.isEmpty()) {
        showMiddleArrow = false;
        const QRectF br = getBubbleRect(bubbleText);

        if (bubbleText.count() > 5)
            painter->drawRect(br);
        else
            painter->drawEllipse(br);

        painter->drawText(br, Qt::AlignCenter, bubbleText);
     }

    // arrows

    const double angle_rad = atan( double(qAbs(mXStart-mXEnd)) / double(qAbs(mYStart-mYEnd)) );
    const double angle_deg = angle_rad * 180. / M_PI;

    QPainterPath path;
    const qreal arrow_w = 15;
    const qreal arrow_l = 25;
    path.moveTo(-arrow_w/2., arrow_l/2.);
    path.lineTo(arrow_w/2., arrow_l/2.);
    path.lineTo(0., -arrow_l/2.);
    path.closeSubpath();

    const QRectF axeBox = QRectF( qMin(mXStart, mXEnd), qMin(mYStart, mYEnd), qAbs(mXEnd-mXStart), qAbs(mYEnd-mYStart) );

    const qreal posX = axeBox.width()/2.;
    const qreal posY = axeBox.height()/2.;

    const qreal posX1 = axeBox.width()/3.;
    const qreal posX2 = axeBox.width()*0.6666;
    const qreal posY1 = axeBox.height()/3.;
    const qreal posY2 = axeBox.height()*0.6666;

    if (mXStart < mXEnd && mYStart >= mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY2);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY1);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }

    } else if (mXStart < mXEnd && mYStart < mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(180. - angle_deg);
            painter->fillPath(path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY1);
            painter->rotate(180. - angle_deg);
            painter->fillPath(path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY2);
            painter->rotate(180. - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }

    } else if (mXStart >= mXEnd && mYStart < mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(180. + angle_deg);
            painter->fillPath(path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY1);
            painter->rotate(180. + angle_deg);
            painter->fillPath(path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY2);
            painter->rotate(180. + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }

    } else if (mXStart >= mXEnd && mYStart >= mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY2);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY1);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    }


}

QSize ArrowItem::getBubbleSize(const QString& text) const
{
    int w = 0;
    int h = 0;
    if (!text.isEmpty()) {
        QFont font;
        if (mShowDelete)
            font.setPointSizeF(18.);
        else
            font.setPointSizeF(12.);

        QFontMetrics metrics(font);
        w = metrics.boundingRect(text).width() + 10 ;
        h = metrics.height() + 10;

        if (text.count() < 5) {
            w = std::max(w, h);
            h = w;
        }
    }

    return QSize(w, h);
}

QRectF ArrowItem::getBubbleRect(const QString& text) const
{
    const QSize s = getBubbleSize(text);
    const QRectF rect = boundingRect();

    const int bubble_x = rect.x() + (rect.width() - s.width()) / 2 ;
    const int bubble_y = rect.y() + (rect.height() - s.height()) / 2 ;
    return QRectF(QPoint(bubble_x, bubble_y), s);

}

QString ArrowItem::getBubbleText() const
{
    QString bubbleText;
    if (mShowDelete)
        if (mType == eEvent)
            bubbleText = "X";
        else
            bubbleText = "?";

    else if (mType == ePhase) {
            PhaseConstraint::GammaType gammaType = PhaseConstraint::GammaType (mData.value(STATE_CONSTRAINT_GAMMA_TYPE).toInt());
            if (gammaType == PhaseConstraint::eGammaFixed)
                bubbleText = "hiatus ≥ " + QString::number(mData.value(STATE_CONSTRAINT_GAMMA_FIXED).toDouble());
            else if (gammaType == PhaseConstraint::eGammaRange)
                bubbleText = "min hiatus ∈ [" + QString::number(mData.value(STATE_CONSTRAINT_GAMMA_MIN).toDouble()) +
                "; " + QString::number(mData.value(STATE_CONSTRAINT_GAMMA_MAX).toDouble()) + "]";
        }

    return bubbleText;
}

EventItem* ArrowItem::findEventItemWithJsonId(const int id)
{
     QList<AbstractItem*> listItems = mScene->getItemsList();
     foreach (AbstractItem* it, listItems) {
        EventItem* ev = static_cast<EventItem*>(it);
        const QJsonObject evJson = ev->getData();
        if (evJson.value(STATE_ID) == id)
            return ev;
    }
    return nullptr;
}
