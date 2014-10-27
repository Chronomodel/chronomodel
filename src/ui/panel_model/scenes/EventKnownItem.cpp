#include "EventKnownItem.h"
#include "Phase.h"
#include "Event.h"
#include "Painting.h"
#include "ProjectManager.h"
#include "Project.h"
#include "QtUtilities.h"
#include "Painting.h"
#include <QtWidgets>


EventKnownItem::EventKnownItem(EventsScene* eventsScene, const QJsonObject& event, QGraphicsItem* parent):EventItem(eventsScene, event, parent)
{
    mPhasesHeight = 20.f;
}

EventKnownItem::~EventKnownItem()
{
    
}

QRectF EventKnownItem::boundingRect() const
{
    QFont font = qApp->font();
    QFontMetrics metrics(font);
    QString name = mEvent[STATE_EVENT_NAME].toString();
    
    qreal w = metrics.width(name) + 2 * (mBorderWidth + mEltsMargin);
    qreal h = mTitleHeight + mPhasesHeight + 2*mBorderWidth + 3*mEltsMargin;
    
    w = qMax(w, 100.);
    
    w += 40;
    h += 40;
    
    return QRectF(-w/2, -h/2, w, h);
}

void EventKnownItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF rect = boundingRect();
    QRectF r = rect.adjusted(20, 20, -20, -20);
    
    QColor eventColor = QColor(mEvent[STATE_EVENT_RED].toInt(),
                               mEvent[STATE_EVENT_GREEN].toInt(),
                               mEvent[STATE_EVENT_BLUE].toInt());
    QColor eventColorLight = eventColor;
    eventColorLight.setAlpha(100);
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(eventColor);
    painter->drawEllipse(rect);
    
    if(isSelected())
    {
        painter->setPen(QPen(mainColorDark, 3.f));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
    }
    
    // Phases
    
    /*int y = r.height() - mBorderWidth - mPhasesHeight;
    QRectF pr = r.adjusted(0, y, 0, 0);
    QPainterPath clip;
    clip.addRoundedRect(rect, 5, 5);
    painter->setClipPath(clip);
    int numPhases = (int)mEvent->mPhases.size();
    float w = pr.width()/numPhases;
    for(int i=0; i<numPhases; ++i)
    {
        QColor c = mEvent->mPhases[i]->mColor;
        painter->setPen(c);
        painter->setBrush(c);
        painter->drawRect(r.x() + i*w, pr.y(), w, pr.height());
    }
    if(numPhases == 0)
    {
        painter->setPen(isSelected() ? Qt::white : Qt::black);
        painter->drawText(pr, Qt::AlignCenter, QObject::tr("No Phase"));
    }*/
    
    // Name
    
    QRectF tr(r.x() + mBorderWidth + mEltsMargin,
              r.y() + mBorderWidth + mEltsMargin,
              r.width() - 2*mBorderWidth - 2*mEltsMargin,
              mTitleHeight);
    
    QFont font = qApp->font();
    painter->setFont(font);
    QFontMetrics metrics(font);
    QString name = mEvent[STATE_EVENT_NAME].toString();
    name = metrics.elidedText(name, Qt::ElideRight, tr.width());
    
    QColor frontColor = getContrastedColor(eventColor);
    painter->setPen(frontColor);
    painter->drawText(tr, Qt::AlignCenter, name);
}

void EventKnownItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    e->ignore();
}

QRectF EventKnownItem::toggleRect() const
{
    return QRectF();
}
