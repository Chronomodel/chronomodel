/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include "CalibrationCurve.h"
#include "EventConstraint.h"
#include "PhaseConstraint.h"
#include "ModelCurve.h"
#include "Project.h"
#include "Bound.h"
#include "PluginAbstract.h"
#include "QtUtilities.h"
#include "Generator.h"

#include <QObject>

#include <string>
#include <utility>


bool sortEvents(Event* e1, Event* e2) {return (e1->mItemY < e2->mItemY);}
bool sortPhases(Phase* p1, Phase* p2) {return (p1->mItemY < p2->mItemY);}


// Events Branches
QVector<QVector<Event*> > ModelUtilities::getNextBranches(const QVector<Event*> &curBranch, Event* lastNode)
{
    QVector<QVector<Event*> > branches;
    QList<EventConstraint*> &cts = lastNode->mConstraintsFwd;
    if (cts.size() > 0) {
        for (auto& ct : cts) {
            QVector<Event*> branch = curBranch;
            Event* newNode = ct->mEventTo;

            if (newNode->mLevel <= lastNode->mLevel)
                newNode->mLevel = lastNode->mLevel + 1;

            if (!branch.contains(newNode)) {
                branch.append(newNode);
                QVector<QVector<Event*> > nextBranches = getNextBranches(branch, ct->mEventTo);

                for (auto& nb : nextBranches)
                    branches.append(nb);

            } else {
                QStringList evtNames;

                 for (auto& b : branch)
                     evtNames << b->mName;


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
        for (auto& s : starts) {
            QVector<QVector<Event*> > eventBranches;
            try {
                eventBranches = getBranchesFromEvent(s);
            } catch(QString error) {
                throw std::move(error);
            }
            for (auto& eb : eventBranches)
                branches.append(eb);
        }
    }
    return branches;
}




// Phases Branches
QVector<QVector<Phase*> > ModelUtilities::getNextBranches(const QVector<Phase*> &curBranch, Phase* lastNode, const double gammaSum, const double maxLength)
{
    QVector<QVector<Phase*> > branches;
    QList<PhaseConstraint*> &cts = lastNode->mConstraintsFwd;
    if (cts.size() > 0) {
        for (auto& ct : cts) {
            QVector<Phase*> branch = curBranch;
            Phase* newNode = ct->mPhaseTo;

            double gamma = gammaSum;
            if (ct->mGammaType == PhaseConstraint::eGammaFixed)
                gamma += ct->mGammaFixed;

            else if (ct->mGammaType == PhaseConstraint::eGammaRange)
                gamma += ct->mGammaMin;

            if (gamma < maxLength) {
                if (newNode->mLevel <= lastNode->mLevel)
                    newNode->mLevel = lastNode->mLevel + 1;

                if (!branch.contains(newNode)) {
                    branch.append(newNode);
                    QVector<QVector<Phase*> > nextBranches = getNextBranches(branch, ct->mPhaseTo, gamma, maxLength);
                    for (auto& nb : nextBranches)
                        branches.append(nb);
                }
                else {
                    QStringList names;
                    for (auto& b : branch)
                        names << b->mName;
                    names << newNode->mName;

                    throw QObject::tr("Circularity found in phases model !\rPlease correct this branch :\r") + names.join(" -> ");
                }
            }
            else {
                QStringList names;
                for (auto& c : curBranch)
                    names << c->mName;

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
    for (auto& p : phases) {
        p->mLevel = 0;
        if (p->mConstraintsBwd.size() == 0)
            starts.append(p);
    }
    if (starts.size() == 0 && phases.size() != 0)
        throw QObject::tr("Circularity found in phases model !");

    try {
            for (auto& s : starts) {
                const QVector<QVector<Phase*> > &phaseBranches = getBranchesFromPhase(s, maxLength);
                for (auto& pb : phaseBranches) {
                    branches.append(pb);
                }
            }
    } catch (QString error){
            throw std::move(error);
    }
    return branches;
}


/**
 * @brief ModelUtilities::unsortEvents We adapte The modern version of the Fisher–Yates shuffle
 * more : https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle?oldid=636272393#Modern_method
 * https://en.cppreference.com/w/cpp/algorithm/random_shuffle
 * @param events
 * @return
 */
QList<Event*> ModelUtilities::unsortEvents(const QList<Event*> &events)
{
    QList<Event*> results (events);
    for (int i = results.size()-1; i > 0; --i){
        std::swap(results[i], results[Generator::randomUniformInt(0, i)]);
    }
    return results;
}


QString ModelUtilities::getMCMCSettingsLog(const Model* model)
{
    QString log;

    log += QObject::tr("Number of chain %1").arg(QString::number(model->mMCMCSettings.mNumChains)) + "<br>";
    log += QObject::tr("Number of burn-in iterations : %1").arg(QString::number(model->mMCMCSettings.mIterPerBurn)) + "<br>";
    log += QObject::tr("Number of Max batches : %1").arg(QString::number(model->mMCMCSettings.mMaxBatches)) + "<br>";
    log += QObject::tr("Number of iterations per batches : %1").arg(QString::number(model->mMCMCSettings.mIterPerBatch)) + "<br>";
    log += QObject::tr("Number of running iterations : %1").arg(QString::number(model->mMCMCSettings.mIterPerAquisition)) + "<br>";
    log += QObject::tr("Thinning Interval : %1").arg(QString::number(model->mMCMCSettings.mThinningInterval)) + "<br>";
    log += QObject::tr("Mixing level : %1").arg(QString::number(model->mMCMCSettings.mMixingLevel)) + "<br>";

    return log;
}

#pragma mark Results in HTML format
QString ModelUtilities::modelDescriptionHTML(const ModelCurve* model)
{
    const bool curveModel = model->mProject->isCurve();

    QString log;
    // Study period
    QLocale locale = QLocale();
    log += line(textBold(textBlack(QObject::tr("Prior Study Period : [ %1 : %2 ] %3").arg(locale.toString(model->mSettings.getTminFormated()), locale.toString(model->mSettings.getTmaxFormated()), DateUtils::getAppSettingsFormatStr() ))));
    log += line(textOrange(QObject::tr("Number of Phase  : %1").arg(QString::number(model->mPhases.size()))));
    log += line(textBlue(QObject::tr("Number of Event  : %1").arg(QString::number(model->mEvents.size()))));
    log += "<hr>";

    int i = 0;
    for (auto&& pEvent : model->mEvents) {
        if (pEvent->type() == Event::eBound) {
            auto bound = dynamic_cast<Bound*>(pEvent);
            log += line(textRed(QObject::tr("Bound ( %1 / %2 ) : %3 ( %4  phases,  %5 const. back.,  %6 const.fwd.)").arg(QString::number(i+1), QString::number(model->mEvents.size()), bound->mName,
                                                                                                               QString::number(bound->mPhases.size()),
                                                                                                               QString::number(bound->mConstraintsBwd.size()),
                                                                                                               QString::number(bound->mConstraintsFwd.size()))));
            log += line(textRed(QObject::tr("- Fixed Value : %1 ").arg(stringForLocal(bound->mFixed))));
        } else {
            log += line(textBlue(QObject::tr("Event ( %1 / %2 ) : %3 ( %4 data, %5 phases,  %6 const. back.,  %7 const. fwd.)").arg(QString::number(i+1), QString::number(model->mEvents.size()), pEvent->mName,
                                                                                                                         QString::number(pEvent->mDates.size()),
                                                                                                                         QString::number(pEvent->mPhases.size()),
                                                                                                                         QString::number(pEvent->mConstraintsBwd.size()),
                                                                                                                         QString::number(pEvent->mConstraintsFwd.size()))
                                 + "<br>" + QObject::tr("- Method : %1").arg(MHVariable::getSamplerProposalText(pEvent->mTheta.mSamplerProposal))));
        }

        if (curveModel) {
            switch (model->mCurveSettings.mProcessType) {
            case CurveSettings::eProcess_Univariate:
                log += line(textGreen(QObject::tr("- Measure : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                    break;
            case CurveSettings::eProcess_Depth:
                log += line(textGreen(QObject::tr("- Depth : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                break;
            case CurveSettings::eProcess_Field:
                log += line(textGreen(QObject::tr("- Field : %1 ±  %2").arg(stringForLocal(pEvent->mZField), stringForLocal(pEvent->mS_ZField))));
                break;
            case CurveSettings::eProcess_Inclination:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                break;
            case CurveSettings::eProcess_Declination:
                log += line(textGreen(QObject::tr("- Declination : %1 ; Inclination %2 ±  %3").arg(stringForLocal(pEvent->mYDec), stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                break;

            case CurveSettings::eProcess_2D:
                log += line(textGreen(QObject::tr("- X : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Y : %1 ±  %2").arg(stringForLocal(pEvent->mYDec), stringForLocal(pEvent->mS_Y))));
                break;
            case CurveSettings::eProcess_3D:
                log += line(textGreen(QObject::tr("- X : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Y : %1 ±  %2")).arg(stringForLocal(pEvent->mYDec), stringForLocal(pEvent->mS_Y)));
                log += line(textGreen(QObject::tr("- Z : %1 ±  %2").arg(stringForLocal(pEvent->mZField), stringForLocal(pEvent->mS_ZField))));
                break;

            case CurveSettings::eProcess_Spherical:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Declination : %1").arg(stringForLocal(pEvent->mYDec))));
                break;
            case CurveSettings::eProcess_Unknwon_Dec:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                    log += line(textGreen(QObject::tr("- Field : %1").arg(stringForLocal(pEvent->mYDec))));
                break;
            case CurveSettings::eProcess_Vector:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(pEvent->mXIncDepth), stringForLocal(pEvent->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Declination : %1").arg(stringForLocal(pEvent->mYDec))));
                log += line(textGreen(QObject::tr("- Field : %1 ±  %2").arg(stringForLocal(pEvent->mZField), stringForLocal(pEvent->mS_ZField))));
                break;
            default:
                break;
            }

        }
        int j = 0;
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
        log += line(textOrange(QObject::tr("Phase ( %1 / %2 ) : %3 ( %4 events, %5 const. back., %6 const. fwd.)").arg(QString::number(i+1), QString::number(model->mPhases.size()), pPhase->mName,
                                                                                                              QString::number(pPhase->mEvents.size()),
                                                                                                              QString::number(pPhase->mConstraintsBwd.size()),
                                                                                                              QString::number(pPhase->mConstraintsFwd.size()))
                               + "<br>" + QObject::tr("- Type : %1").arg(pPhase->getTauTypeText())));
        log += "<br>";

        log += textBlue(QObject::tr("Events")) + " : ";
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
        log += line(textGreen( QObject::tr(" - Process : %1").arg(model->mCurveSettings.processText())));
        if (model->mCurveSettings.mProcessType == CurveSettings::eProcess_Depth )
            log += line(textGreen(QObject::tr(" - Minimal Rate : %1").arg(stringForLocal(model->mCurveSettings.mThreshold))));


        if (model->mCurveSettings.mUseErrMesure) {
            log += line(textGreen( QObject::tr(" - Use Measurement Error")));
        }

        if (model->mCurveSettings.mTimeType == CurveSettings::eModeBayesian) {
            log += line(textGreen( QObject::tr(" - Event Date : Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Event Date : Fixed on init value")));
        }

        if (model->mCurveSettings.mVarianceType == CurveSettings::eModeBayesian) {
            log += line(textGreen( QObject::tr(" - Std gi : Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Std gi : Fixed = %1").arg(QString::number(sqrt(model->mCurveSettings.mVarianceFixed)))));
        }

        if (model->mCurveSettings.mUseVarianceIndividual) {
            log += line(textGreen( QObject::tr(" - Use Std gi : Individual")));

        }  else {
            log += line(textGreen( QObject::tr(" - Use Global Std g")));
        }

        if (model->mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {
            log += line(textGreen( QObject::tr(" - Smoothing : Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Smoothing : Fixed = %1").arg(QString::number(model->mCurveSettings.mLambdaSpline))));
        }

    }

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
                HTMLText += line(textBold(textRed(QObject::tr("Bound ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), bound->mName))));
                HTMLText += line(textBold(textRed(QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(bound->mTheta.mX), DateUtils::getAppSettingsFormatStr()))));
            }

        }  else {
            HTMLText += line(textBold(textBlue(QObject::tr("Event ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), event->mName))));
            HTMLText += line(textBold(textBlue(QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(event->mTheta.mX), DateUtils::getAppSettingsFormatStr()))));
            if (event->mTheta.mLastAccepts.size()>2 && event->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                const auto acceptRate = event->mTheta.getCurrentAcceptRate();
                const auto samplerType = event->mTheta.mSamplerProposal;
                if (samplerType == MHVariable::eMHAdaptGauss && (acceptRate > 0.46 || acceptRate < 0.42) )
                    HTMLText += line(textBlue(QObject::tr("     Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.) + " %"))  + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");
                else
                    HTMLText += line(textBlue(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));

                HTMLText += line(textBlue(QObject::tr(" - Sigma_MH on Theta : %1").arg(stringForLocal(event->mTheta.mSigmaMH))));
            }

            HTMLText += line(textBlue(QObject::tr(" - S02 : %1").arg(stringForLocal(event->mS02.mX))));
        }

        if (curveModel) {
            HTMLText += line(textGreen(QObject::tr(" - Std gi : %1").arg(stringForLocal(sqrt(event->mVg.mX)))));
            if (event->mVg.mLastAccepts.size()>2  && event->mVg.mSamplerProposal!= MHVariable::eFixe) {
                const auto acceptRate = event->mVg.getCurrentAcceptRate();
                const auto samplerType = event->mVg.mSamplerProposal;
                if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                    HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                else
                    HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.)+" %")) + " ("  + MHVariable::getSamplerProposalText(samplerType)) + ")");

                HTMLText += line(textGreen(QObject::tr(" - Sigma_MH on Std gi : %1").arg(stringForLocal(event->mVg.mSigmaMH))));
            }

            // Recherche indice de l'event dans la liste de spline, car les events sont réordonnés
            int thetaIdx;
            const MCMCSpline& spline =  model->mSpline;
            for (thetaIdx = 0; thetaIdx < model->mEvents.size(); thetaIdx++) {
                if ( spline.splineX.vecThetaEvents.at(thetaIdx) == event->mTheta.mX)
                    break;
            }

            if (model->mCurveSettings.mProcessType == CurveSettings::eProcess_Univariate) {
                    HTMLText += line(textGreen(QObject::tr(" - G : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));

            } else {
                HTMLText += line(textGreen(QObject::tr(" - Gx : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));
                if (spline.splineY.vecG.size() != 0)
                    HTMLText += line(textGreen(QObject::tr(" - Gy : %1").arg(stringForLocal(spline.splineY.vecG.at(thetaIdx)))));
                if (spline.splineZ.vecG.size() != 0)
                    HTMLText += line(textGreen(QObject::tr(" - Gz : %1").arg(stringForLocal(spline.splineZ.vecG.at(thetaIdx)))));
            }

        }
        int j = 0;
        for (auto& date : event->mDates) {
            ++j;
            HTMLText += "<br>";

            HTMLText += line(textBold(textBlack(QObject::tr("Data ( %1 / %2 ) : %3").arg(QString::number(j), QString::number(event->mDates.size()), date.mName))));
            HTMLText += line(textBlack(QObject::tr(" - ti : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(date.mTi.mX), DateUtils::getAppSettingsFormatStr())));
            if (date.mTi.mSamplerProposal == MHVariable::eMHSymGaussAdapt) {
                if (date.mTi.mLastAccepts.size()>2) {
                    const auto acceptRate = date.mTi.getCurrentAcceptRate();
                    const auto samplerType = date.mTi.mSamplerProposal;
                    if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                        HTMLText += line(textBlack(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                    else
                        HTMLText += line(textBlack(QObject::tr("     Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.) +" %")) + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");

                }
                HTMLText += line(textBlack(QObject::tr(" - Sigma_MH on ti : %1").arg(stringForLocal(date.mTi.mSigmaMH))));
            }

            HTMLText += line(textBlack(QObject::tr(" - Sigma_i : %1").arg(stringForLocal(date.mSigmaTi.mX))));
            if (date.mSigmaTi.mLastAccepts.size()>2) {
                const auto acceptRate = date.mSigmaTi.getCurrentAcceptRate();
                const auto samplerType = date.mSigmaTi.mSamplerProposal;
                if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                    HTMLText += line(textBlack(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                else
                    HTMLText += line(textBlack(QObject::tr("     Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.) +" %")) + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");

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
            HTMLText += line(textBold(textOrange(QObject::tr("Phase ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mPhases.size()), phase->mName))));
            HTMLText += line(textOrange(QObject::tr(" - Begin : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mAlpha.mX), DateUtils::getAppSettingsFormatStr())));
            HTMLText += line(textOrange(QObject::tr(" - End : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mBeta.mX), DateUtils::getAppSettingsFormatStr())));
            HTMLText += line(textOrange(QObject::tr(" - Tau : %1").arg(stringForLocal(phase->mTau.mX))));
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
        if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe) {
            HTMLText +=  line(textGreen(QObject::tr("Fixed Smoothing : %1").arg(QLocale().toString(model->mLambdaSpline.mX, 'G', 2))));

        } else {
            HTMLText +=  line(textGreen(QObject::tr("Log10 Smoothing : %1").arg(QLocale().toString(log10(model->mLambdaSpline.mX), 'G', 2))));
            if (model->mLambdaSpline.mLastAccepts.size() > 2) {
                const auto acceptRate = model->mLambdaSpline.getCurrentAcceptRate();
                const auto samplerType = model->mLambdaSpline.mSamplerProposal;
                if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                    HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                else
                    HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.)+" %"))  + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");

               HTMLText +=  line(textGreen(QObject::tr(" - Sigma_MH on Smoothing : %1").arg(stringForLocal(model->mLambdaSpline.mSigmaMH))));
            }
        }
        HTMLText += "<hr>";
        HTMLText +=  line(textGreen(QObject::tr("sqrt Shrinkage param. : %1").arg(QLocale().toString(sqrt(model->mS02Vg.mX), 'G', 2))));
        if (model->mS02Vg.mLastAccepts.size() > 2) {
            const auto acceptRate = model->mS02Vg.getCurrentAcceptRate();
            const auto samplerType = model->mS02Vg.mSamplerProposal;
            if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
            else
                HTMLText += line(textGreen(QObject::tr("     Current Acceptance Rate : ") + textRed(stringForLocal(acceptRate*100.)) +" % (" + MHVariable::getSamplerProposalText(samplerType)) + ")");

            HTMLText +=  line(textGreen(QObject::tr(" - Sigma_MH on Shrinkage param. : %1").arg(stringForLocal(model->mS02Vg.mSigmaMH))));
        }
    }

    return HTMLText;
}



QString ModelUtilities::dateResultsHTML(const Date* d, const Model* model)
{
    Q_ASSERT(d);
    QString text = line(textBold(textBlack(QObject::tr("Data : %1").arg(d->mName)))) + "<br>";
    text += line(textBold(textBlack(QObject::tr("Posterior calib. date"))));

    if (d->mTi.mSamplerProposal != MHVariable::eFixe && model) {

        short position = ModelUtilities::HPDOutsideSudyPeriod(d->mTi.mFormatedHPD, model);
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
    text += line(textBlack(d->mTi.resultsString("",DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat))) ;

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
        text += line(textRed(e->mTheta.resultsString("", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat)));

    }
    else {
        text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->mName)))) + "<br>";
        text += line(textBold(textBlue(QObject::tr("Posterior Event Date"))));
        text += line(textBlue(e->mTheta.resultsString("", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat)));


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

    if (e->mType == Event::eBound) {
        text += line(textBold(textRed(QObject::tr("Bound : %1").arg(e->mName))));

    } else {
        text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->mName))));
    }

    if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
            text = line(textBold(textGreen(QObject::tr("Std gi"))));
            text += line(textGreen(QObject::tr("Fixed value : %1").arg(QString::number(e->mVg.mRawTrace->at(0)))));

    } else {
        text += line(textBold(textGreen(QObject::tr("Posterior Std gi"))));
        text += line(textGreen(e->mVg.resultsString("", nullptr, nullptr)));
    }
    return text;
}

QString ModelUtilities::phaseResultsHTML(const Phase* p)
{
    QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->mName))));
    text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("Begin (posterior distrib.)"))));
    text += line(textOrange(p->mAlpha.resultsString("", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat)));

    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("End (posterior distrib.)"))));
    text += line(textOrange(p->mBeta.resultsString("", DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat)));

    if (p->mTimeRange != std::pair<double, double>(- INFINITY, +INFINITY)) {
        text += "<br>";
        // we suppose it's the same mThreshohdUsed than alpha
        const QString result = QObject::tr("Phase Time Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mAlpha.mThresholdUsed),
                                                                                            stringForLocal(p->getFormatedTimeRange().first),
                                                                                            stringForLocal(p->getFormatedTimeRange().second),
                                                                                            DateUtils::getAppSettingsFormatStr());
        text += line(textBold(textOrange(result)));
    }

    return text;
}

QString ModelUtilities::durationResultsHTML(const Phase* p)
{
   QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->mName))));
   text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

   text += "<br>";
   text += line(textBold(textOrange(QObject::tr("Duration (posterior distrib.)"))));
   text += line(textOrange(p->mDuration.resultsString(QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"), nullptr)));

   return text;
}

QString ModelUtilities::tempoResultsHTML(const Phase* p)
{
    Q_ASSERT(p);
    QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->mName))));
    text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    return text;
}

QString ModelUtilities::activityResultsHTML(const Phase* p)
{
    QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->mName))));
    text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    text += "<br>";

    if (p->mEvents.size()<2) {
        text += line(textOrange("<i>" + QObject::tr("No Stat.")  + "</i>"));
        return text;
    }

    double t1 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("t_min").mValue);
    double t2 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("t_max").mValue);

    if (t1>t2)
        std::swap(t1, t2);
    text += line(textOrange("<i>" + QObject::tr("Trace Stat.")  + "</i>"));
    text += line(textOrange("Theta min = " + stringForLocal(t1) + " " + DateUtils::getAppSettingsFormatStr()));
    text += line(textOrange("Theta max = " + stringForLocal(t2) + " " + DateUtils::getAppSettingsFormatStr()));

    text += "<hr>";
    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("Activity"))));
    t1 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_TimeRange_min").mValue);
    t2 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_TimeRange_max").mValue);

    if (t1>t2)
        std::swap(t1, t2);

    text += line(textOrange(QObject::tr("Activity Time Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg( stringForLocal(p->mValueStack.at("Activity_TimeRange_Level").mValue),
                                                                                        stringForLocal(t1),
                                                                                        stringForLocal(t2),
                                                                                        DateUtils::getAppSettingsFormatStr())));

    text += line(textOrange("Activity Span = " + stringForLocal(t2-t1) ));
    text += line(textOrange("Activity Theta mean = " + stringForLocal(DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_mean95").mValue)) + " " + DateUtils::getAppSettingsFormatStr()));
    text += line(textOrange("Activity Theta std = " + stringForLocal(p->mValueStack.at("Activity_std95").mValue) ));

    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("h Estimation"))));

    QString textValue = stringForLocal(p->mValueStack.at("Activity_Significance_Score").mValue, true);
    const double threshold = p->mValueStack.at("Activity_Threshold").mValue;
    const auto hActi = p->mValueStack.at("Activity_h").mValue;
    text += line(textOrange("Significance Score"  + QString(" ( %1 %) = ").arg(threshold) + textValue + QString(" with h = %1").arg(hActi)));

    text += line(""); 
    text += line(textOrange("Activity max"  + QString(" = %1").arg(p->mValueStack.at("Activity_max").mValue) ));
    text += line(textOrange("Activity mode"  + QString(" = %1").arg(DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_mode").mValue)) + " " + DateUtils::getAppSettingsFormatStr() )) ;

    const double hUnif = (3.686*p->mValueStack.at("Activity_std95").mValue)/pow(p->mEvents.size(), 1./5.);
    text += "<br>";
    text += line(textOrange("Optimal h Unif = " + stringForLocal(hUnif)));

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

    if (p->mGapRange != QPair<double, double>()) {
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

    if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe  && model->mCurveSettings.mLambdaSplineType != CurveSettings::eInterpolation) {
        text += line(textGreen(QObject::tr("Lambda Spline; Fixed value = %1").arg(QString::number(pow(10, model->mLambdaSpline.mRawTrace->at(0))))));

    } else if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe && model->mCurveSettings.mLambdaSplineType == CurveSettings::eInterpolation) {
        text += line(textGreen(QObject::tr("- Lambda Spline; Interpolation Fixed value = %1").arg(QString::number(0))));

    }else {
        text += line(textGreen(QObject::tr("- Mean of the log10 of Lambda Spline = %1").arg(stringForLocal(model->mLambdaSpline.mResults.funcAnalysis.mean))));
    }

    if (model->mS02Vg.mSamplerProposal == MHVariable::eFixe) {
        text += line(textGreen(QObject::tr("- S02 Vg; Fixed value = %1").arg(QString::number(pow( model->mS02Vg.mRawTrace->at(0), 2.)))));

    } else {
        text += line(textGreen(QObject::tr("- Mean of the sqrt of S02 Vg = %1").arg(stringForLocal(model->mS02Vg.mResults.funcAnalysis.mean))));
    }

    if (model->mCurveSettings.mProcessType == CurveSettings::eProcess_Depth) {
         text += "<br>" + line( textBold(textGreen(QObject::tr("Positive curves accepted with Threshold : %1").arg(stringForLocal(model->mCurveSettings.mThreshold)))));
        const unsigned requiredCurve = floor(model->mMCMCSettings.mIterPerAquisition / model->mMCMCSettings.mThinningInterval);
        unsigned totalPositvIter = 0;
        unsigned totalPequiredCurve = 0;
        float rate;

        int i = 0;
        for (const auto &ch : model->mChains) {
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
    QString text;
    if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe  && model->mCurveSettings.mLambdaSplineType != CurveSettings::eInterpolation) {
        text = line(textBold(textGreen(QObject::tr("Lambda Spline"))));
        text += line(textGreen(QObject::tr("Fixed value : %1").arg(QString::number(pow(10, model->mLambdaSpline.mRawTrace->at(0))))));

    } else if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe && model->mCurveSettings.mLambdaSplineType == CurveSettings::eInterpolation) {
        text = line(textBold(textGreen(QObject::tr("Lambda Spline"))));
        text += line(textGreen(QObject::tr("Interpolation Fixed value : %1").arg(QString::number(0))));

    }else {
        text = line(textBold(textGreen(QObject::tr("Stat. on the log10 of Lambda Spline"))));
        text += line(textGreen(model->mLambdaSpline.resultsString("", nullptr, nullptr)));
    }
    return text;
}

QString ModelUtilities::S02ResultsHTML(const ModelCurve* model)
{
    QString text;
    if (model->mS02Vg.mSamplerProposal == MHVariable::eFixe) {
        text = line(textBold(textGreen(QObject::tr("S02 Vg"))));
        text += line(textGreen(QObject::tr("Fixed value : %1").arg(QString::number(pow( model->mS02Vg.mRawTrace->at(0), 2.)))));

    } else {
        text = line(textBold(textGreen(QObject::tr("Stat. on the sqrt S02 Vg"))));
        text += line(textGreen(model->mS02Vg.resultsString("", nullptr, nullptr)));
    }
    return text;
}
/**
 * @brief HPDOutsideSudyPeriod
 * @param hpd, the hpd must be send with the good date format
 * @param model
 * @return -1 if there is solution under Study Period; 0 if all solution is inside Study Period;
 *  +1 if there is solution over Study Period; +2 if there is both solution under and over the Study Period
 */
short ModelUtilities::HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const Model* model)
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

void sampleInCumulatedRepartition (Event* event, const StudyPeriodSettings &settings, const double min, const double max)
{

    // Creation of the cumulative date distribution
    // 1 - Search for tmin and tmax, distribution curves, identical to the calibration.
    double unionTmin (+INFINITY);
    double unionTmax (-INFINITY);
    double unionStep (settings.mStep);
    for (auto&& d : event->mDates) {
        if (d.mCalibration != nullptr && !d.mCalibration->mVector.isEmpty() ) {
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
    // std::max(unionStep, (unionTmax - unionTmin)/3.) prevent (unionTmax - unionTmin) = 0 with DoubleExp

    if (unionTmax < min) {
        event->mTheta.mX = Generator::gaussByDoubleExp((unionTmax + unionTmin)/2., std::max(unionStep, (unionTmax - unionTmin)/3.), min, max);
#ifdef DEBUG
        if (event->mTheta.mX == min)
            qDebug() << "sampleInCumulatedRepartition unionTmax< min and (event->mTheta.mX == min)";
#endif
    } else if (max < unionTmin) {
        event->mTheta.mX = Generator::gaussByDoubleExp((unionTmax + unionTmin)/2., std::max(unionStep, (unionTmax - unionTmin)/3.), min, max);

#ifdef DEBUG
        if (event->mTheta.mX == max)
            qDebug() << "[sampleInCumulatedRepartition] max<unionTmin and (event->mTheta.mX == max)";
#endif
    } else {
        unionTmin = std::max(unionTmin, min);
        unionTmax = std::min(unionTmax, max);

        // 3 - Creation of the cumulative distribution curves in the interval
        QVector<double> unionRepartition (0);
        double tWhile = unionTmin;
        double sumWhile = 0.;

        while (tWhile<= unionTmax) {
            sumWhile= 0.;
            for (auto&& d : event->mDates) {
                //sumWhile += interpolate_value_from_curve(tWhile, d.mCalibration->mRepartition, d.mCalibration->mTmin, d.mCalibration->mTmax);
               sumWhile += d.mCalibration->repartition_interpolate(tWhile);
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
        if ( (minRepartition != 0. || maxRepartition != 0.) &&
             (unionRepartition.size() > 1)) {
            const double idx = vector_interpolate_idx_for_value(Generator::randomUniform()*(maxRepartition-minRepartition) + minRepartition, unionRepartition);
            event->mTheta.mX = unionTmin + idx * unionStep;
#ifdef DEBUG
        if (event->mTheta.mX == min || event->mTheta.mX == max)
            qDebug() << "[sampleInCumulatedRepartition] event->mTheta.mX == min || event->mTheta.mX == max";
#endif

        } else {
            event->mTheta.mX = Generator::randomUniform(min, max);
        }

    }
}

void sampleInCumulatedRepartition_thetaFixe (Event *event, const StudyPeriodSettings& settings)
{

    // Creation of the cumulative date distribution
    // 1 - Search for tmin and tmax, distribution curves, identical to the calibration.
    double unionTmin (+INFINITY);
    double unionTmax (-INFINITY);
    double unionStep (settings.mStep);
    for (auto&& d : event->mDates) {
        if (d.mCalibration != nullptr && !d.mCalibration->mVector.isEmpty() ) {
            unionTmin = std::min(unionTmin, d.mCalibration->mTmin);
            unionTmax = std::max(unionTmax, d.mCalibration->mTmax);
            unionStep = std::min(unionStep, d.mCalibration->mStep);

        } else {
            unionTmin = settings.mTmin;
            unionTmax = settings.mTmax;
        }

    }

    // 2 - Creation of the cumulative distribution curves in the interval
        QVector<double> unionRepartition (0);
        double tWhile (unionTmin);
        double sumWhile (0.);

        while (tWhile<= unionTmax) {
            sumWhile= 0.;
            for (auto&& d : event->mDates) {
                //sumWhile += interpolate_value_from_curve(tWhile, d.mCalibration->mRepartition, d.mCalibration->mTmin, d.mCalibration->mTmax);
                sumWhile += d.mCalibration->repartition_interpolate(tWhile);
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
        if ( (minRepartition != 0. || maxRepartition != 0.) &&
             (unionRepartition.size() > 1)) {
            const double idx = vector_interpolate_idx_for_value(0.5*(maxRepartition-minRepartition) + minRepartition, unionRepartition);
            event->mTheta.mX = unionTmin + idx * unionStep;

        } else {
            event->mTheta.mX = Generator::randomUniform(settings.mTmin, settings.mTmax);
        }


}

QString HTML_to_text(const QString &HTML)
{
        QString result;
        // recherche-remplacement retour chariot
        QString HTML2 (HTML);
        HTML2.replace("<br>","\r");

        //supression balise HTLM
        bool balise_open = false;
        for (auto c : HTML2) {
            if (c == '<')
                balise_open = true;
            else if (c == '>')
                balise_open = false;
            else if (balise_open)
                result.append(c);

        }
        return result;
}
