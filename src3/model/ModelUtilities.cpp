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
#include "EventConstraint.h"
#include "PhaseConstraint.h"
#include "ModelCurve.h"
#include "Project.h"
#include "Bound.h"
#include "PluginAbstract.h"
#include "QtUtilities.h"
#include "Generator.h"

#include <string>
#include <utility>

#include <QObject>

bool sortEvents(Event* e1, Event* e2) {return (e1->mItemY < e2->mItemY);}
bool sortPhases(Phase* p1, Phase* p2) {return (p1->mItemY < p2->mItemY);}


// Obsolete , is replaced by the Date::getWiggleDesc()
QString ModelUtilities::getDeltaText(const Date& date)
{
    QString result;
    const PluginAbstract* plugin = date.mPlugin;
    const QString str = QObject::tr("Wiggle");
    if (plugin&& plugin->wiggleAllowed()) {
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
        for (int i = 0; i < cts.size(); ++i) {
            QVector<Event*> branch = curBranch;
            Event* newNode = cts.at(i)->mEventTo;

            if (newNode->mLevel <= lastNode->mLevel)
                newNode->mLevel = lastNode->mLevel + 1;

            if (!branch.contains(newNode)) {
                branch.append(newNode);
                QVector<QVector<Event*> > nextBranches = getNextBranches(branch, cts[i]->mEventTo);

                for (int j = 0; j < nextBranches.size(); ++j)
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
    for (auto&& event : events) {
        event->mLevel = 0;
        if (event->mConstraintsBwd.size() == 0)
            starts.append(event);
    }

    if (starts.size() == 0 && events.size() != 0)
        throw QObject::tr("Circularity found in events model !");

    else {
        for (int i = 0; i < starts.size(); ++i) {
            QVector<QVector<Event*> > eventBranches;
            try {
                eventBranches = getBranchesFromEvent(starts[i]);
            } catch(QString error) {
                throw std::move(error);
            }
            for (int j = 0; j < eventBranches.size(); ++j)
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
        for (int i = 0; i < cts.size(); ++i) {
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
                    for (int j = 0; j < nextBranches.size(); ++j)
                        branches.append(nextBranches[j]);
                }
                else {
                    QStringList names;
                    for (int j = 0; j < branch.size(); ++j)
                        names << branch[j]->mName;
                    names << newNode->mName;

                    throw QObject::tr("Circularity found in phases model !\rPlease correct this branch :\r") + names.join(" -> ");
                }
            }
            else {
                QStringList names;
                for (int j = 0; j < curBranch.size(); ++j)
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
    for (int i = 0; i < phases.size(); ++i) {
        phases[i]->mLevel = 0;
        if (phases[i]->mConstraintsBwd.size() == 0)
            starts.append(phases[i]);
    }
    if (starts.size() == 0 && phases.size() != 0)
        throw QObject::tr("Circularity found in phases model !");

    for (int i = 0; i < starts.size(); ++i) {
        QVector<QVector<Phase*> > phaseBranches;
        try {
            phaseBranches = getBranchesFromPhase(starts[i], maxLength);
        } catch (QString error){
            throw std::move(error);
        }
        for (int j = 0; j < phaseBranches.size(); ++j)
            branches.append(phaseBranches[j]);
    }
    return branches;
}


//Sort events by level
// Obsolete
/*
QVector<Event*> ModelUtilities::sortEventsByLevel(const QList<Event*>& events)
{
    int numSorted = 0;
    int curLevel = 0;
    QVector<Event*> results;

    while (numSorted < events.size()) {
        for (int i = 0; i < events.size(); ++i) {
            if (events[i]->mLevel == curLevel) {
                results.append(events[i]);
                ++numSorted;
            }
        }
        ++curLevel;
    }
    return results;
}
*/
/*
// Obsolete
QVector<Phase*> ModelUtilities::sortPhasesByLevel(const QList<Phase*>& phases)
{
    int numSorted = 0;
    int curLevel = 0;
    QVector<Phase*> results;

    while (numSorted < phases.size()) {
        for (int i = 0; i < phases.size(); ++i) {
            if (phases[i]->mLevel == curLevel) {
                results.append(phases[i]);
                ++numSorted;
            }
        }
        ++curLevel;
    }
    return results;
}
*/

/**
 * @brief ModelUtilities::unsortEvents We adapte The modern version of the Fisher–Yates shuffle
 * more : https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle?oldid=636272393#Modern_method
 * https://en.cppreference.com/w/cpp/algorithm/random_shuffle
 * @param events
 * @return
 */
QVector<Event*> ModelUtilities::unsortEvents(const QList<Event*>& events)
{
    QVector<Event*> results(events.toVector());
   for (int i = results.size()-1; i > 0; --i){
        std::swap(results[i], results[Generator::randomUniformInt(0, i)]);
    }
    return results;
}

QString ModelUtilities::dateResultsText(const Date* d, const Model* model, const bool forCSV)
{
    Q_ASSERT(d);
    QString text;
    const QString nl = "\r";

    text += QObject::tr("Data : %1").arg(d->mName) + nl + nl;
    text += QObject::tr("Posterior calib. date") + nl;
    text += d->mTi.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV) ;

    if (model) {
        short position = ModelUtilities::HPDOutsideSudyPeriod(d->mTi.mHPD, model);
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
    return text;
}

QString ModelUtilities::sigmaTiResultsText(const Date* d, const bool forCSV)
{
    Q_ASSERT(d);
    const QString nl = "\r";

    QString text = QObject::tr("Data : %1").arg(d->mName) + nl + nl;

    text += QObject::tr("Posterior Std ti") + nl;
    if (forCSV)
        text += d->mSigmaTi.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), nullptr, forCSV);
    else
        text += d->mSigmaTi.resultsString(nl, "", DateUtils::getAppSettingsFormatStr(), nullptr, forCSV);

    return text;
}

QString ModelUtilities::eventResultsText(const Event* e, bool withDates, const Model* model, const bool forCSV)
{
    Q_ASSERT(e);
    QString text;
    const QString nl = "\r";

    if (e->mType == Event::eBound) {
        text += QObject::tr("Bound : %1").arg(e->mName) + nl;
        text += QObject::tr("Posterior Bound Date") + nl;
        text += e->mTheta.resultsString( nl, "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV);

        if (!e->mVG.mAllAccepts->isEmpty()) {
           text +=  nl;
           text += QObject::tr("Posterior Std G") + nl;
           text += e->mVG.resultsString("<br>", "", nullptr, nullptr, false) + nl;
       }
        text += nl+"----------------------"+nl;
    }
    else  {
        text += QObject::tr("Event : %1").arg(e->mName) + nl;
        text += QObject::tr("Posterior Event Date") + nl;
        text += e->mTheta.resultsString( nl,"", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, forCSV);
        if (!e->mVG.mAllAccepts->isEmpty()) {
           text +=  nl;
           text += QObject::tr("Posterior Std G") + nl;
           text += e->mVG.resultsString("<br>", "", nullptr, nullptr, false) + nl;
       }
        if (withDates) {
            text += nl + nl;
            text += "----------------------"+nl;
            for (auto && date : e->mDates)
                text += dateResultsText( &(date), model, forCSV) + nl + nl;
        }
    }

    return text;
}

QString ModelUtilities::VgResultsText(const Event* e)
{
    Q_ASSERT(e);
    QString text;
    const QString nl = "\r";

    if (!e->mVG.mAllAccepts->isEmpty()) {
        if (e->mType == Event::eBound) {
            text += QObject::tr("Bound : %1").arg(e->mName);
        } else  {
            text += QObject::tr("Event : %1").arg(e->mName);
        }

        //text +=  nl;
        text += QObject::tr("Posterior Std G") + nl;
        text += e->mVG.resultsString("<br>", "", nullptr, nullptr, false) + nl;

        text += nl+"----------------------"+nl;
    }

    return text;
}

QString ModelUtilities::phaseResultsText(const Phase* p, const bool forCSV)
{
    const QString nl = "\r";

    QString text = QObject::tr("Phase : %1").arg(p->mName) + nl ;
    text += QObject::tr("Number of Events : %1").arg(p->mEvents.size()) + nl;

    text += nl ;
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

QString ModelUtilities::durationResultsText(const Phase* p, const bool forCSV)
{
    const QString nl = "\r";

    QString text = QObject::tr("Phase : %1").arg(p->mName) + nl ;
    text += QObject::tr("Number of Events : %1").arg(p->mEvents.size()) + nl;

    text += QObject::tr("Duration (posterior distrib.)") + nl;
    text += p->mDuration.resultsString(nl, QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), nullptr , forCSV);

    return text;
}

QString ModelUtilities::tempoResultsText(const Phase* p)
{
    const QString nl = "\r";

    QString text = QObject::tr("Phase : %1").arg(p->mName) + nl ;

    text += QObject::tr("Number of Events : %1").arg(p->mEvents.size()) + nl;

    return text;
}

QString ModelUtilities::activityResultsText(const Phase* p, const bool forCSV)
{
    const QString nl = "\r";

    QString text = QObject::tr("Phase : %1").arg(p->mName) + nl ;
    text += QObject::tr("Number of Events : %1").arg(p->mEvents.size()) + nl;

    text += QObject::tr("Duration (posterior distrib.)") + nl;
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
        text += QObject::tr("Min Hiatus fixed = %1").arg(p->mGammaFixed);
        break;
    case PhaseConstraint::eGammaUnknown :
        text += QObject::tr("Min Hiatus unknown") ;
        break;
    case PhaseConstraint::eGammaRange : // no longer used
        text += QObject::tr("Min Hiatus between %1 and %2").arg(p->mGammaMin, p->mGammaMax);
        break;
    default:

        break;
    }

    if (p->mTransitionRange != QPair<double,double>()) {
        text += nl;
        // we suppose it's the same mThreshohdUsed than alpha
        if (forCSV) {
            const QString result = QObject::tr("Transition Range") + QString(" (%1 %) : [ %2 ; %3 ] %4").arg(stringForCSV(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                                                       stringForCSV(p->getFormatedTransitionRange().first),
                                                                                                                       stringForCSV(p->getFormatedTransitionRange().second),
                                                                                                                       DateUtils::getAppSettingsFormatStr());

            text += result + nl;
        } else {
            const QString result = QObject::tr("Transition Range") + QString(" (%1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                                                       stringForLocal(p->getFormatedTransitionRange().first),
                                                                                                                       stringForLocal(p->getFormatedTransitionRange().second),
                                                                                                                       DateUtils::getAppSettingsFormatStr());

            text += result + nl;
        }

    }


    if (p->mGapRange != QPair<double,double>()) {
        text += nl;
        if (std::isinf(p->getFormatedGapRange().first) || std::isinf(p->getFormatedGapRange().second))
           text += QObject::tr("No Gap") ;

        else
        if (forCSV) {
            const QString result = QObject::tr("Gap Range") + QString(" ( %1 ) : [ %2 ; %3 ]").arg(stringForCSV(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                                             stringForCSV(p->getFormatedGapRange().first),
                                                                                                             stringForCSV(p->getFormatedGapRange().second) );

            text += result + nl;
        } else {
            const QString result = QObject::tr("Gap Range") + QString(" ( %1 ) : [ %2 ; %3 ]").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                                             stringForLocal(p->getFormatedGapRange().first),
                                                                                                             stringForLocal(p->getFormatedGapRange().second) );

            text += result + nl;
        }


    }
    return text;
}

QString ModelUtilities::curveResultsText(const ModelCurve* model)
{
    const QString nl = "\r";

    QString text = QObject::tr("Curve") + nl;
  //  text += QObject::tr("- Smoothing Acceptance Rate : %1 %").arg( stringForLocal(100. * model->mLambdaSpline.getCurrentAcceptRate())) + nl;
    text += QObject::tr("Stat on the log10 of Lambda Spline") + nl;
    text += model->mLambdaSpline.resultsString("<br>", "", nullptr, nullptr, false);

    text += QObject::tr("Stat on the sqrt of S02 Vg") + nl;
    text += model->mS02Vg.resultsString("<br>", "", nullptr, nullptr, false);

    if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth) {
        text += nl + QObject::tr("Positive curves accepted with Threshold : %1").arg(stringForLocal(model->mCurveSettings.mThreshold));

        const unsigned requiredCurve = floor(model->mMCMCSettings.mIterPerAquisition / model->mMCMCSettings.mThinningInterval);
        unsigned totalPositvIter = 0;
        unsigned totalPequiredCurve = 0;
        float rate;
        int i = 0;
        for (auto ch : model->mChains) {
            const unsigned positvIter= ch.mRealyAccepted;
            totalPositvIter += positvIter;
            const unsigned requiredCurveChain = floor(model->mMCMCSettings.mIterPerAquisition / model->mMCMCSettings.mThinningInterval);
            totalPequiredCurve += requiredCurveChain;
            rate = (float)positvIter/(float)requiredCurve * 100.;
            text += nl + QObject::tr("- Accepted Curves for Chain %1 : %2 / %3 = %4 % ").
                                  arg(QString::number(++i), QString::number(positvIter),
                                      QString::number(requiredCurveChain), stringForLocal(rate) ) ;

        }


        rate  = (float)totalPositvIter/(float)totalPequiredCurve * 100.;
        text += nl + QObject::tr("- Accepted Curves for All Chain : %2 / %3 = %4 % ").
                              arg(QString::number(totalPositvIter),
                                  QString::number(totalPequiredCurve), stringForLocal(rate) ) ;

    }

    return text;
}

QString ModelUtilities::lambdaResultsText(const ModelCurve* model)
{
    const QString nl = "\r";

    //QString text = QObject::tr("Curve") + nl;
    QString text = QObject::tr("Stat on the log10 of Lambda Spline") + nl;
    text += model->mLambdaSpline.resultsString("<br>", "", nullptr, nullptr, false);

    return text;
}

QString ModelUtilities::S02ResultsText(const ModelCurve* model)
{
    const QString nl = "\r";

    //QString text = QObject::tr("Curve") + nl;

    QString text = QObject::tr("Stat on the sqrt of S02 Vg") + nl;
    text += model->mS02Vg.resultsString("<br>", "", nullptr, nullptr, false);

    return text;
}
// HTML Output
QString ModelUtilities::modelDescriptionHTML(const ModelCurve* model)
{
    bool curveModel = model->mProject->isCurve();

    QString log;
    // Study period
    QLocale locale = QLocale();
    log += line(textBold(textBlack(QObject::tr("Prior Study Period : [ %1 : %2 ] %3").arg(locale.toString(model->mSettings.getTminFormated()), locale.toString(model->mSettings.getTmaxFormated()), DateUtils::getAppSettingsFormatStr() ))));
    log += "<hr>";

    int i = 0;
    for (auto&& pEvent : model->mEvents) {
        if (pEvent->type() == Event::eBound) {
            log += line(textRed(QObject::tr("Bound ( %1 / %2 ) : %3 ( %4  phases,  %5 const. back.,  %6 const.fwd.)").arg(QString::number(i+1), QString::number(model->mEvents.size()), pEvent->mName,
                                                                                                               QString::number(pEvent->mPhases.size()),
                                                                                                               QString::number(pEvent->mConstraintsBwd.size()),
                                                                                                               QString::number(pEvent->mConstraintsFwd.size()))));
        } else {
            log += line(textBlue(QObject::tr("Event ( %1 / %2 ) : %3 ( %4 data, %5 phases,  %6 const. back.,  %7 const. fwd.)").arg(QString::number(i+1), QString::number(model->mEvents.size()), pEvent->mName,
                                                                                                                         QString::number(pEvent->mDates.size()),
                                                                                                                         QString::number(pEvent->mPhases.size()),
                                                                                                                         QString::number(pEvent->mConstraintsBwd.size()),
                                                                                                                         QString::number(pEvent->mConstraintsFwd.size()))
                                 + "<br>" + QObject::tr("- Method : %1").arg(MHVariable::getSamplerProposalText(pEvent->mTheta.mSamplerProposal))));
        }

        if (curveModel) {
            if (model->mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector) {
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Declination : %1").arg(stringForLocal(pEvent->mYDec))));
                log += line(textGreen(QObject::tr("- Field : %1 ±  %2").arg(stringForLocal(pEvent->mZField), stringForLocal(pEvent->mS_ZField))));

            } else if (model->mCurveSettings.mProcessType == CurveSettings::eProcessType2D) {
                log += line(textGreen(QObject::tr("- X : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Y : %1 ±  %2").arg(stringForLocal(pEvent->mYDec), stringForLocal(pEvent->mS_Y))));

            } else if (model->mCurveSettings.mProcessType == CurveSettings::eProcessType3D) {
                log += line(textGreen(QObject::tr("- X : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Y : %1 ±  %2")).arg(stringForLocal(pEvent->mYDec), stringForLocal(pEvent->mS_Y)));
                log += line(textGreen(QObject::tr("- Z : %1 ±  %2").arg(stringForLocal(pEvent->mZField), stringForLocal(pEvent->mS_ZField))));

            } else if (model->mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical) {
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Declination : %1").arg(stringForLocal(pEvent->mYDec))));

            }  else  if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth) {
                log += line(textGreen(QObject::tr("- Depth : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));

            } else if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeField) {
                log += line(textGreen(QObject::tr("- Field : %1 ±  %2").arg(stringForLocal(pEvent->mZField), stringForLocal(pEvent->mS_ZField))));

            } else if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeInclination) {
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));

            } else if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeDeclination) {
                log += line(textGreen(QObject::tr("- Declination : %1 ; Inclination %2 ±  %3").arg(stringForLocal(pEvent->mYDec), stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));

            } else if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeOther) {
                log += line(textGreen(QObject::tr("- Measure : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));

            }

        }
        int j(0);
        for (auto&& date : pEvent->mDates) {
            log += "<br>";
            log += line(textBlack(QObject::tr("Data ( %1 / %2 ) : %3").arg(QString::number(j+1), QString::number(pEvent->mDates.size()), date.mName)
                                  + "<br>" + QObject::tr("- Type : %1").arg(date.mPlugin->getName())
                                  + "<br>" + QObject::tr("- Method : %1").arg(MHVariable::getSamplerProposalText(date.mTi.mSamplerProposal))
                                  + "<br>" + QObject::tr("- Params : %1").arg(date.getDesc())));
            ++j;
        }
        log += "<hr>";
        log += "<br>";
        ++i;
    }

    i = 0;
    for (auto &&pPhase : model->mPhases) {
        log += line(textPurple(QObject::tr("Phase ( %1 / %2 ) : %3 ( %4 events, %5 const. back., %6 const. fwd.)").arg(QString::number(i+1), QString::number(model->mPhases.size()), pPhase->mName,
                                                                                                              QString::number(pPhase->mEvents.size()),
                                                                                                              QString::number(pPhase->mConstraintsBwd.size()),
                                                                                                              QString::number(pPhase->mConstraintsFwd.size()))
                               + "<br>" + QObject::tr("- Type : %1").arg(pPhase->getTauTypeText())));
        log += "<br>";

        log += textBlue(QObject::tr("Event")) + " : ";
        for (auto &&pEvent : pPhase->mEvents)
            log += textBlue(pEvent->mName) + " - ";

        log += "<hr>";
        log += "<br>";
        ++i;
    }


    for (auto&& pPhaseConst : model->mPhaseConstraints) {
        log += "<hr>";
        log += line(textBold(textGreen( QObject::tr("Succession from %1 to %2").arg(pPhaseConst->mPhaseFrom->mName, pPhaseConst->mPhaseTo->mName))));

        switch(pPhaseConst->mGammaType) {
            case PhaseConstraint::eGammaFixed :
                log += line(textBold(textGreen( QObject::tr("Min Hiatus fixed = %1").arg(pPhaseConst->mGammaFixed))));
                break;
            case PhaseConstraint::eGammaUnknown :
                log += line(textBold(textGreen( QObject::tr("Min Hiatus unknown") )));
                break;
            case PhaseConstraint::eGammaRange : //no longer used
                 log += line(textBold(textGreen( QObject::tr("Min Hiatus between %1 and %2").arg(pPhaseConst->mGammaMin, pPhaseConst->mGammaMax))));
                break;
            default:
                log += "Hiatus undefined -> ERROR";
            break;
        }

        log += "<hr>";

    }

    if (curveModel) {
        log += line(textBold(textGreen( QObject::tr("Curve Parameters"))));
        switch(model->mCurveSettings.mProcessType) {
        case CurveSettings::eProcessTypeNone :
            log += line(textBold(textGreen( QObject::tr(" - No functionnal link") )));
            break;
        case CurveSettings::eProcessTypeUnivarie :
            log += textGreen( QObject::tr(" - Process : Univariate on "));

            switch(model->mCurveSettings.mVariableType) {
            case CurveSettings::eVariableTypeInclination :
                log += line(textGreen( QObject::tr("Inclination")));
                break;
            case CurveSettings::eVariableTypeDeclination :
                log += line(textGreen( QObject::tr("Declination")));
                break;
            case CurveSettings::eVariableTypeField :
                log += line(textGreen( QObject::tr("Field")));
                break;
            case CurveSettings::eVariableTypeDepth :
                 log += line(textGreen( QObject::tr("Depth")));
                break;
            case CurveSettings::eVariableTypeOther :
                 log += line(textGreen( QObject::tr("Any Measurement")));
                break;
            }

            break;

        case CurveSettings::eProcessType2D :
            log += line(textGreen( QObject::tr(" - Process : 2D")));
            break;
        case CurveSettings::eProcessTypeSpherical :
            log += line(textGreen( QObject::tr(" - Process : Spherical") ));
            break;

        case CurveSettings::eProcessTypeVector :
            log += line(textGreen( QObject::tr(" - Process : Vector")));
            break;

        case CurveSettings::eProcessType3D :
             log += line(textGreen( QObject::tr(" - Process : 3D")));
            break;
        }

        if (model->mCurveSettings.mUseErrMesure) {
            log += line(textGreen( QObject::tr(" - Use Measurement Error")));
        }

        if (model->mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
            log += line(textGreen( QObject::tr(" - Time : Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Time : Fixed on init value")));
        }

        if (model->mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            log += line(textGreen( QObject::tr(" - Variance : Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Variance : Fixed = %1").arg(QString::number(model->mCurveSettings.mVarianceFixed))));
        }

        if (model->mCurveSettings.mUseVarianceIndividual) {
            log += line(textGreen( QObject::tr(" - Use Variance : Individual")));

        }  else {
            log += line(textGreen( QObject::tr(" - Use Variance : Global")));
        }

        if (model->mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {
            log += line(textGreen( QObject::tr(" - Smoothing : Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Smoothing : Fixed = %1").arg(QString::number(model->mCurveSettings.mLambdaSpline))));
        }

    }

    return log;

}


QString ModelUtilities::getMCMCSettingsLog(const Model* model)
{
    QString log;
   // log += "<hr>";
    log += QObject::tr("Number of chain %1").arg(QString::number(model->mMCMCSettings.mNumChains)) +"<br>";
    log += QObject::tr("Number of burn-in iterations : %1").arg(QString::number(model->mMCMCSettings.mIterPerBurn)) + "<br>";
    log += QObject::tr("Number of Max batches : %1").arg(QString::number(model->mMCMCSettings.mMaxBatches)) + "<br>";
    log += QObject::tr("Number of iterations per batches : %1").arg(QString::number(model->mMCMCSettings.mIterPerBatch)) + "<br>";
    log += QObject::tr("Number of running iterations : %1").arg(QString::number(model->mMCMCSettings.mIterPerAquisition)) + "<br>";
    log += QObject::tr("Thinning Interval : %1").arg(QString::number(model->mMCMCSettings.mThinningInterval)) + "<br>";
    log += QObject::tr("Mixing level : %1").arg(QString::number(model->mMCMCSettings.mMixingLevel)) + "<br>";



    return log;
}

QString ModelUtilities::modelStateDescriptionHTML(const ModelCurve* model, QString stateDescript)
{
    bool curveModel = model->mProject->isCurve();

    int i = 0;
    QString HTMLText = stateDescript;
    HTMLText += "<hr>";
    HTMLText += textBold("Events  (with their data)");
    for (auto& event : model->mEvents) {
        ++i;
        HTMLText += "<hr><br>";

        if (event->type() == Event::eBound) {
             const Bound* bound = dynamic_cast<const Bound*>(event);
            if (bound) {
                HTMLText += line(textRed(QObject::tr("Bound ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), bound->mName)));
                HTMLText += line(textRed(QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(bound->mTheta.mX), DateUtils::getAppSettingsFormatStr())));
            }

        }  else {
            HTMLText += line(textBlue(QObject::tr("Event ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), event->mName)));
            HTMLText += line(textBlue(QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(event->mTheta.mX), DateUtils::getAppSettingsFormatStr()))); 
            if (event->mTheta.mLastAccepts.size()>2 && event->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                HTMLText += line(textBlue(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(event->mTheta.getCurrentAcceptRate()*100.), MHVariable::getSamplerProposalText(event->mTheta.mSamplerProposal))));
                HTMLText += line(textBlue(QObject::tr(" - Sigma_MH on Theta : %1").arg(stringForLocal(event->mTheta.mSigmaMH))));
            }

            HTMLText += line(textBlue(QObject::tr(" - S02 : %1").arg(stringForLocal(event->mS02))));
        }

        if (curveModel) {
            HTMLText += line(textGreen(QObject::tr(" - Variance on G : %1").arg(stringForLocal(event->mVG.mX))));
            if (event->mVG.mLastAccepts.size()>2  && event->mVG.mSamplerProposal!= MHVariable::eFixe) {
                HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(event->mVG.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(event->mVG.mSamplerProposal))));
                HTMLText += line(textGreen(QObject::tr(" - Sigma_MH on Variance on G : %1").arg(stringForLocal(event->mVG.mSigmaMH))));
            }

            // Recherche indice de l'event dans la liste de spline, car les events sont réordonnés
            int thetaIdx;
            const MCMCSpline& spline =  model->mSplinesTrace.back();
            for (thetaIdx=0; thetaIdx < model->mEvents.size(); thetaIdx++) {
                if ( spline.splineX.vecThetaEvents.at(thetaIdx) == event->mTheta.mX)
                    break;
            }

            if (model->mCurveSettings.mProcessType != CurveSettings::eProcessTypeNone) {
                if (model->mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
                    if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth)
                        HTMLText += line(textGreen(QObject::tr(" - Depth thresHold : %1").arg(stringForLocal(model->mCurveSettings.mThreshold))));

                    HTMLText += line(textGreen(QObject::tr(" - G : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));

                } else {
                    HTMLText += line(textGreen(QObject::tr(" - Gx : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));
                    if (spline.splineY.vecG.size() != 0)
                        HTMLText += line(textGreen(QObject::tr(" - Gy : %1").arg(stringForLocal(spline.splineY.vecG.at(thetaIdx)))));
                    if (spline.splineZ.vecG.size() != 0)
                        HTMLText += line(textGreen(QObject::tr(" - Gz : %1").arg(stringForLocal(spline.splineZ.vecG.at(thetaIdx)))));
                }
            }
        }
        int j = 0;
        for (auto& date : event->mDates) {
            ++j;
            HTMLText += "<br>";

            HTMLText += line(textBlack(QObject::tr("Data ( %1 / %2 ) : %3").arg(QString::number(j), QString::number(event->mDates.size()), date.mName)));
            HTMLText += line(textBlack(QObject::tr(" - ti : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(date.mTi.mX), DateUtils::getAppSettingsFormatStr())));
            if (date.mTi.mSamplerProposal == MHVariable::eMHSymGaussAdapt) {
                if (date.mTi.mLastAccepts.size()>2) {
                    HTMLText += line(textBlack(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(date.mTi.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(date.mTi.mSamplerProposal))));
                }
                HTMLText += line(textBlack(QObject::tr(" - Sigma_MH on ti : %1").arg(stringForLocal(date.mTi.mSigmaMH))));
            }

            HTMLText += line(textBlack(QObject::tr(" - Sigma_i : %1").arg(stringForLocal(date.mSigmaTi.mX))));
            if (date.mSigmaTi.mLastAccepts.size()>2) {
                HTMLText += line(textBlack(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(date.mSigmaTi.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(date.mSigmaTi.mSamplerProposal))));
            }
            HTMLText += line(textBlack(QObject::tr(" - Sigma_MH on Sigma_i : %1").arg(stringForLocal(date.mSigmaTi.mSigmaMH))));
            if (date.mDeltaType != Date::eDeltaNone)
                HTMLText += line(textBlack(QObject::tr(" - Delta_i : %1").arg(stringForLocal(date.mDelta))));

        }
    }

    if (model->mPhases.size() > 0) {
        HTMLText += "<hr>";
        HTMLText += textBold(QObject::tr("Phases"));
        HTMLText += "<hr>";

        int i = 0;
        for (auto& phase : model->mPhases) {
            ++i;
            HTMLText += "<br>";
            HTMLText += line(textPurple(QObject::tr("Phase ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mPhases.size()), phase->mName)));
            HTMLText += line(textPurple(QObject::tr(" - Begin : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mAlpha.mX), DateUtils::getAppSettingsFormatStr())));
            HTMLText += line(textPurple(QObject::tr(" - End : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mBeta.mX), DateUtils::getAppSettingsFormatStr())));
            HTMLText += line(textPurple(QObject::tr(" - Tau : %1").arg(stringForLocal(phase->mTau.mX))));
        }
    }

    if (model->mPhaseConstraints.size() > 0) {
        HTMLText += "<hr>";
        HTMLText += textBold(textGreen(QObject::tr("Phases Constraints"))) ;
        HTMLText += "<hr>";

        int i = 0;
        for (auto& constraint : model->mPhaseConstraints) {
            ++i;
            HTMLText += "<br>";
            HTMLText += line(textGreen(QObject::tr("Succession ( %1 / %2) : from %3 to %4").arg(QString::number(i), QString::number(model->mPhaseConstraints.size()),constraint->mPhaseFrom->mName, constraint->mPhaseTo->mName)));
            HTMLText += line(textGreen(QObject::tr(" - Gamma : %1").arg(stringForLocal(constraint->mGamma))));
        }
    }

    if (curveModel) {
        HTMLText += "<hr>";
        HTMLText += textBold(textGreen(QObject::tr("Curve"))) ;
        HTMLText += "<hr>";
        HTMLText +=  line(textGreen(QObject::tr("Log10 Smoothing : %1").arg(QLocale().toString(log10(model->mLambdaSpline.mX), 'G', 2))));
        if (model->mLambdaSpline.mLastAccepts.size()>2  && model->mLambdaSpline.mSamplerProposal!= MHVariable::eFixe) {
            HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(model->mLambdaSpline.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(model->mLambdaSpline.mSamplerProposal))));
            HTMLText +=  line(textGreen(QObject::tr(" - Sigma_MH on Smoothing : %1").arg(stringForLocal(model->mLambdaSpline.mSigmaMH))));
        }
        HTMLText += "<hr>";
        HTMLText +=  line(textGreen(QObject::tr("sqrt S02 Vg : %1").arg(QLocale().toString(log10(model->mS02Vg.mX), 'G', 2))));
        if (model->mS02Vg.mLastAccepts.size()>2) {
            HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(model->mS02Vg.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(model->mS02Vg.mSamplerProposal))));
            HTMLText +=  line(textGreen(QObject::tr(" - Sigma_MH on S02 Vg : %1").arg(stringForLocal(model->mS02Vg.mSigmaMH))));
        }
    }

    return HTMLText;
}

QString ModelUtilities::modelStateDescriptionText(const ModelCurve *model, QString stateDescript)
{
    bool curveModel = model->mProject->isCurve();
    const QString nl = "\r";
    int i = 0;
    QString text = stateDescript;
    text += nl;
    text += "Events  (with their data)";
    for (auto& event : model->mEvents) {
        ++i;
        text += nl;

        if (event->type() == Event::eBound) {
            const Bound* bound = dynamic_cast<const Bound*>(event);
            if (bound) {
                text += QObject::tr("Bound ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), bound->mName);
                text += QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(bound->mTheta.mX), DateUtils::getAppSettingsFormatStr());
            }

        }  else {
            text += QObject::tr("Event ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), event->mName);
            text += QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(event->mTheta.mX), DateUtils::getAppSettingsFormatStr());

            if (event->mTheta.mLastAccepts.size()>2 && event->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                text += QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(event->mTheta.getCurrentAcceptRate()*100.), MHVariable::getSamplerProposalText(event->mTheta.mSamplerProposal));
                text += QObject::tr(" - Sigma_MH on Theta : %1").arg(stringForLocal(event->mTheta.mSigmaMH));
            }

            text += QObject::tr(" - S02 : %1").arg(stringForLocal(event->mS02));
        }

        if (curveModel) {
            text += QObject::tr(" - Variance on G : %1").arg(stringForLocal(event->mVG.mX));
            if (event->mVG.mLastAccepts.size()>2  && event->mVG.mSamplerProposal!= MHVariable::eFixe) {
                text += QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(event->mVG.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(event->mVG.mSamplerProposal));
            }
            text += QObject::tr(" - Sigma_MH on Variance on G : %1").arg(stringForLocal(event->mVG.mSigmaMH));


            // Recherche indice de l'event dans la liste de spline, car les events sont réordonnés
            int thetaIdx;
            const MCMCSpline& spline =  model->mSplinesTrace.back();
            for (thetaIdx=0; thetaIdx < model->mEvents.size(); thetaIdx++) {
                if ( spline.splineX.vecThetaEvents.at(thetaIdx) == event->mTheta.mX)
                    break;
            }

            if (model->mCurveSettings.mProcessType == CurveSettings::eProcessTypeUnivarie) {
                if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth)
                    text += QObject::tr(" - Depth thresHold : %1").arg(stringForLocal(model->mCurveSettings.mThreshold));

                text += QObject::tr(" - G : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)));

            } else {
                text += QObject::tr(" - Gx : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)));
                if (spline.splineY.vecG.size() != 0)
                    text += QObject::tr(" - Gy : %1").arg(stringForLocal(spline.splineY.vecG.at(thetaIdx)));
                if (spline.splineZ.vecG.size() != 0)
                    text += QObject::tr(" - Gz : %1").arg(stringForLocal(spline.splineZ.vecG.at(thetaIdx)));
            }
        }
        int j = 0;
        for (auto& date : event->mDates) {
            ++j;
            text += nl;

            text += QObject::tr("Data ( %1 / %2 ) : %3").arg(QString::number(j), QString::number(event->mDates.size()), date.mName);
            text += QObject::tr(" - ti : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(date.mTi.mX), DateUtils::getAppSettingsFormatStr());
            if (date.mTi.mSamplerProposal == MHVariable::eMHSymGaussAdapt) {
                if (date.mTi.mLastAccepts.size()>2) {
                    text += QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(date.mTi.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(date.mTi.mSamplerProposal));
                }
                text += QObject::tr(" - Sigma_MH on ti : %1").arg(stringForLocal(date.mTi.mSigmaMH));

            }

            text += QObject::tr(" - Sigma_i : %1").arg(stringForLocal(date.mSigmaTi.mX));
            if (date.mSigmaTi.mLastAccepts.size()>2) {
                text += QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(date.mSigmaTi.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(date.mSigmaTi.mSamplerProposal));
            }
            text += QObject::tr(" - Sigma_MH on Sigma_i : %1").arg(stringForLocal(date.mSigmaTi.mSigmaMH));
            if (date.mDeltaType != Date::eDeltaNone)
                text += QObject::tr(" - Delta_i : %1").arg(stringForLocal(date.mDelta));

        }
    }

    if (model->mPhases.size() > 0) {
        text += "<hr>";
        text += QObject::tr("Phases");
        text += "<hr>";

        int i = 0;
        for (auto& phase : model->mPhases) {
            ++i;
            text += nl;
            text += QObject::tr("Phase ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mPhases.size()), phase->mName);
            text += QObject::tr(" - Begin : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mAlpha.mX), DateUtils::getAppSettingsFormatStr());
            text += (QObject::tr(" - End : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mBeta.mX), DateUtils::getAppSettingsFormatStr()));
            text += QObject::tr(" - Tau : %1").arg(stringForLocal(phase->mTau.mX));
        }
    }

    if (model->mPhaseConstraints.size() > 0) {
        text += nl;
        text += QObject::tr("Phases Constraints") ;
        text += nl;

        int i = 0;
        for (auto& constraint : model->mPhaseConstraints) {
            ++i;
            text += nl;
            text += QObject::tr("Succession ( %1 / %2) : from %3 to %4").arg(QString::number(i), QString::number(model->mPhaseConstraints.size()),constraint->mPhaseFrom->mName, constraint->mPhaseTo->mName);
            text += QObject::tr(" - Gamma : %1").arg(stringForLocal(constraint->mGamma));
        }
    }

    if (curveModel) {
        text += nl;
        text += QObject::tr("Curve") ;
        text += nl;
        text +=  QObject::tr("Log10 Smoothing : %1").arg(QLocale().toString(log10(model->mLambdaSpline.mX), 'G', 2));
        if (model->mLambdaSpline.mLastAccepts.size()>2  && model->mLambdaSpline.mSamplerProposal!= MHVariable::eFixe) {
            text += QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(model->mLambdaSpline.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(model->mLambdaSpline.mSamplerProposal));
            text += QObject::tr(" - Sigma_MH on Smoothing : %1").arg(stringForLocal(model->mLambdaSpline.mSigmaMH));
        }

        text += nl;
        text +=  QObject::tr("sqrt S02 Vg : %1").arg(QLocale().toString(log10(model->mS02Vg.mX), 'G', 2));
        if (model->mS02Vg.mLastAccepts.size()>2) {
            text += QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(model->mS02Vg.getCurrentAcceptRate() *100.), MHVariable::getSamplerProposalText(model->mS02Vg.mSamplerProposal));
            text += QObject::tr(" - Sigma_MH on S02 Vg : %1").arg(stringForLocal(model->mS02Vg.mSigmaMH));
        }
    }

return text;
}

QString ModelUtilities::dateResultsHTML(const Date* d, const Model* model)
{
    Q_ASSERT(d);
    QString text = line(textBold(textBlack(QObject::tr("Data : %1").arg(d->mName)))) + "<br>";
    text += line(textBold(textBlack(QObject::tr("Posterior calib. date"))));

    if (model) {
        short position = ModelUtilities::HPDOutsideSudyPeriod(d->mTi.mHPD, model);
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

    text += line(textBlack(d->mTi.resultsString("<br>", "",DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false))) ;

    return text;
}

QString ModelUtilities::sigmaTiResultsHTML(const Date* d)
{
    Q_ASSERT(d);
    QString text;
    text += line(textBold(textBlack(QObject::tr("Data : %1").arg(d->mName)))) + "<br>";

    text += line(textBold(textBlack(QObject::tr("Posterior Std ti"))));
    text += line(textBlack(d->mSigmaTi.resultsString()));
    return text;
}

QString ModelUtilities::eventResultsHTML(const Event* e, const bool withDates, const Model* model)
{
    Q_ASSERT(e);
    QString text;
    if (e->mType == Event::eBound) {
        text += line(textBold(textRed(QObject::tr("Bound : %1").arg(e->mName)))) + "<br>";
        text += line(textBold(textRed(QObject::tr("Posterior Bound Date"))));
        text += line(textRed(e->mTheta.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));
/*
        if (!e->mVG.mAllAccepts->isEmpty()) {
           text +=  "<br>";
           text += line(textBold(textGreen(QObject::tr("Posterior Std gi"))));
           text += line(textGreen(e->mVG.resultsString("<br>", "", nullptr, nullptr, false)));
       }*/
    }
    else {
        text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->mName)))) + "<br>";
        text += line(textBold(textBlue(QObject::tr("Posterior Event Date"))));
        text += line(textBlue(e->mTheta.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));

 /*       if (!e->mVG.mAllAccepts->isEmpty()) {
           text +=  "<br>";
           text += line(textBold(textGreen(QObject::tr("Posterior Std gi"))));
           text += line(textGreen(e->mVG.resultsString("<br>", "", nullptr, nullptr, false)));
       }*/

        if (withDates) {
            for (auto&& date : e->mDates)
                text += "<br><br>" + dateResultsHTML(&(date), model);
        }
    }
    return text;
}

QString ModelUtilities::VgResultsHTML(const Event* e)
{
    Q_ASSERT(e);
    QString text;

    if (!e->mVG.mAllAccepts->isEmpty()) {
        if (e->mType == Event::eBound) {
            text += line(textBold(textRed(QObject::tr("Bound : %1").arg(e->mName))));

         } else {
            text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->mName))));
        }

        text += line(textBold(textGreen(QObject::tr("Posterior Std G"))));
        text += line(textGreen(e->mVG.resultsString("<br>", "", nullptr, nullptr, false)));
    }
    return text;
}

QString ModelUtilities::phaseResultsHTML(const Phase* p)
{
    QString text = line(textBold(textPurple(QObject::tr("Phase : %1").arg(p->mName))));
    text += line(textPurple(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    text += "<br>";
    text += line(textBold(textPurple(QObject::tr("Begin (posterior distrib.)"))));
    text += line(textPurple(p->mAlpha.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));

    text += "<br>";
    text += line(textBold(textPurple(QObject::tr("End (posterior distrib.)"))));
    text += line(textPurple(p->mBeta.resultsString("<br>", "", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false)));

    if (p->mTimeRange != QPair<double, double>()) {
        text += "<br>";
        // we suppose it's the same mThreshohdUsed than alpha
        const QString result = QObject::tr("Phase Time Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mAlpha.mThresholdUsed),
                                                                                            stringForLocal(p->getFormatedTimeRange().first),
                                                                                            stringForLocal(p->getFormatedTimeRange().second),
                                                                                            DateUtils::getAppSettingsFormatStr());
        text += line(textBold(textPurple(result)));
    }
/*
    text += "<br>";
    text += line(textBold(textPurple(QObject::tr("Duration (posterior distrib.)"))));
    text += line(textPurple(p->mDuration.resultsString("<br>", QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), nullptr, false)));
*/
    return text;
}

QString ModelUtilities::durationResultsHTML(const Phase* p)
{
   QString text = line(textBold(textPurple(QObject::tr("Phase : %1").arg(p->mName))));
   text += line(textPurple(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

   text += "<br>";
   text += line(textBold(textPurple(QObject::tr("Duration (posterior distrib.)"))));
   text += line(textPurple(p->mDuration.resultsString("<br>", QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), nullptr, false)));

   return text;
}

QString ModelUtilities::tempoResultsHTML(const Phase* p)
{
    Q_ASSERT(p);
    QString text = line(textBold(textPurple(QObject::tr("Phase : %1").arg(p->mName))));
    text += line(textPurple(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    return text;
}

QString ModelUtilities::activityResultsHTML(const Phase* p)
{
    Q_ASSERT(p);
    QString text = line(textBold(textPurple(QObject::tr("Phase : %1").arg(p->mName))));
    text += line(textPurple(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    text += "<br>";
    text += line(textBold(textPurple(QObject::tr("Activity"))));

    double t1 = DateUtils::convertToAppSettingsFormat(p->mActivityValueStack.at("t_min").mValue);
    double t2 = DateUtils::convertToAppSettingsFormat(p->mActivityValueStack.at("t_max").mValue);

    if (t1>t2)
        std::swap(t1, t2);
    text += line(textPurple("<i>" + QObject::tr("Trace Stat.")  + "</i>"));
    text += line(textPurple("Theta min = " + stringForLocal(t1) + " " + DateUtils::getAppSettingsFormatStr()));
    text += line(textPurple("Theta max = " + stringForLocal(t2) + " " + DateUtils::getAppSettingsFormatStr()));

    t1 = DateUtils::convertToAppSettingsFormat(p->mActivityValueStack.at("minQ25").mValue);
    t2 = DateUtils::convertToAppSettingsFormat(p->mActivityValueStack.at("maxQ975").mValue);
    if (t1>t2)
        std::swap(t1, t2);
    text += line(textPurple("Min Q<sub>2,5</sub> = " + stringForLocal(t1) + " " + DateUtils::getAppSettingsFormatStr()));
    text += line(textPurple("Max Q<sub>97,5</sub> = " + stringForLocal(t2) + " " + DateUtils::getAppSettingsFormatStr()));

    text += "<hr>";
    text += line(textBold(textPurple(QObject::tr("Unif. Predict."))));
    t1 = DateUtils::convertToAppSettingsFormat(p->mActivityValueStack.at("t_min_R").mValue);
    t2 = DateUtils::convertToAppSettingsFormat(p->mActivityValueStack.at("t_max_R").mValue);

    if (t1>t2)
        std::swap(t1, t2);
    text += line(textPurple("Unif. Theta min = " + stringForLocal(t1) + " " + DateUtils::getAppSettingsFormatStr()));
    text += line(textPurple("Unif. Theta max = " + stringForLocal(t2) + " " + DateUtils::getAppSettingsFormatStr()));

    QString textValue = stringForLocal(p->mActivityValueStack.at("R_etendue").mValue);
    text += line(textPurple("Unif. Span = " + textValue ));

    textValue = stringForLocal(p->mActivityValueStack.at("Significance Score").mValue);
    text += line(textPurple("Significance Score = " + textValue));

    return text;
}

QString ModelUtilities::constraintResultsHTML(const PhaseConstraint* p)
{
    QString text = line(textBold(textGreen(QObject::tr("Succession : from %1 to %2").arg(p->mPhaseFrom->mName, p->mPhaseTo->mName))));

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
        text += "<br>";
        // we suppose it's the same mThreshohdUsed than alpha
        const QString result = textGreen(QObject::tr("Transition Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                            stringForLocal(p->getFormatedTransitionRange().first),
                                                                                            stringForLocal(p->getFormatedTransitionRange().second),
                                                                                            DateUtils::getAppSettingsFormatStr()));

        text += line(textGreen(result));
    }

    if (p->mGapRange != QPair<double,double>()) {
        QString result;
        if (std::isinf(p->getFormatedGapRange().first) || std::isinf(p->getFormatedGapRange().second))
           result = textGreen(QObject::tr("No Gap") );

        else
            result = textGreen(QObject::tr("Gap Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                           stringForLocal(p->getFormatedGapRange().first),
                                                                           stringForLocal(p->getFormatedGapRange().second),
                                                                           DateUtils::getAppSettingsFormatStr()));

        text += line(textGreen(result + "<br>"));
    }

    return text;
}

QString ModelUtilities::curveResultsHTML(const ModelCurve* model)
{
    QString text;

    text += line( textBold(textGreen(QObject::tr("Curve"))) );
    //text += "<br>";
    text += line(textGreen(QObject::tr(" - Number of Ref. points = %1").arg(model->mEvents.size())));

    text += line(textGreen(QObject::tr("- Mean of the log10 of Lambda Spline = %1").arg(stringForLocal(model->mLambdaSpline.mResults.funcAnalysis.mean))));
    text += line(textGreen(QObject::tr("- Mean of the sqrt of S02 Vg = %1").arg(stringForLocal(model->mS02Vg.mResults.funcAnalysis.mean))));

    if (model->mCurveSettings.mVariableType == CurveSettings::eVariableTypeDepth) {
         text += "<br>" + line( textBold(textGreen(QObject::tr("Positive curves accepted with Threshold : %1").arg(stringForLocal(model->mCurveSettings.mThreshold)))));
        const unsigned requiredCurve = floor(model->mMCMCSettings.mIterPerAquisition / model->mMCMCSettings.mThinningInterval);
        unsigned totalPositvIter = 0;
        unsigned totalPequiredCurve = 0;
        float rate;

        int i = 0;
        for (auto& ch : model->mChains) {
            const unsigned positvIter= ch.mRealyAccepted;
            totalPositvIter += positvIter;
            const unsigned requiredCurveChain = floor(model->mMCMCSettings.mIterPerAquisition / model->mMCMCSettings.mThinningInterval);
            totalPequiredCurve += requiredCurveChain;
            rate = (float)positvIter/(float)requiredCurve * 100.;
            text += line(textGreen(QObject::tr("- Accepted Curves for Chain %1 : %2 / %3 = %4 % ").
                                  arg(QString::number(++i), QString::number(positvIter),
                                      QString::number(requiredCurve), stringForLocal(rate))) );

        }
        rate  = (float)totalPositvIter/(float)totalPequiredCurve * 100.;
        text += line(textGreen( QObject::tr("- Accepted Curves for All Chain : %2 / %3 = %4 % ").
                              arg(QString::number(totalPositvIter),
                                  QString::number(totalPequiredCurve), stringForLocal(rate) ) ));
    }
    text += "<hr>";
    return text;
}

QString ModelUtilities::lambdaResultsHTML(const ModelCurve* model)
{
    QString text = line(textBold(textGreen(QObject::tr("Stat. on the log10 of Lambda Spline"))));
    text += line(textGreen(model->mLambdaSpline.resultsString("<br>", "", nullptr, nullptr, false)));

    return text;
}

QString ModelUtilities::S02ResultsHTML(const ModelCurve* model)
{
    QString text = line(textBold(textGreen(QObject::tr("Stat. on the sqrt of S02 Vg"))));
    text += line(textGreen(model->mS02Vg.resultsString("<br>", "", nullptr, nullptr, false)));

    return text;
}
/**
 * @brief HPDOutsideSudyPeriod
 * @param hpd, the hpd must be send with the good date format
 * @param model
 * @return -1 if there is solution under Study Period; 0 if all solution is inside Study Period;
 *  +1 if there is solution over Study Period; +2 if there is both solution under and over the Study Period
 */
short ModelUtilities::HPDOutsideSudyPeriod(const QMap<double, double>& hpd, const Model* model)
{
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

void sampleInCumulatedRepartition( Event* event, const ProjectSettings &settings, const double min, const double max)
{

    // Création de la cumulé de répartition de date
    // 1 - Search for tmin and tmax, distribution curves, identical to the calibration.
    double unionTmin (+INFINITY);
    double unionTmax (-INFINITY);
    double unionStep (settings.mStep);
    for (auto&& d : event->mDates) {
        if (d.mCalibration != nullptr && !d.mCalibration->mCurve.isEmpty() ) {
            unionTmin = std::min(unionTmin, d.mCalibration->mTmin);
            unionTmax = std::max(unionTmax, d.mCalibration->mTmax);
            unionStep = std::min(unionStep, d.mCalibration->mStep);

        } else {
            unionTmin = settings.mTmin;
            unionTmax = settings.mTmax;
        }

    }
    // 2- Search for the common interval between constraints and calibrations

    /* In ChronoModel 2.0, we initialize the theta uniformly between tmin and tmax possible.
     *  unsortedEvents.at(i)->mTheta.mX = Generator::randomUniform(min, max);
     * Now, we use the cumulative date density distribution function.
     */

    // Calibrated outside the constraints
    // This case must be dissociated in two, the density is on the right or the density is on the left, thus favouring one of the sides.

    if (unionTmax < min) {
        event->mTheta.mX = Generator::gaussByDoubleExp((unionTmax + unionTmin)/2., (unionTmax - unionTmin)/3., min, max);
#ifdef DEBUG
        if (event->mTheta.mX == min)
            qDebug() << "sampleInCumulatedRepartition unionTmax< min and (event->mTheta.mX == min)";
#endif
    } else if (max < unionTmin) {
        event->mTheta.mX = Generator::gaussByDoubleExp((unionTmax + unionTmin)/2., (unionTmax - unionTmin)/3., min, max);

#ifdef DEBUG
        if (event->mTheta.mX == max)
            qDebug() << "sampleInCumulatedRepartition max<unionTmin and (event->mTheta.mX == max)";
#endif
    } else {
        unionTmin = std::max(unionTmin, min);
        unionTmax = std::min(unionTmax, max);

        // 3 - Création de la cumulé des courbes de répartition dans l'intervalle
        QVector<double> unionRepartition (0);
        double tWhile (unionTmin);
        double sumWhile (0.);

        while (tWhile<= unionTmax) {
            sumWhile= 0.;
            for (auto&& d : event->mDates) {
                sumWhile += interpolate_value_from_curve(tWhile, d.mCalibration->mRepartition, d.mCalibration->mTmin, d.mCalibration->mTmax);

            }
            unionRepartition.append(sumWhile);
            tWhile += unionStep;

        }

        /* Given the stratigraphic constraints and the possibility of having dates outside the study period.
         * The maximum of the distribution curve can be different from the number of dates
         * and the minimum can be different from 0.
         */


        const double maxRepartition (unionRepartition.last());
        const double minRepartition (unionRepartition.first());
        if ( (minRepartition!=0. || maxRepartition!= 0.) &&
             (unionRepartition.size()>1)) {

            const double idx = vector_interpolate_idx_for_value(Generator::randomUniform()*(maxRepartition-minRepartition) + minRepartition, unionRepartition);
            event->mTheta.mX = unionTmin + idx * unionStep;
#ifdef DEBUG
        if (event->mTheta.mX == min || event->mTheta.mX == max)
            qDebug() << "sampleInCumulatedRepartition event->mTheta.mX == min || event->mTheta.mX == max";
#endif

        } else {
            event->mTheta.mX = Generator::randomUniform(min, max);
        }

    }
}