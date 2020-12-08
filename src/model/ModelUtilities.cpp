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

#include "ModelUtilities.h"
#include "Date.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"
#include "../PluginAbstract.h"
#include "QtUtilities.h"
#include "Generator.h"

#include <QObject>
#include <utility>

#define MHAdaptGaussStr QObject::tr("MH : proposal = adapt. Gaussian random walk")
#define BoxMullerStr QObject::tr("AR : proposal = Gaussian")
#define DoubleExpStr QObject::tr("AR : proposal = Double-Exponential")

#define MHIndependantStr QObject::tr("MH : proposal = prior distribution")
#define InversionStr QObject::tr("MH : proposal = distribution of calibrated date")
#define MHSymGaussAdaptStr QObject::tr("MH : proposal = adapt. Gaussian random walk")


bool sortEvents(Event* e1, Event* e2) {return (e1->mItemY < e2->mItemY);}
bool sortPhases(Phase* p1, Phase* p2) {return (p1->mItemY < p2->mItemY);}

Event::Method ModelUtilities::getEventMethodFromText(const QString& text)
{
    if (text == MHAdaptGaussStr)
        return Event::eMHAdaptGauss;

    else if (text == BoxMullerStr)
        return Event::eBoxMuller;

    else if (text == DoubleExpStr)
        return Event::eDoubleExp;

    else  {
        // ouch... what to do ???
        return Event::eDoubleExp;
    }
}

QString ModelUtilities::getEventMethodText(const Event::Method method)
{
    switch(method)
    {
        case Event::eMHAdaptGauss:
        {
            return MHAdaptGaussStr;
        }
        case Event::eBoxMuller:
        {
            return BoxMullerStr;
        }
        case Event::eDoubleExp:
        {
            return DoubleExpStr;
        }
        default:
        {
            return QObject::tr("Unknown");
        }
    }
}

Date::DataMethod ModelUtilities::getDataMethodFromText(const QString& text)
{
    if (text == MHIndependantStr)
        return Date::eMHSymetric;

    else if (text == InversionStr)
        return Date::eInversion;

    else if (text == MHSymGaussAdaptStr)
        return Date::eMHSymGaussAdapt;

    else {
        // ouch... what to do ???
        return Date::eMHSymGaussAdapt;
    }
}

QString ModelUtilities::getDataMethodText(const Date::DataMethod method)
{
    switch(method)
    {
        case Date::eMHSymetric:
        {
            return MHIndependantStr;
        }
        case Date::eInversion:
        {
            return InversionStr;
        }
        case Date::eMHSymGaussAdapt:
        {
            return MHSymGaussAdaptStr;
        }
        default:
        {
            return QObject::tr("Unknown");
        }
    }
}

// Obsolete , is replaced by the Date::getWiggleDesc()
QString ModelUtilities::getDeltaText(const Date& date)
{
    QString result;
    const PluginAbstract* plugin = date.mPlugin;
    const QString str = QObject::tr("Wiggle");
    if (plugin && plugin->wiggleAllowed()) {
        switch (date.mDeltaType) {
            case Date::eDeltaFixed:
                result = date.mDeltaFixed != 0. ?  str + " : " + QString::number(date.mDeltaFixed) : "";
                break;
            case Date::eDeltaRange:
                result = (date.mDeltaMin !=0. && date.mDeltaMax != 0.) ? str + " : [" + QString::number(date.mDeltaMin) + ", " + QString::number(date.mDeltaMax) + "]" : "";
                break;
            case Date::eDeltaGaussian:
                result = (date.mDeltaError>0.) ? str + " : " + QString::number(date.mDeltaAverage) + " ± " + QString::number(date.mDeltaError) : "";
                break;
            case Date::eDeltaNone:
            default:
                break;
        }
    }
    return result;
}

// Events Branches
QVector<QVector<Event*> > ModelUtilities::getNextBranches(const QVector<Event*>& curBranch, Event* lastNode)
{
    QVector<QVector<Event*> > branches;
    QList<EventConstraint*> cts = lastNode->mConstraintsFwd;
    if (cts.size() > 0) {
        for (int i=0; i<cts.size(); ++i) {
            QVector<Event*> branch = curBranch;
            Event* newNode = cts.at(i)->mEventTo;

            if (newNode->mLevel <= lastNode->mLevel)
                newNode->mLevel = lastNode->mLevel + 1;

            if (!branch.contains(newNode)) {
                branch.append(newNode);
                QVector<QVector<Event*> > nextBranches = getNextBranches(branch, cts[i]->mEventTo);

                for (int j=0; j<nextBranches.size(); ++j)
                    branches.append(nextBranches.at(j));

            } else {
                QStringList evtNames;

                 for (int j=0; j<branch.size(); ++j)
                     evtNames << branch.at(j)->mName;


                evtNames << newNode->mName;

                throw QObject::tr("Circularity found in events model !\rPlease correct this branch :\r") + evtNames.join(" -> ");
            }
        }
    } else {
        branches.append(curBranch);
    }
    return branches;
}

QVector<QVector<Event*> > ModelUtilities::getBranchesFromEvent(Event* start)
{
    QVector<Event*> startBranch;
    start->mLevel = 0;
    startBranch.append(start);

    QVector<QVector<Event*> > nextBranches;
    try {
        nextBranches = getNextBranches(startBranch, start);
    } catch(QString error){
        throw std::move(error);
    }

    return nextBranches;
}


QVector<QVector<Event*> > ModelUtilities::getAllEventsBranches(const QList<Event*>& events)
{
    QVector<QVector<Event*> > branches;

    // ----------------------------------------
    //  Put all events level to 0 and
    //  store events at start of branches (= not having constraint backward)
    // ----------------------------------------
    QVector<Event*> starts;
    for ( auto && event : events) {
        event->mLevel = 0;
        if (event->mConstraintsBwd.size() == 0)
            starts.append(event);
    }

    if (starts.size() == 0 && events.size() != 0)
        throw QObject::tr("Circularity found in events model !");

    else {
        for (int i=0; i<starts.size(); ++i) {
            QVector<QVector<Event*> > eventBranches;
            try {
                eventBranches = getBranchesFromEvent(starts[i]);
            } catch(QString error) {
                throw std::move(error);
            }
            for (int j=0; j<eventBranches.size(); ++j)
                branches.append(eventBranches[j]);
        }
    }
    return branches;
}




// Phases Branches
QVector<QVector<Phase*> > ModelUtilities::getNextBranches(const QVector<Phase*>& curBranch, Phase* lastNode, const double gammaSum, const double maxLength)
{
    QVector<QVector<Phase*> > branches;
    QList<PhaseConstraint*> cts = lastNode->mConstraintsFwd;
    if (cts.size() > 0) {
        for (int i=0; i<cts.size(); ++i) {
            QVector<Phase*> branch = curBranch;
            Phase* newNode = cts[i]->mPhaseTo;

            double gamma = gammaSum;
            if (cts[i]->mGammaType == PhaseConstraint::eGammaFixed)
                gamma += cts[i]->mGammaFixed;

            else if (cts[i]->mGammaType == PhaseConstraint::eGammaRange)
                gamma += cts[i]->mGammaMin;

            if (gamma < maxLength) {
                if (newNode->mLevel <= lastNode->mLevel)
                    newNode->mLevel = lastNode->mLevel + 1;

                if (!branch.contains(newNode)) {
                    branch.append(newNode);
                    QVector<QVector<Phase*> > nextBranches = getNextBranches(branch, cts[i]->mPhaseTo, gamma, maxLength);
                    for (int j=0; j<nextBranches.size(); ++j)
                        branches.append(nextBranches[j]);
                }
                else {
                    QStringList names;
                    for (int j=0; j<branch.size(); ++j)
                        names << branch[j]->mName;
                    names << newNode->mName;

                    throw QObject::tr("Circularity found in phases model !\rPlease correct this branch :\r") + names.join(" -> ");
                }
            }
            else {
                QStringList names;
                for (int j=0; j<curBranch.size(); ++j)
                    names << curBranch[j]->mName;
                names << newNode->mName;
                throw QObject::tr("Phases branch too long :\r") + names.join(" -> ");
            }
        }
    }
    else
        branches.append(curBranch);

    return branches;
}

QVector<QVector<Phase*> > ModelUtilities::getBranchesFromPhase(Phase* start, const double maxLength)
{
    Q_ASSERT(start);
    QVector<Phase*> startBranch;
    start->mLevel = 0;
    startBranch.append(start);

    QVector<QVector<Phase*> > nextBranches;
    try {
        nextBranches = getNextBranches(startBranch, start, 0, maxLength);
    } catch(QString error) {
        throw std::move(error);
    }

    return nextBranches;
}


QVector<QVector<Phase*> > ModelUtilities::getAllPhasesBranches(const QList<Phase*>& phases, const double maxLength)
{
    QVector<QVector<Phase*> > branches;

    QVector<Phase*> starts;
    for (int i=0; i<phases.size(); ++i) {
        phases[i]->mLevel = 0;
        if (phases[i]->mConstraintsBwd.size() == 0)
            starts.append(phases[i]);
    }
    if (starts.size() == 0 && phases.size() != 0)
        throw QObject::tr("Circularity found in phases model !");

    for (int i=0; i<starts.size(); ++i) {
        QVector<QVector<Phase*> > phaseBranches;
        try {
            phaseBranches = getBranchesFromPhase(starts[i], maxLength);
        } catch (QString error){
            throw std::move(error);
        }
        for (int j=0; j<phaseBranches.size(); ++j)
            branches.append(phaseBranches[j]);
    }
    return branches;
}


//Sort events by level
QVector<Event*> ModelUtilities::sortEventsByLevel(const QList<Event*>& events)
{
    int numSorted = 0;
    int curLevel = 0;
    QVector<Event*> results;

    while (numSorted < events.size()) {
        for (int i=0; i<events.size(); ++i) {
            if (events[i]->mLevel == curLevel) {
                results.append(events[i]);
                ++numSorted;
            }
        }
        ++curLevel;
    }
    return results;
}

QVector<Phase*> ModelUtilities::sortPhasesByLevel(const QList<Phase*>& phases)
{
    int numSorted = 0;
    int curLevel = 0;
    QVector<Phase*> results;

    while (numSorted < phases.size()) {
        for (int i=0; i<phases.size(); ++i) {
            if (phases[i]->mLevel == curLevel) {
                results.append(phases[i]);
                ++numSorted;
            }
        }
        ++curLevel;
    }
    return results;
}

/**
 * @brief ModelUtilities::unsortEvents We adapte The modern version of the Fisher–Yates shuffle
 * more : https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle?oldid=636272393#Modern_method
 * @param events
 * @return
 */
QVector<Event*> ModelUtilities::unsortEvents(const QList<Event*>& events)
{
    QVector<Event*> results(events.toVector());

    for (int i=results.size()-1; i>0; --i)
        std::swap (results[i], results[Generator::randomUniformInt(0, i)]);

    return results;
}

QString ModelUtilities::dateResultsText(const Date* d, const Model* model, const bool forCSV)
{
    Q_ASSERT(d);
    QString text;
    const QString nl = "\r";

    text += QObject::tr("Data : %1").arg(d->mName) + nl + nl;
    text += d->mTheta.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV) ;

    if (model) {
        short position = ModelUtilities::HPDOutsideSudyPeriod(d->mTheta.mHPD, model);
        switch (position) {
            case -1:
                text += QObject::tr("Solutions exist under study period");
                break;
            case +1:
                text += QObject::tr("Solutions exist over study period");
                break;
            case +2:
                text += QObject::tr("Solutions exist outside study period");
                break;
            default:
                break;
        }
     }
    text += nl + nl;
    text += QObject::tr("Posterior Std. Deviation") + nl;
    if (forCSV)
        text += d->mSigma.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), nullptr, forCSV);
    else
         text += d->mSigma.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), nullptr, forCSV);

    return text;
}

QString ModelUtilities::eventResultsText(const Event* e, bool withDates, const Model* model, const bool forCSV)
{
    Q_ASSERT(e);
    QString text;
    const QString nl = "\r";

    if (e->mType == Event::eKnown) {
        text += QObject::tr("Bound : %1").arg(e->mName) + nl;
        text += e->mTheta.resultsString( nl, "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV);

        text += nl+"----------------------"+nl;
    }
    else  {
        text += QObject::tr("Event : %1").arg(e->mName) + nl;
        text += e->mTheta.resultsString( nl,"", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV);

        if (withDates) {
            text += nl + nl;
            text += "----------------------"+nl;
            for (auto && date : e->mDates)
                text += dateResultsText( &(date), model, forCSV) + nl + nl;
        }
    }

    return text;
}

QString ModelUtilities::phaseResultsText(const Phase* p, const bool forCSV)
{
    Q_ASSERT(p);
    QString text;
    const QString nl = "\r";

    text += QObject::tr("Phase : %1").arg(p->mName) + nl + nl;

    text += nl + nl;
    text += QObject::tr("Begin (posterior distrib.)") + nl;
    text += p->mAlpha.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV);


    text += nl + nl;
    text += QObject::tr("End (posterior distrib.)") + nl;
    text += p->mBeta.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV);

    if (p->mTimeRange != QPair<double,double>()) {
        text += nl + nl;
        // we suppose it's the same mThreshohdUsed than alpha
        if (forCSV) {
            const QString result = QObject::tr("Phase Time Range") + QString(" ( %1 %) : [ %2 : %3 ] %4").arg(stringForCSV(p->mAlpha.mThresholdUsed, true),
                                                                                                stringForCSV(p->getFormatedTimeRange().first, true),
                                                                                                stringForCSV(p->getFormatedTimeRange().second, true),
                                                                                                DateUtils::getAppSettingsFormatStr());
            text += result;
        } else {
            const QString result = QObject::tr("Phase Time Range") + QString(" ( %1 %) : [ %2 : %3 ] %4").arg(stringForLocal(p->mAlpha.mThresholdUsed, false),
                                                                                                stringForLocal(p->getFormatedTimeRange().first, false),
                                                                                                stringForLocal(p->getFormatedTimeRange().second, false),
                                                                                                DateUtils::getAppSettingsFormatStr());
            text += result;
        }


    }

    return text;
}

QString ModelUtilities::tempoResultsText(const Phase* p, const bool forCSV)
{
    Q_ASSERT(p);
    QString text;
    const QString nl = "\r";

    text += QObject::tr("Phase : %1").arg(p->mName) + nl + nl;

    text += QObject::tr("Duration") + nl;
    text += p->mDuration.resultsString(nl, QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), nullptr , forCSV);

    return text;
}

QString ModelUtilities::constraintResultsText(const PhaseConstraint* p, const bool forCSV)
{
    Q_ASSERT(p);
    QString text;
    const QString nl = "\r";
        text += nl;
        text += textGreen(QObject::tr("Succession : from %1 to %2").arg(p->mPhaseFrom->mName, p->mPhaseTo->mName));

        switch(p->mGammaType) {
            case PhaseConstraint::eGammaFixed :
                text += textGreen(QObject::tr("Min Hiatus fixed = %1").arg(p->mGammaFixed));
                break;
            case PhaseConstraint::eGammaUnknown :
                text += textGreen(QObject::tr("Min Hiatus unknown")) ;
                break;
            case PhaseConstraint::eGammaRange : // no longer used
                 text += textGreen(QObject::tr("Min Hiatus between %1 and %2").arg(p->mGammaMin, p->mGammaMax));
                 break;
            default:

            break;
        }

        if (p->mTransitionRange != QPair<double,double>()) {
            text += nl;
            // we suppose it's the same mThreshohdUsed than alpha
            if (forCSV) {
                const QString result = textGreen(QObject::tr("Transition Range") + QString(" (%1 %) : [ %2 ; %3 ] %4").arg(stringForCSV(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                                   stringForCSV(p->getFormatedTransitionRange().first),
                                                                                                   stringForCSV(p->getFormatedTransitionRange().second),
                                                                                                   DateUtils::getAppSettingsFormatStr()));

                text += result + nl;
            } else {
                const QString result = textGreen(QObject::tr("Transition Range") + QString(" (%1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                                   stringForLocal(p->getFormatedTransitionRange().first),
                                                                                                   stringForLocal(p->getFormatedTransitionRange().second),
                                                                                                   DateUtils::getAppSettingsFormatStr()));

                text += result + nl;
            }

        }


        if (p->mGapRange != QPair<double,double>()) {
            text += nl;

            if (forCSV) {
                const QString result = textGreen(QObject::tr("Gap Range") + QString(" ( %1 ) : [ %2 ; %3 ]").arg(stringForCSV(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                         stringForCSV(p->getFormatedGapRange().first),
                                                                                         stringForCSV(p->getFormatedGapRange().second) ));

                text += result + nl;
            } else {
                const QString result = textGreen(QObject::tr("Gap Range") + QString(" ( %1 ) : [ %2 ; %3 ]").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                         stringForLocal(p->getFormatedGapRange().first),
                                                                                         stringForLocal(p->getFormatedGapRange().second) ));

                text += result + nl;
            }


        }
    return text;
}




QString ModelUtilities::dateResultsHTML(const Date* d, const Model* model)
{
    Q_ASSERT(d);
    QString text;
    text += line(textBold(textBlack(QObject::tr("Data : %1").arg(d->mName)))) + "<br>";
    text += line(textBold(textBlack(QObject::tr("Posterior calib. date"))));

    if (model) {
        short position = ModelUtilities::HPDOutsideSudyPeriod(d->mTheta.mHPD, model);
        switch (position) {
            case -1:
               text += line( textBold(textRed(QObject::tr("Solutions exist before study period") )) );
                break;
            case +1:
                text += line( textBold(textRed(QObject::tr("Solutions exist after study period"))) );
                break;
            case +2:
                text += line( textBold(textRed(QObject::tr("Solutions exist outside study period"))) );
                break;
            default:
                break;
        }
     }


    text += line(textBlack(d->mTheta.resultsString("<br>", "",DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false))) ;

    text += line("<br>");
    text += line(textBold(textBlack(QObject::tr("Posterior Std. Deviation"))));
    text += line(textBlack(d->mSigma.resultsString()));
    return text;
}

QString ModelUtilities::eventResultsHTML(const Event* e, const bool withDates, const Model* model)
{
    Q_ASSERT(e);
    QString text;
    if (e->mType == Event::eKnown) {
        text += line(textBold(textRed(QObject::tr("Bound : %1").arg(e->mName)))) + "<br>";
        text += line(textBold(textRed(QObject::tr("Posterior bound date"))));
        text += line(textRed(e->mTheta.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));
    }
    else {
        text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->mName)))) + "<br>";
        text += line(textBold(textBlue(QObject::tr("Posterior event date"))));
        text += line(textBlue(e->mTheta.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));
        if (withDates){
            for (auto&& date : e->mDates)
                text += "<br><br>" + dateResultsHTML(&(date), model);
        }
    }
    return text;
}

QString ModelUtilities::phaseResultsHTML(const Phase* p)
{
    Q_ASSERT(p);
    QString text;
//        text += "<hr>"; // useless line
    text += line(textBold(textPurple(QObject::tr("Phase : %1").arg(p->mName))));

    text += "<br>";
    text += line(textBold(textPurple(QObject::tr("Begin (posterior distrib.)"))));
    text += line(textPurple(p->mAlpha.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));

    text += "<br>";
    text += line(textBold(textPurple(QObject::tr("End (posterior distrib.)"))));
    text += line(textPurple(p->mBeta.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));

    if (p->mTimeRange != QPair<double,double>()) {
        text += "<br>";
        // we suppose it's the same mThreshohdUsed than alpha
        const QString result = QObject::tr("Phase Time Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mAlpha.mThresholdUsed),
                                                                                            stringForLocal(p->getFormatedTimeRange().first),
                                                                                            stringForLocal(p->getFormatedTimeRange().second),
                                                                                            DateUtils::getAppSettingsFormatStr());
        text += line(textBold(textPurple(result)));
    }
    return text;
}

QString ModelUtilities::tempoResultsHTML(const Phase* p)
{
    Q_ASSERT(p);
    QString text;
    text += line(textBold(textPurple(QObject::tr("Phase : %1").arg(p->mName))));

    text += "<br>";
    text += line(textBold(textPurple(QObject::tr("Duration (posterior distrib.)"))));
    text += line(textPurple(p->mDuration.resultsString("<br>", QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), nullptr, false)));

    return text;
}

QString ModelUtilities::constraintResultsHTML(const PhaseConstraint* p)
{
    Q_ASSERT(p);
    QString text;
    text += line(textBold(textGreen(QObject::tr("Succession : from %1 to %2").arg(p->mPhaseFrom->mName, p->mPhaseTo->mName))));

    if (p->mTransitionRange != QPair<double,double>()) {
        text += "<br>";
        // we suppose it's the same mThreshohdUsed than alpha
        const QString result = QObject::tr("Transition Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                            stringForLocal(p->getFormatedTransitionRange().first),
                                                                                            stringForLocal(p->getFormatedTransitionRange().second),
                                                                                            DateUtils::getAppSettingsFormatStr());

        text += line(textGreen(result));
    }

    if (p->mGapRange != QPair<double,double>()) {
        QString result;
        if (std::isinf(p->getFormatedGapRange().first) || std::isinf(p->getFormatedGapRange().second))
           result = QObject::tr("No Gap") ;

        else
            result = QObject::tr("Gap Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                           stringForLocal(p->getFormatedGapRange().first),
                                                                           stringForLocal(p->getFormatedGapRange().second),
                                                                           DateUtils::getAppSettingsFormatStr());

        text += line(textGreen(result + "<br>"));
    }

    return text;
}

/**
 * @brief HPDOutsideSudyPeriod
 * @param hpd, the hpd must be send with the good date format
 * @param model
 * @return -1 if there is solution under Study Period; 0 if all solution is inside Sudy Period;
 *  +1 if there is solution over Study Period; +2 if there is both solution under and over the Study Period
 */
short ModelUtilities::HPDOutsideSudyPeriod(const QMap<double, double>& hpd, const Model* model)
{
    Q_ASSERT(model);
    QMap<double, double>::const_iterator iter(hpd.constBegin());
    short answer = 0;
    const double tmin = model->mSettings.getTminFormated();
    const double tmax = model->mSettings.getTmaxFormated();
    // we suppose QMap is sort <
    while (iter != hpd.constEnd()) {
        const double v = iter.value();
        if (v > 0) {
           const double t = iter.key();
           if (t<tmin)
               answer = -1;

           else if (t>tmax && answer == -1)
              return 2;

           else if (t>tmax)
               return 1;

        }

        ++iter;
    }
    return answer;
}
