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

#include "ArrowTmpItem.h"
#include <QtWidgets>

ArrowTmpItem::ArrowTmpItem(QGraphicsItem* parent):QGraphicsItem(parent),
mBubbleHeight(30.),
mXFrom(0),
mYFrom(0),
mXTo(0),
mYTo(0),
mState(eNormal),
mLocked(false)
{
    // set the Arrow over all item
    setZValue(2.);
}

void ArrowTmpItem::setFrom(const int &x, const int &y)
{
    mXFrom = x;
    mYFrom = y;
}

void ArrowTmpItem::setTo(const int& x, const int& y)
{
    prepareGeometryChange();
    if (!mLocked) {
        mXTo = x;
        mYTo = y;
    }
}

void ArrowTmpItem::setState(const State state)
{
    prepareGeometryChange();
    mState = state;
}

void ArrowTmpItem::setLocked(bool locked)
{
    mLocked = locked;
}

QRectF ArrowTmpItem::boundingRect() const
{
    const int penWidth = 2;
    const qreal arrow_w (15.);
    qreal x = qMin(mXFrom, mXTo);
    qreal y = qMin(mYFrom, mYTo);
    qreal w = std::abs(mXTo - mXFrom) + 2*penWidth + arrow_w; // when the arrow is horizontal
    qreal h = std::abs(mYTo - mYFrom) + 2*penWidth + arrow_w; // when the arrow is vertical

    const QString text = getBubbleText();

    if (!text.isEmpty()) {
        const qreal xa = (mXFrom + mXTo - mBubbleHeight)/2.;
        const qreal ya = (mYFrom + mYTo - mBubbleHeight)/2.;

        x = qMin(x, xa);
        y = qMin(y, ya);
        w = qMax(w, mBubbleHeight);
        h = qMax(h, mBubbleHeight);
    }

    return QRectF(x, y, w, h);

}

QString ArrowTmpItem::getBubbleText() const
{
    QString bubbleText = ""; //mState == eNormal
    if (mState == eForbidden)
        bubbleText = "X";

    else if (mState == eAllowed)
        bubbleText = "OK";

     return bubbleText;
}

/**
 * @brief ArrowTmpItem::paint Draw the dach line linking Event or Phase
 * @param painter
 * @param option
 * @param widget
 */
void ArrowTmpItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    const int penWidth (2);
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
     }
    painter->setPen(QPen(color, penWidth, Qt::DashLine));
    painter->drawLine(mXFrom, mYFrom, mXTo, mYTo);

    // arrows

    const double angle_rad = atan(double(std::abs(mXFrom-mXTo)) / double(std::abs(mYFrom-mYTo)));
    const double angle_deg = angle_rad * 180. / M_PI;

    QPainterPath path;
    const qreal arrow_w (10.);
    const qreal arrow_l (15.);
    path.moveTo(-arrow_w/2., arrow_l/2.);
    path.lineTo(arrow_w/2., arrow_l/2.);
    path.lineTo(0, -arrow_l/2.);
    path.closeSubpath();

     const QPoint centrum ((mXFrom + mXTo)/2, (mYFrom + mYTo)/2);

    if (mXFrom < mXTo && mYFrom > mYTo) {
        painter->save();
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(angle_deg);
        painter->fillPath(path, color);
        painter->restore();

    } else if (mXFrom < mXTo && mYFrom < mYTo) {
        painter->save();
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(180 - angle_deg);
        painter->fillPath(path, color);
        painter->restore();

    } else if (mXFrom > mXTo && mYFrom < mYTo) {
        painter->save();
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(180 + angle_deg);
        painter->fillPath(path, color);
        painter->restore();

    } else if (mXFrom > mXTo && mYFrom > mYTo) {
        painter->save();
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(-angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }

    // Message
    const qreal ra (mBubbleHeight/2. * 0.7 * .5); // radius of the cercle * sin(45°) * coef

    QRectF r(centrum.x() - mBubbleHeight/2., centrum.y() - mBubbleHeight/2., mBubbleHeight, mBubbleHeight);
    QFont font (qApp->font());
    font.setBold(true);
    font.setPointSizeF(mBubbleHeight*0.4); //depend of the font
    QFontMetrics fm (font);
    painter->setFont(font);
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rTex(centrum.x() - fm.boundingRect(getBubbleText()).width()/2., centrum.y() - fm.ascent()/2., fm.boundingRect(getBubbleText()).width(), fm.height());

    switch (mState) {

        case eForbidden:
            //qDebug() <<"ArrowTmpItem::paint mSate==eForbidden";
            painter->setBrush(Qt::white);
            painter->drawEllipse(r);
            painter->setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap));
            painter->drawLine(QPointF(centrum.x() - ra, centrum.y() - ra), QPointF(centrum.x() + ra, centrum.y() + ra));
            painter->drawLine(QPointF(centrum.x() - ra, centrum.y() + ra),QPointF( centrum.x() + ra, centrum.y() - ra));

        break;

        case eAllowed:
            //qDebug() <<"ArrowTmpItem::paint mSate==eAllowed";
            painter->setBrush(Qt::white);
            painter->drawEllipse(r);

            painter->drawText(rTex,  getBubbleText());//, QTextOption(Qt::AlignCenter));

        break;

        case eNormal:
        break;

    }
}
