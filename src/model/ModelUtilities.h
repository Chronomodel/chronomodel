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

#ifndef MODELUTILITIES_H
#define MODELUTILITIES_H

#include "ModelCurve.h"
#include "Date.h"
#include "Event.h"
#include "Phase.h"

#include <QIcon>
#include <QString>

class ModelUtilities
{
public:

    static std::vector<std::vector<std::shared_ptr<Event>>> getNextBranches(const std::vector<std::shared_ptr<Event>> &curBranch, std::shared_ptr<Event> lastNode);
    static std::vector<std::vector<std::shared_ptr<Event>>> getBranchesFromEvent(std::shared_ptr<Event> start);
    static std::vector<std::vector<std::shared_ptr<Event>>> getAllEventsBranches(const std::vector<std::shared_ptr<Event>>& events);

    static std::vector<std::vector<Phase *> > getNextBranches(const std::vector<Phase *> &curBranch, const Phase *lastNode, const double gammaSum, const double maxLength);
    static std::vector<std::vector<Phase *> > getBranchesFromPhase(Phase *start, const double maxLength);
    static std::vector<std::vector<Phase*> > getAllPhasesBranches(const std::vector<std::shared_ptr<Phase> > &events, const double maxLength);

    static std::vector<std::shared_ptr<Event>> unsortEvents(const std::vector<std::shared_ptr<Event>> &events);
    static QString modelDescriptionHTML(const std::shared_ptr<ModelCurve> model);
    static QString getMCMCSettingsLog(const std::shared_ptr<ModelCurve> model = nullptr);
    static QString modelStateDescriptionHTML(const std::shared_ptr<ModelCurve> model = nullptr, QString stateDescript = "");
    static QString modelStateDescriptionText(const std::shared_ptr<ModelCurve> model = nullptr, QString stateDescript = "");

#pragma mark Results in HTML format
    static QString dateResultsHTML(const Date* d, const std::shared_ptr<ModelCurve> &model = nullptr);
    static QString dateResultsHTML(const Date* d, const double tmin_formated, const double tmax_formated);
    static QString sigmaTiResultsHTML(const Date* d);

    static QString eventResultsHTML(const std::shared_ptr<Event> e, const bool withDates, const std::shared_ptr<ModelCurve> model = nullptr);
    static QString eventResultsHTML(const std::shared_ptr<Event> e, const bool withDates, const double tmin_formated, const double tmax_formated, bool with_curve = false);
    static QString EventS02ResultsHTML(const std::shared_ptr<Event> e);
    static QString VgResultsHTML(const std::shared_ptr<Event> e);

    static QString phaseResultsHTML(const std::shared_ptr<Phase> p);

    static QString tempoResultsHTML(const std::shared_ptr<Phase> p);
    static QString activityResultsHTML(const std::shared_ptr<Phase> p);
    static QString durationResultsHTML(const std::shared_ptr<Phase> p);
    static QString constraintResultsHTML(const std::shared_ptr<PhaseConstraint> p);

    static QString curveResultsHTML(const std::shared_ptr<ModelCurve> model = nullptr);
    static QString lambdaResultsHTML(const std::shared_ptr<ModelCurve> model = nullptr);
    static QString S02ResultsHTML(const std::shared_ptr<ModelCurve> model = nullptr);

    static short HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const std::shared_ptr<ModelCurve> model);
    static short HPDOutsideSudyPeriod(const std::map<double, double> &hpd, const std::shared_ptr<ModelCurve> model);

    static short HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const double tmin_formated, const double tmax_formated);
    static short HPDOutsideSudyPeriod(const std::map<double, double> &hpd, const double tmin_formated, const double tmax_formated);

};

std::string html_to_plain_text(const std::string &html);
QString html_to_plain_text(const QString &html);


double sample_in_repartition(std::shared_ptr<CalibrationCurve> calibrateCurve, const double min, const double max);

void sampleInCumulatedRepartition_thetaFixe (std::shared_ptr<Event> event, const StudyPeriodSettings &settings);
double sample_in_Repartition_date_fixe(const Date &d, const StudyPeriodSettings& settings);

// These 2 global functions are used to sort events and phases lists in result view

bool sortEvents(std::shared_ptr<Event> e1, std::shared_ptr<Event> e2);
bool sortPhases(std::shared_ptr<Phase> p1, std::shared_ptr<Phase> p2);


#endif
