#include "EventKnownItem.h"
#include "Phase.h"
#include "Event.h"
#include "EventsScene.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "GraphView.h"
#include "EventKnown.h"
#include "StdUtilities.h"
#include "Project.h"
#include <QtWidgets>


EventKnownItem::EventKnownItem(EventsScene* eventsScene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent):EventItem(eventsScene, event, settings, parent),
mThumbH(20),
mThumbVisible(true)
{
    setEvent(event, settings);
    mScene = static_cast<AbstractScene*>(eventsScene);
}

EventKnownItem::~EventKnownItem()
{
    
}

void EventKnownItem::setEvent(const QJsonObject& event, const QJsonObject& settings)
{
    prepareGeometryChange();
    
    mData = event;
    
    // ----------------------------------------------
    //  Update item position and selection
    // ----------------------------------------------
    setSelected(mData.value(STATE_IS_SELECTED).toBool());
    setPos(mData.value(STATE_ITEM_X).toDouble(),
           mData.value(STATE_ITEM_Y).toDouble());
    // ----------------------------------------------
    //  Check if item should be greyed out
    // ----------------------------------------------
    //updateGreyedOut();
    mGreyedOut = false;
    // ----------------------------------------------
    //  Recreate thumb
    // ----------------------------------------------
    const double tmin = settings.value(STATE_SETTINGS_TMIN).toDouble();
    const double tmax = settings.value(STATE_SETTINGS_TMAX).toDouble();
    const double step = settings.value(STATE_SETTINGS_STEP).toDouble();
    
    EventKnown bound = EventKnown::fromJson(event);
    // if Fixed Bound with fixed value in study period or uniform Bound with bound.mUniformStart<bound.mUniformEnd
   /* if(  ( (bound.mKnownType==EventKnown::eFixed) && (tmin<=bound.mFixed) && (bound.mFixed<=tmax) )
      || ( (bound.mKnownType==EventKnown::eUniform) &&
           (bound.mUniformStart<bound.mUniformEnd)  && (bound.mUniformStart< tmax) && (bound.mUniformEnd>tmin)     )) */
    if ( (tmin<=bound.mFixed) && (bound.mFixed<=tmax) ) {
        bound.updateValues(tmin, tmax, step);

        GraphView* graph = new GraphView();
        graph->setFixedSize(200, 50);
        graph->setMargins(0, 0, 0, 0);

        graph->setRangeX(tmin, tmax);
        graph->setCurrentX(tmin, tmax);
        graph->setRangeY(0, 1.);

        graph->showXAxisArrow(false);
        graph->showXAxisTicks(false);
        graph->showXAxisSubTicks(false);
        graph->showXAxisValues(false);

        graph->showYAxisArrow(false);
        graph->showYAxisTicks(false);
        graph->showYAxisSubTicks(false);
        graph->showYAxisValues(false);

        graph->setXAxisMode(GraphView::eHidden);
        graph->setYAxisMode(GraphView::eHidden);

        //---------------------

        GraphCurve curve;
        curve.mName = "Bound";
        curve.mBrush = Painting::mainColorLight;
        curve.mPen = QPen(Painting::mainColorLight, 2.);

        curve.mIsHorizontalSections = true;
        qreal tLower;
        qreal tUpper;

        tLower = bound.mFixed;
        tUpper = tLower;


        curve.mSections.append(qMakePair(tLower,tUpper));
        graph->addCurve(curve);
        //---------------------

        mThumb = QImage(graph->size(),QImage::Format_ARGB32_Premultiplied);
        graph->render(&mThumb);
        delete graph;

    } else
        mThumb = QImage();

    // ----------------------------------------------
    //  Repaint based on mEvent
    // ----------------------------------------------
    //update(); Done by prepareGeometryChange() at the function start
}

QRectF EventKnownItem::boundingRect() const
{
    //QFont font = qApp->font();
    //QFontMetrics metrics(font);
    //const QString name = mData.value(STATE_NAME).toString();
    
    //qreal w = metrics.width(name) + 2 * (mBorderWidth + mEltsMargin);
    // the size is independant of the size name

    qreal h = mTitleHeight + mThumbH + mPhasesHeight + 2*mEltsMargin;
    h += 40.;

    qreal w = 202;//qMax(w, 150.);
    //w += 52.;



    return QRectF(-w/2, -h/2, w, h);
}

void EventKnownItem::setDatesVisible(bool visible)
{
    mThumbVisible = visible;
}

void EventKnownItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect = boundingRect();

    const QColor eventColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
                               mData.value(STATE_COLOR_GREEN).toInt(),
                               mData.value(STATE_COLOR_BLUE).toInt());

    if (isSelected()) {
        painter->setPen(QPen(Painting::mainColorDark, 3.));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
    }

    const double side = 30;//40.;
    const double top = 15;//25.;

    QRectF nameRect(rect.x() + side, rect.y() + top, rect.width() - 2*side, mTitleHeight);
    QRectF thumbRect(rect.x() + side, rect.y() + top + mEltsMargin + mTitleHeight, rect.width() - 2*side, mThumbH);
    QRectF phasesRect(rect.x() + side, rect.y() + top + 2*mEltsMargin + mTitleHeight + mThumbH, rect.width() - 2*side, mPhasesHeight);

    phasesRect.adjust(1, 1, -1, -1);
    // detect selected phases and set mGreyedOut
    const QJsonArray phases = getPhases();
    const int numPhases = phases.size();
    const double w = phasesRect.width()/numPhases;

    if (mGreyedOut) //setting with setGreyedOut() just above
        painter->setOpacity(0.1);
    else
        painter->setOpacity(1.);

    // the elliptic item box
    painter->setPen(Qt::NoPen);
    painter->setBrush(eventColor);
    painter->drawEllipse(rect);

    // Name

    QFont font = AppSettings::font();
    font.setPointSizeF(11.);
    painter->setFont(font);
    QFontMetrics metrics(font);
    QString name = mData.value(STATE_NAME).toString();
    name = metrics.elidedText(name, Qt::ElideRight, nameRect.width() - 5);

    QColor frontColor = getContrastedColor(eventColor);
    painter->setPen(frontColor);
    painter->drawText(nameRect, Qt::AlignCenter, name);



    // thumbnail
    if (mThumb.isNull()) {
        painter->fillRect(thumbRect, Qt::white);
        painter->setPen(Qt::red);
        painter->drawText(thumbRect, Qt::AlignCenter, tr("Invalid bound"));
    }
    else if (mThumbVisible)
        painter->drawImage(thumbRect, mThumb, mThumb.rect());


    // Phases

    if (numPhases == 0) {
     //   QFont font = qApp->font();
        //font.setPointSizeF(11.);
        painter->setFont(font);
        painter->fillRect(phasesRect, QColor(0, 0, 0, 180));
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(phasesRect, Qt::AlignCenter, tr("No Phase"));
    } else {
        for (int i=0; i<numPhases; ++i) {
            QJsonObject phase = phases.at(i).toObject();
            QColor c(phase.value(STATE_COLOR_RED).toInt(),
                     phase.value(STATE_COLOR_GREEN).toInt(),
                     phase.value(STATE_COLOR_BLUE).toInt());
            painter->setPen(c);
            painter->setBrush(c);
            painter->drawRect(phasesRect.x() + i*w, phasesRect.y(), w, phasesRect.height());
        }
    }

    painter->setPen(QColor(0, 0, 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(phasesRect);


    // Border
    painter->setBrush(Qt::NoBrush);
    if (mMergeable) {
        painter->setPen(QPen(Qt::white, 5.));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));

        painter->setPen(QPen(Painting::mainColorLight, 3., Qt::DashLine));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
    } else if (isSelected()){
        painter->setPen(QPen(Qt::white, 5.));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));

        painter->setPen(QPen(Qt::red, 3.));
        painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
    }
    // restore Opacity
    painter->setOpacity(1.);

}

void EventKnownItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    e->ignore();
}

QRectF EventKnownItem::toggleRect() const
{
    return QRectF();
}
