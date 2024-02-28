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

#include "ArrowItem.h"
#include "EventItem.h"
#include "MainWindow.h"
#include "PhaseItem.h"

#include <QtWidgets>

#include <math.h>


ArrowItem::ArrowItem(AbstractScene* scene, Type type, const QJsonObject& constraint, QGraphicsItem* parent):QGraphicsItem(parent),
    mType(type),
    mScene(scene),
    mStart(0, 0),
    mEnd(0., 0),
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

    setData(constraint); // init position
    //mData = constraint;
   /* QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(30);
    shadow->setOffset(3, 3);
    setGraphicsEffect(shadow);*/
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
    mStart = QPointF(x, y);

}

void ArrowItem::setTo(const double x, const double y)
{
    prepareGeometryChange();
    mEnd = QPointF(x, y);
}

void ArrowItem::setGreyedOut(bool greyedOut)
{
    mGreyedOut = greyedOut;
    update();
}

void ArrowItem::updatePosition()
{
    prepareGeometryChange();

    const QJsonObject &state = MainWindow::getInstance()->getState();

    const int fromId = mData.value(STATE_CONSTRAINT_BWD_ID).toInt();
    const int toId = mData.value(STATE_CONSTRAINT_FWD_ID).toInt();

    QJsonObject from;
    QJsonObject to;

    if (mType == eEvent) {
        const QJsonArray &events = state.value(STATE_EVENTS).toArray();
        for (const auto& ev : events) {
            const QJsonObject &event = ev.toObject();
            if (event.value(STATE_ID).toInt() == fromId)
                from = event;
            if (event.value(STATE_ID).toInt() == toId)
                to = event;
        }
    } else {
        const QJsonArray &phases = state.value(STATE_PHASES).toArray();
        for (const auto &ph : phases ) {
            const QJsonObject &phase = ph.toObject();
            if (phase.value(STATE_ID).toInt() == fromId)
                from = phase;
            if (phase.value(STATE_ID).toInt() == toId)
                to = phase;
        }
    }

    if (mType == eEvent) {
        EventItem* ev_from = findEventItemWithJsonId(fromId);
        mStart = ev_from->pos();

        EventItem* ev_to = findEventItemWithJsonId(toId);
        mEnd = ev_to->pos();

        const double angle_rad = atan2( double(ev_from->y()  - ev_to->y() ) , double(ev_from->x()-ev_to->x()) );
        //const double angle_deg = angle_rad * 180. / M_PI;
        //qDebug()<<" theta contactpos from = "<<angle_deg;

        mStartContact = contactPos(angle_rad, ev_from);


        const double angle_rad2 = atan2( double(ev_to->y()  - ev_from->y() ) , double(ev_to->x()-ev_from->x()) );
        //const double angle_deg2 = angle_rad2 * 180. / M_PI;
        //qDebug()<<" theta contactpos to = "<<angle_deg2;
        mEndContact = contactPos(angle_rad2, ev_to);

    } else {
        PhaseItem* ph_from = findPhaseItemWithJsonId(fromId);
        mStart = ph_from->pos();

        PhaseItem* ph_to = findPhaseItemWithJsonId(toId);
        mEnd = ph_to->pos();

const double angle_rad = atan2( double(ph_from->y()  - ph_to->y() ) , double(ph_from->x()-ph_to->x()) );
       // const double angle_deg = angle_rad * 180. / M_PI;

        mStartContact = contactPos(angle_rad, ph_from);

       // qDebug()<<" theta contactpos from = "<<angle_deg;

        const double angle_rad2 = atan2( double(ph_to->y()  - ph_from->y() ) , double(ph_to->x()-ph_from->x()) );
        //const double angle_deg2 = angle_rad2 * 180. / M_PI;
        //qDebug()<<" theta contactpos to = "<<angle_deg2;
        mEndContact = contactPos(angle_rad2, ph_to);
    }

}

QPointF ArrowItem::contactPos(const double theta, AbstractItem* e)
{
    const double item_height = e->boundingRect().height() - 20.; // Il faut enlever l'espace suplémentaire, servant à effacer la trace
    const double item_width = e->boundingRect().width() - 20.;
    double a1 = atan2( item_height , item_width );
    double a2 = atan2(-item_height , item_width );
    double a3 = atan2( -item_height , -item_width );
    double a4 = atan2( item_height ,- item_width );
#ifdef DEBUG
   /* auto a11 = a1* 180. / M_PI;
    auto a22 = a2* 180. / M_PI;
    auto a33 = a3* 180. / M_PI;
    auto a44 = a4* 180. / M_PI;
    qDebug()<<"[ArrowItem::contactPos] "<<a11<<a22<<a33<<a44;
    qDebug()<<"[ArrowItem::contactPos] theta "<<theta* 180. / M_PI;;
*/
#endif
    double xp, yp;
    if (a3<=theta && theta<a2) {
        xp = e->x() + item_height/tan(theta) /2.; // tan(theta) < 0
        yp = e->y() + item_height /2.;

    } else if (a2<=theta && theta<a1) {
        xp = e->x() - item_width /2.;
        yp = e->y() - tan(theta)* item_width /2.;


    } else if (a1<=theta && theta<a4) {
        xp = e->x() - item_height/tan(theta) /2.;
        yp = e->y() - item_height /2.;

    } else {  // if (a3<theta && theta<a4) {
        xp = e->x() +  item_width /2.;
        yp = e->y() + tan(theta)* item_width /2.;
    }
    return QPointF(xp, yp);
}


void ArrowItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem::mouseDoubleClickEvent(e);
    mScene->constraintDoubleClicked(this, e);
}

void ArrowItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
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

    qreal x = std::min(mStart.x(), mEnd.x());
    qreal y = std::min(mStart.y(), mEnd.y());
    qreal w = std::abs(mEnd.x() - mStart.x());
    qreal h = std::abs(mEnd.y() - mStart.y());


    if (!text.isEmpty()) {
        const QSizeF s = getBubbleSize(text);
        const qreal xa = (mStartContact.x() + mEndContact.x() - s.width())/2.;
        const qreal ya = (mStartContact.y() + mEndContact.y() - s.height())/2.;

        x = std::min(x, xa);
        y = std::min(y, ya);
        w = std::max(w, qreal (s.width()));
        h = std::max(h, qreal (s.height()));

    } else { // the arrow size in the shape()
        const qreal xa = (mStart.x() + mEnd.x() - 15.)/2.;
        const qreal ya = (mStart.y() + mEnd.y() - 15.)/2.;

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

    if (mStart.x() < mEnd.x() && mStart.y() >= mEnd.y()) {
        path.moveTo(mStart.x() + shift, mStart.y());
        path.lineTo(mStart.x(), mStart.y() - shift);
        path.lineTo(mEnd.x() - shift, mEnd.y());

        path.lineTo(mEnd.x(), mEnd.y() + shift);

    } else if (mStart.x() < mEnd.x() && mStart.y() < mEnd.y()) {
        path.moveTo(mStart.x() + shift, mStart.y());

        path.lineTo(mStart.x(), mStart.y() + shift);

        path.lineTo(mEnd.x() - shift, mEnd.y());

        path.lineTo(mEnd.x(), mEnd.y() - shift);

    } else if (mStart.x() >= mEnd.x() && mStart.y() < mEnd.y()) {
        path.moveTo(mStart.x() - shift, mStart.y());
        path.lineTo(mStart.x(), mStart.y() + shift);
        path.lineTo(mEnd.x() + shift, mEnd.y());
        path.lineTo(mEnd.x(), mEnd.y() - shift);

    } else if (mStart.x() >= mEnd.x() && mStart.y() >= mEnd.y()) {
        path.moveTo(mStart.x() - shift, mStart.y());
        path.lineTo(mStart.x(), mStart.y() - shift);
        path.lineTo(mEnd.x() + shift, mEnd.y());
        path.lineTo(mEnd.x(), mEnd.y() + shift);

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
    painter->drawLine(mStart, mEnd);

    // Arrows

    const QString bubbleText = getBubbleText();

    const bool showMiddleArrow = !mShowDelete && bubbleText.isEmpty();

    const double angle_rad = atan( double(qAbs(mStart.x() - mEnd.x())) / double(qAbs(mStart.y() - mEnd.y())) );
    const double angle_deg = angle_rad * 180. / M_PI;

    ;
    const qreal arrow_w = 15.;
    const qreal arrow_l = 25.;
    QPainterPath arrow_path(QPointF(-arrow_w/2., arrow_l/2.));
    arrow_path.lineTo(arrow_w/2., arrow_l/2.);
    arrow_path.lineTo(0., -arrow_l/2.);
    arrow_path.closeSubpath();

    const QRectF axeBox = QRectF( std::min(mStartContact.x(), mEndContact.x()), std::min(mStartContact.y(), mEndContact.y()), std::abs(mEndContact.x() - mStartContact.x()), std::abs(mEndContact.y() - mStartContact.y()) );

    const qreal posX = axeBox.width()/2.;
    const qreal posY = axeBox.height()/2.;

    const qreal posX1 = axeBox.width()/4.;
    const qreal posX2 = 3.*axeBox.width()/4.;
    const qreal posY1 = axeBox.height()/4.;
    const qreal posY2 = axeBox.height()*3./4.;

    if (mStart.x() < mEnd.x() && mStart.y() >= mEnd.y()) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY2);
            painter->rotate(angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY1);
            painter->rotate(angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();
        }

    } else if (mStart.x() < mEnd.x() && mStart.y() < mEnd.y()) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(180. - angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY1);
            painter->rotate(180. - angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY2);
            painter->rotate(180. - angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();
        }

    } else if (mStart.x() >= mEnd.x() && mStart.y() < mEnd.y()) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(180. + angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY1);
            painter->rotate(180. + angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY2);
            painter->rotate(180. + angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();
        }

    } else if (mStart.x() >= mEnd.x() && mStart.y() >= mEnd.y()) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(-angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

        } else {
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY1);
            painter->rotate(-angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY2);
            painter->rotate(-angle_deg);
            painter->fillPath(arrow_path, color);
            painter->restore();

        }
    }


    // Bubble
    QFont font (qApp->font().family(), 10, 50, false);

    if (mShowDelete) {
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
        const QRectF br = getBubbleRect(bubbleText);

        if (bubbleText.size() > 5)
            painter->drawRect(br);
        else
            painter->drawEllipse(br);

        painter->drawText(br, Qt::AlignCenter, bubbleText);
    }

}

QSizeF ArrowItem::getBubbleSize(const QString& text) const
{
    qreal w = 0;
    qreal h = 0;
    if (!text.isEmpty()) {
        QFont font;
        if (mShowDelete)
            font.setPointSizeF(18.);
        else
            font.setPointSizeF(12.);

        QFontMetricsF metrics(font);
        w = metrics.horizontalAdvance(text) + 10 ;
        h = metrics.height() + 10;

        if (text.size() < 5) {
            w = std::max(w, h);
            h = w;
        }
    }

    return QSizeF(w, h);
}

QRectF ArrowItem::getBubbleRect(const QString& text) const
{
    const QSizeF s = getBubbleSize(text);

    const qreal bubble_x = (mStartContact.x() + mEndContact.x() - s.width()) / 2. ;
    const qreal bubble_y = (mStartContact.y() + mEndContact.y() - s.height()) / 2. ;

    return QRectF(QPointF(bubble_x, bubble_y), s);

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

PhaseItem* ArrowItem::findPhaseItemWithJsonId(const int id)
{
    QList<AbstractItem*> listItems = mScene->getItemsList();
    foreach (AbstractItem* it, listItems) {
        PhaseItem* ph = static_cast<PhaseItem*>(it);
        const QJsonObject phJson = ph->getData();
        if (phJson.value(STATE_ID) == id)
            return ph;
    }
    return nullptr;
}
