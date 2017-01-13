#ifndef ModelUtilities_H
#define ModelUtilities_H

#include <QIcon>
#include <QString>
#include <QVector>
#include "Model.h"
#include "Date.h"
#include "Event.h"
#include "Phase.h"


class ModelUtilities
{
public:
    static QString getEventMethodText(const Event::Method method);
    static QString getDataMethodText(const Date::DataMethod method);
    static QString getDeltaText(const Date& date);
    
    static Event::Method getEventMethodFromText(const QString& text);
    static Date::DataMethod getDataMethodFromText(const QString& text);
    
    static QVector<QVector<Event*> > getNextBranches(const QVector<Event*>& curBranch, Event* lastNode);
    static QVector<QVector<Event*> > getBranchesFromEvent(Event* start);
    static QVector<QVector<Event*> > getAllEventsBranches(const QList<Event*>& events);
    
    static QVector<QVector<Phase*> > getNextBranches(const QVector<Phase*>& curBranch, Phase* lastNode, const double gammaSum, const double maxLength);
    static QVector<QVector<Phase*> > getBranchesFromPhase(Phase* start, const double maxLength);
    static QVector<QVector<Phase*> > getAllPhasesBranches(const QList<Phase*>& events, const double maxLength);
    
    static QVector<Event*> sortEventsByLevel(const QList<Event*>& events);
    static QVector<Phase*> sortPhasesByLevel(const QList<Phase*>& phases);
    
    static QVector<Event*> unsortEvents(const QList<Event*>& events);
    
    static QString dateResultsText(const Date* d, const Model* model = nullptr, const bool forCSV = false);
    static QString eventResultsText(const Event* e, bool withDates, const Model* model = nullptr, const bool forCSV = false);
    static QString phaseResultsText(const Phase* p, const bool forCSV = false);
    static QString constraintResultsText(const PhaseConstraint* p, const bool forCSV = false);
    
    static QString dateResultsHTML(const Date* d, const Model* model = nullptr);
    static QString eventResultsHTML(const Event* e,const bool withDates, const Model* model = nullptr);
    static QString phaseResultsHTML(const Phase* p);
    static QString constraintResultsHTML(const PhaseConstraint* p);

    static short HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const Model* model);
};

// These 2 global functions are used to sort events and phases lists in result view

bool sortEvents(Event* e1, Event* e2);
bool sortPhases(Phase* p1, Phase* p2);



#endif

