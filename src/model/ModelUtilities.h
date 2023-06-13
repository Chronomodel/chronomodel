/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2021

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

#ifndef MODELUTILITIES_H
#define MODELUTILITIES_H

#include "Model.h"
#include "ModelCurve.h"
#include "Date.h"
#include "Event.h"
#include "Phase.h"

#include <QIcon>
#include <QString>
#include <QVector>

class ModelUtilities
{
public:

   // static QString getDataMethodText(const Date::DataMethod method);
    static QString getDeltaText(const Date& date); // Obsolete , is replaced by the Date::getWiggleDesc()


    //static Event::Method getEventMethodFromText(const QString& text);
   // static Date::DataMethod getDataMethodFromText(const QString& text);

    static QVector<QVector<Event*> > getNextBranches(const QVector<Event*>& curBranch, Event* lastNode);
    static QVector<QVector<Event*> > getBranchesFromEvent(Event* start);
    static QVector<QVector<Event*> > getAllEventsBranches(const QList<Event*>& events);

    static QVector<QVector<Phase*> > getNextBranches(const QVector<Phase*>& curBranch, Phase* lastNode, const double gammaSum, const double maxLength);
    static QVector<QVector<Phase*> > getBranchesFromPhase(Phase* start, const double maxLength);
    static QVector<QVector<Phase*> > getAllPhasesBranches(const QList<Phase*>& events, const double maxLength);

    // static QVector<Event*> sortEventsByLevel(const QList<Event*>& events); // Obsolete
    // static QVector<Phase*> sortPhasesByLevel(const QList<Phase*>& phases); // Obsolete

    static QVector<Event*> unsortEvents(const QList<Event*>& events);
    static QString modelDescriptionHTML(const ModelCurve* model);
    static QString getMCMCSettingsLog(const Model* model = nullptr);
    static QString modelStateDescriptionHTML(const ModelCurve *model = nullptr, QString stateDescript = "");
    static QString modelStateDescriptionText(const ModelCurve *model = nullptr, QString stateDescript = "");

    // Results in text format
    static QString dateResultsText(const Date* d, const Model* model = nullptr, const bool forCSV = false);
    static QString sigmaTiResultsText(const Date* d, const bool forCSV = false);

    static QString eventResultsText(const Event* e, bool withDates, const Model* model = nullptr, const bool forCSV = false);
    static QString VgResultsText(const Event* e);

    static QString phaseResultsText(const Phase* p, const bool forCSV = false);
    static QString durationResultsText(const Phase* p, const bool forCSV = false);
    static QString tempoResultsText(const Phase* p);
    static QString activityResultsText(const Phase* p, const bool forCSV = false);

    static QString constraintResultsText(const PhaseConstraint* p, const bool forCSV = false);

    static QString curveResultsText(const ModelCurve* model = nullptr);
    static QString lambdaResultsText(const ModelCurve* model = nullptr);
    static QString S02ResultsText(const ModelCurve* model = nullptr);

    // Results in HTML format
    static QString dateResultsHTML(const Date* d, const Model* model = nullptr);
    static QString sigmaTiResultsHTML(const Date* d);

    static QString eventResultsHTML(const Event* e, const bool withDates, const Model* model = nullptr);
    static QString VgResultsHTML(const Event* e);

    static QString phaseResultsHTML(const Phase* p);

    static QString tempoResultsHTML(const Phase* p);
    static QString activityResultsHTML(const Phase* p);
    static QString durationResultsHTML(const Phase* p);
    static QString constraintResultsHTML(const PhaseConstraint* p);

    static QString curveResultsHTML(const ModelCurve* model = nullptr);
    static QString lambdaResultsHTML(const ModelCurve* model = nullptr);
    static QString S02ResultsHTML(const ModelCurve* model = nullptr);

    static short HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const Model* model);
};

void sampleInCumulatedRepartition (Event *event, const StudyPeriodSettings& settings, const double min, const double max);
void sampleInCumulatedRepartition_thetaFixe (Event *event, const StudyPeriodSettings& settings);

// These 2 global functions are used to sort events and phases lists in result view

bool sortEvents(Event* e1, Event* e2);
bool sortPhases(Phase* p1, Phase* p2);



#endif
