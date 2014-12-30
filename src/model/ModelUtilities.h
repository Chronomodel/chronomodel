#ifndef ModelUtilities_H
#define ModelUtilities_H

#include <QIcon>
#include <QString>
#include <QVector>
#include "Date.h"
#include "Event.h"
#include "Phase.h"


class ModelUtilities
{
public:
    static QString getEventMethodText(Event::Method method);
    static QString getDataMethodText(Date::DataMethod method);
    static QString getDeltaText(const Date& date);
    
    static QVector<QVector<Event*>> getNextBranches(const QVector<Event*>& curBranch, Event* lastNode);
    static QVector<QVector<Event*>> getBranchesFromEvent(Event* start);
    static QVector<QVector<Event*>> getAllEventsBranches(const QList<Event*>& events);
    
    static QVector<QVector<Phase*>> getNextBranches(const QVector<Phase*>& curBranch, Phase* lastNode, const double gammaSum, const double maxLength);
    static QVector<QVector<Phase*>> getBranchesFromPhase(Phase* start, const double maxLength);
    static QVector<QVector<Phase*>> getAllPhasesBranches(const QList<Phase*>& events, const double maxLength);
    
    static QVector<Event*> sortEventsByLevel(const QList<Event*>& events);
};

#endif