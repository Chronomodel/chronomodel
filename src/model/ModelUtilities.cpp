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

#include <utility>
#include <regex>



bool sortEvents(std::shared_ptr<Event> e1, std::shared_ptr<Event> e2)
{
    return (e1->mItemY < e2->mItemY);
}

bool sortPhases(std::shared_ptr<Phase> p1, std::shared_ptr<Phase> p2)
{
    return (p1->mItemY < p2->mItemY);
}


// Events Branches
std::vector<std::vector<std::shared_ptr<Event>> > ModelUtilities::getNextBranches(const std::vector<std::shared_ptr<Event>> &curBranch, std::shared_ptr<Event> lastNode)
{
    std::vector<std::vector<std::shared_ptr<Event>> > branches;
    const std::vector<std::shared_ptr<EventConstraint>> &cts = lastNode->mConstraintsFwd;
    if (cts.size() > 0) {
        for (auto& ct : cts) {
            std::vector<std::shared_ptr<Event>> branch = curBranch;
            std::shared_ptr<Event> newNode = ct->mEventTo;

            if (newNode->mLevel <= lastNode->mLevel)
                newNode->mLevel = lastNode->mLevel + 1;

            if (!container_contains(branch, newNode)) {
                branch.push_back(newNode);
                std::vector<std::vector<std::shared_ptr<Event>> > nextBranches = getNextBranches(branch, ct->mEventTo);

                for (auto& nb : nextBranches)
                    branches.push_back(nb);

            } else {
                QStringList evtNames;

                 for (auto& b : branch)
                     evtNames << b->getQStringName();


                evtNames << newNode->getQStringName();

                throw QObject::tr("Circularity found in events model !\rPlease correct this branch :\r") + evtNames.join(" -> ");
            }
        }
    } else {
        branches.push_back(curBranch);
    }
    return branches;
}

std::vector<std::vector<std::shared_ptr<Event>> > ModelUtilities::getBranchesFromEvent(std::shared_ptr<Event> start)
{
    std::vector<std::shared_ptr<Event>> startBranch;
    start->mLevel = 0;
    startBranch.push_back(start);

    std::vector<std::vector<std::shared_ptr<Event>> > nextBranches;
    try {
        nextBranches = getNextBranches(startBranch, start);
    } catch(QString error){
        throw std::move(error);
    }

    return nextBranches;
}


std::vector<std::vector<std::shared_ptr<Event>> > ModelUtilities::getAllEventsBranches(const std::vector<std::shared_ptr<Event>>& events)
{
    std::vector<std::vector<std::shared_ptr<Event>> > branches;

    // ----------------------------------------
    //  Put all events level to 0 and
    //  store events at start of branches (= not having constraint backward)
    // ----------------------------------------
    std::vector<std::shared_ptr<Event>> starts;
    for (auto&& event : events) {
        event->mLevel = 0;
        if (event->mConstraintsBwd.size() == 0)
            starts.push_back(event);
    }

    if (starts.size() == 0 && events.size() != 0)
        throw QObject::tr("Circularity found in events model !");

    else {
        for (auto& s : starts) {
            std::vector<std::vector<std::shared_ptr<Event>> > eventBranches;
            try {
                eventBranches = getBranchesFromEvent(s);
            } catch(QString error) {
                throw std::move(error);
            }
            for (auto& eb : eventBranches)
                branches.push_back(eb);
        }
    }
    return branches;
}




// Phases Branches
std::vector<std::vector<Phase*> > ModelUtilities::getNextBranches(const std::vector<Phase*> &curBranch, const Phase* lastNode, const double gammaSum, const double maxLength)
{
    std::vector<std::vector<Phase*> > branches;
    const std::vector<std::shared_ptr<PhaseConstraint>> &cts = lastNode->mConstraintsNextPhases;
    if (cts.size() > 0) {
        for (auto& ct : cts) {
            std::vector<Phase*> branch = curBranch;
            std::shared_ptr<Phase> newNode = ct->mPhaseTo;

            double gamma = gammaSum;
            if (ct->mGammaType == PhaseConstraint::eGammaFixed)
                gamma += ct->mGammaFixed;

            else if (ct->mGammaType == PhaseConstraint::eGammaRange)
                gamma += ct->mGammaMin;

            if (gamma < maxLength) {
                if (newNode->mLevel <= lastNode->mLevel)
                    newNode->mLevel = lastNode->mLevel + 1;

                if (!container_contains(branch, newNode.get())) {
                    branch.push_back(newNode.get());
                    std::vector<std::vector<Phase*> > nextBranches = getNextBranches(branch, ct->mPhaseTo.get(), gamma, maxLength);
                    for (auto& nb : nextBranches)
                        branches.push_back(nb);
                }
                else {
                    QStringList names;
                    for (auto& b : branch)
                        names << b->getQStringName();
                    names << newNode->getQStringName();

                    throw QObject::tr("Circularity found in phases model !\rPlease correct this branch :\r") + names.join(" -> ");
                }
            }
            else {
                QStringList names;
                for (auto& c : curBranch)
                    names << c->getQStringName();

                names << newNode->getQStringName();
                throw QObject::tr("Phases branch too long :\r") + names.join(" -> ");
            }
        }
    }
    else
        branches.push_back(curBranch);

    return branches;
}

std::vector<std::vector<Phase*> > ModelUtilities::getBranchesFromPhase(Phase* start, const double maxLength)
{
    Q_ASSERT(start);
    std::vector<Phase*> startBranch;
    start->mLevel = 0;
    startBranch.push_back(start);

    std::vector<std::vector<Phase*> > nextBranches;
    try {
        nextBranches = getNextBranches(startBranch, start, 0, maxLength);
    } catch(QString error) {
        throw std::move(error);
    }

    return nextBranches;
}


std::vector<std::vector<Phase *> > ModelUtilities::getAllPhasesBranches(const std::vector<std::shared_ptr<Phase>> &phases, const double maxLength)
{
    std::vector<std::vector<Phase*> > branches;

    std::vector<Phase*> starts;
    for (auto& p : phases) {
        p->mLevel = 0;
        if (p->mConstraintsPrevPhases.size() == 0)
            starts.push_back(p.get());
    }
    if (starts.size() == 0 && phases.size() != 0)
        throw QObject::tr("Circularity found in phases model !");

    try {
            for (auto& s : starts) {
                const std::vector<std::vector<Phase*> > &phaseBranches = getBranchesFromPhase(s, maxLength);
                for (auto& pb : phaseBranches) {
                    branches.push_back(pb);
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
std::vector<std::shared_ptr<Event>> ModelUtilities::unsortEvents(const std::vector<std::shared_ptr<Event> > &events)
{
    std::vector<std::shared_ptr<Event>> results (events);
    for (auto i = results.size()-1; i > 0; --i){
        std::swap(results[i], results[Generator::randomUniformInt(0, (int)i)]);
    }
    return results;
}


QString ModelUtilities::getMCMCSettingsLog(const std::shared_ptr<ModelCurve> model)
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
QString ModelUtilities::modelDescriptionHTML(const std::shared_ptr<ModelCurve> model)
{
    QString log;
    // Study period
    QLocale locale = QLocale();
    log += line(textBold(textBlack(QObject::tr("Prior Study Period : [ %1 : %2 ] %3").arg(locale.toString(model->mSettings.getTminFormated()), locale.toString(model->mSettings.getTmaxFormated()), DateUtils::getAppSettingsFormatStr() ))));
    log += line(textOrange(QObject::tr("Number of Phase  : %1").arg(QString::number(model->mPhases.size()))));
    log += line(textBlue(QObject::tr("Number of Event  : %1").arg(QString::number(model->mEvents.size()))));
    log += "<hr>";

    int i = 0;
    for (auto&& event : model->mEvents) {
        if (event->type() == Event::eBound) {
            auto bound = dynamic_cast<Bound*>(event.get());
            log += line(textRed(QObject::tr("Bound ( %1 / %2 ) : %3 ( %4  phases,  %5 const. back.,  %6 const.fwd.)").arg(QString::number(i+1), QString::number(model->mEvents.size()), bound->getQStringName(),
                                                                                                               QString::number(bound->mPhases.size()),
                                                                                                               QString::number(bound->mConstraintsBwd.size()),
                                                                                                               QString::number(bound->mConstraintsFwd.size()))));
            log += line(textRed(QObject::tr("- Fixed Value : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(bound->mFixed), DateUtils::getAppSettingsFormatStr() )));

        } else {
            log += line(textBlue(QObject::tr("Event ( %1 / %2 ) : %3 ( %4 data, %5 phases,  %6 const. back.,  %7 const. fwd.)").arg(QString::number(i+1), QString::number(model->mEvents.size()), event->getQStringName(),
                                                                                                                         QString::number(event->mDates.size()),
                                                                                                                         QString::number(event->mPhases.size()),
                                                                                                                         QString::number(event->mConstraintsBwd.size()),
                                                                                                                         QString::number(event->mConstraintsFwd.size()))
                                 + "<br>" + QObject::tr("- MCMC %1").arg(MHVariable::getSamplerProposalText(event->mTheta.mSamplerProposal))));
        }

        if (model->is_curve) {
            switch (model->mCurveSettings.mProcessType) {
            case CurveSettings::eProcess_Univariate:
                log += line(textGreen(QObject::tr("- Measure : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                    break;
            case CurveSettings::eProcess_Depth:
                log += line(textGreen(QObject::tr("- Depth : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                break;
            case CurveSettings::eProcess_Field:
                log += line(textGreen(QObject::tr("- Field : %1 ±  %2").arg(stringForLocal(event->mZField), stringForLocal(event->mS_ZField))));
                break;
            case CurveSettings::eProcess_Inclination:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                break;
            case CurveSettings::eProcess_Declination:
                log += line(textGreen(QObject::tr("- Declination : %1 ; Inclination %2 ±  %3").arg(stringForLocal(event->mYDec), stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                break;

            case CurveSettings::eProcess_2D:
                log += line(textGreen(QObject::tr("- X : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Y : %1 ±  %2").arg(stringForLocal(event->mYDec), stringForLocal(event->mS_Y))));
                break;
            case CurveSettings::eProcess_3D:
                log += line(textGreen(QObject::tr("- X : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Y : %1 ±  %2")).arg(stringForLocal(event->mYDec), stringForLocal(event->mS_Y)));
                log += line(textGreen(QObject::tr("- Z : %1 ±  %2").arg(stringForLocal(event->mZField), stringForLocal(event->mS_ZField))));
                break;

            case CurveSettings::eProcess_Spherical:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Declination : %1").arg(stringForLocal(event->mYDec))));
                break;
            case CurveSettings::eProcess_Unknwon_Dec:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Field : %1").arg(stringForLocal(event->mYDec))));
                break;
            case CurveSettings::eProcess_Vector:
                log += line(textGreen(QObject::tr("- Inclination : %1 ±  %2").arg(stringForLocal(event->mXIncDepth), stringForLocal(event->mS_XA95Depth))));
                log += line(textGreen(QObject::tr("- Declination : %1").arg(stringForLocal(event->mYDec))));
                log += line(textGreen(QObject::tr("- Field : %1 ±  %2").arg(stringForLocal(event->mZField), stringForLocal(event->mS_ZField))));
                break;
            default:
                break;
            }

        }
        int j = 0;
        for (auto&& date : event->mDates) {
            log += "<br>";
            log += line(textBlack(QObject::tr("Data ( %1 / %2 ) : %3").arg(QString::number(j+1), QString::number(event->mDates.size()), date.getQStringName())
                                  + "<br>" + QObject::tr("- Type : %1").arg(date.mPlugin->getName())
                                  + "<br>" + QObject::tr("- MCMC %1").arg(MHVariable::getSamplerProposalText(date.mTi.mSamplerProposal))
                                  + "<br>" + QObject::tr("- Params : %1").arg(date.getDesc()))
                        + (date.mDeltaType != Date::eDeltaNone ? textBlack("<br> - "+ date.getWiggleDesc()): ""));
            /*if (date.mDeltaType != Date::eDeltaNone) {
                log += line(textBlack("<br>"+ date.getWiggleDesc()));
            }*/
            ++j;
        }
        log += "<hr>";
        ++i;
    }

    i = 0;
    for (auto &&phase : model->mPhases) {
        log += line(textOrange(QObject::tr("Phase ( %1 / %2 ) : %3 ( %4 events, %5 const. back., %6 const. fwd.)").arg(QString::number(i+1), QString::number(model->mPhases.size()), phase->getQStringName(),
                                                                                                              QString::number(phase->mEvents.size()),
                                                                                                              QString::number(phase->mConstraintsPrevPhases.size()),
                                                                                                              QString::number(phase->mConstraintsNextPhases.size()))
                               + "<br>" + QObject::tr("- Type : %1").arg(phase->getTauTypeText())));
        log += "<br>";

        log += textBlue(QObject::tr("Events")) + " : ";
        for (auto &&event : phase->mEvents)
            log += textBlue(event->getQStringName()) + " - ";

        log += "<br>";
        log += "<hr>";

        ++i;
    }

    for (auto&& phaseConst : model->mPhaseConstraints) {
        log += line(textBold(textGreen( QObject::tr("Succession from %1 to %2").arg(phaseConst->mPhaseFrom->getQStringName(), phaseConst->mPhaseTo->getQStringName()))));

        switch(phaseConst->mGammaType) {
            case PhaseConstraint::eGammaFixed :
                log += line(textBold(textGreen( QObject::tr("Min Hiatus fixed = %1").arg(phaseConst->mGammaFixed))));
                break;
            case PhaseConstraint::eGammaUnknown :
                log += line(textBold(textGreen( QObject::tr("Min Hiatus unknown") )));
                break;
            case PhaseConstraint::eGammaRange : //no longer used
                 log += line(textBold(textGreen( QObject::tr("Min Hiatus between %1 and %2").arg(phaseConst->mGammaMin, phaseConst->mGammaMax))));
                break;
            default:
                log += "Hiatus undefined -> ERROR";
            break;
        }

        log += "<hr>";
        log += "<br>";
    }

    if (model->is_curve) {
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
            log += line(textGreen( QObject::tr(" - Std gi : Individual Bayesian")));

        } else if (model->mCurveSettings.mVarianceType == CurveSettings::eModeGlobal) {
            log += line(textGreen( QObject::tr(" - Std gi : Global Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Std gi : Fixed = %1").arg(QString::number(sqrt(model->mCurveSettings.mVarianceFixed)))));
        }

        if (model->mCurveSettings.mLambdaSplineType == CurveSettings::eModeBayesian) {
            log += line(textGreen( QObject::tr(" - Smoothing : Bayesian")));

        } else {
            log += line(textGreen( QObject::tr(" - Smoothing : Fixed = %1").arg(QString::number(model->mCurveSettings.mLambdaSpline))));
        }

    }

    return log;

}

QString ModelUtilities::modelStateDescriptionHTML(const std::shared_ptr<ModelCurve> model, QString stateDescript)
{
    const bool curveModel = model->is_curve;

    int i = 0;
    QString HTMLText = stateDescript;
    HTMLText += "<hr><br>";
    HTMLText +=  textBold("Events  (with their data)") +"<br>";
    for (auto& event : model->mEvents) {
        ++i;
        HTMLText += "<hr>";

        if (event->type() == Event::eBound) {
            const Bound* bound = dynamic_cast<const Bound*>(event.get());

            if (bound) {
                HTMLText += line(textBold(textRed(QObject::tr("Bound ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), bound->getQStringName()))));
                HTMLText += line(textBold(textRed(QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(bound->mTheta.mX), DateUtils::getAppSettingsFormatStr()))));
            }

        }  else {
            HTMLText += line(textBold(textBlue(QObject::tr("Event ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mEvents.size()), event->getQStringName()))));
            HTMLText += line(textBold(textBlue(QObject::tr(" - Theta : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(event->mTheta.mX), DateUtils::getAppSettingsFormatStr()))));
            if (event->mTheta.mLastAccepts.size()>2 && event->mTheta.mSamplerProposal!= MHVariable::eFixe) {
                const auto acceptRate = event->mTheta.getCurrentAcceptRate();
                const auto samplerType = event->mTheta.mSamplerProposal;
                if (samplerType == MHVariable::eMHAdaptGauss && (acceptRate > 0.46 || acceptRate < 0.42) )
                    HTMLText += line(textBlue(QObject::tr(". . . . . Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.) + " %"))  + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");
                else
                    HTMLText += line(textBlue(QObject::tr(". . . . . Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));

                HTMLText += line(textBlue(QObject::tr(" - Sigma_MH on Theta : %1").arg(stringForLocal(event->mTheta.mSigmaMH))));
            }
#ifdef S02_BAYESIAN
            HTMLText += line(textBold(textBlue(QObject::tr(" - Event Shrinkage : %1").arg(DateUtils::convertToAppSettingsFormatStr(event->mS02Theta.mX)))));
            if (event->mS02Theta.mLastAccepts.size()>2 && event->mS02Theta.mSamplerProposal!= MHVariable::eFixe) {
                const auto acceptRate = event->mS02Theta.getCurrentAcceptRate();
                const auto samplerType = event->mS02Theta.mSamplerProposal;
                if (samplerType == MHVariable::eMHAdaptGauss && (acceptRate > 0.46 || acceptRate < 0.42) )
                    HTMLText += line(textBlue(QObject::tr("     Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.) + " %"))  + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");
                else
                    HTMLText += line(textBlue(QObject::tr("     Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));

                HTMLText += line(textBlue(QObject::tr(" - Sigma_MH on S02 : %1").arg(stringForLocal(event->mS02Theta.mSigmaMH))));
            }
#else
            HTMLText += line(textBlue(QObject::tr(" - Event Shrinkage : %1").arg(stringForLocal(event->mS02Theta.mX))));
#endif
        }

        if (curveModel) {
            HTMLText += line(textGreen(QObject::tr(" - Std gi : %1").arg(stringForLocal(sqrt(event->mVg.mX)))));
            if (event->mVg.mLastAccepts.size()>2  && event->mVg.mSamplerProposal!= MHVariable::eFixe) {
                const auto acceptRate = event->mVg.getCurrentAcceptRate();
                const auto samplerType = event->mVg.mSamplerProposal;
                if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                    HTMLText += line(textGreen(QObject::tr(". . . . . Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                else
                    HTMLText += line(textGreen(QObject::tr(". . . . . Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.)+" %")) + " ("  + MHVariable::getSamplerProposalText(samplerType) + ")"));

                HTMLText += line(textGreen(QObject::tr(" - Sigma_MH on Std gi : %1").arg(stringForLocal(event->mVg.mSigmaMH))));
            }

            // Recherche indice de l'event dans la liste de spline, car les events sont réordonnés
           /* size_t thetaIdx = 0;
            const MCMCSpline& spline =  model->mSpline;
            for (; thetaIdx < model->mEvents.size(); thetaIdx++) {
                if ( spline.splineX.vecThetaReduced.at(thetaIdx) == event->mThetaReduced)
                    break;
            }


            if (model->mCurveSettings.mProcessType == CurveSettings::eProcess_Univariate) {
                HTMLText += line(textGreen(QObject::tr(" - G : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));

            } else if (model->mCurveSettings.mProcessType == CurveSettings::eProcess_Depth) {
                HTMLText += line(textGreen(QObject::tr(" - Depth : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));

            } else {
                HTMLText += line(textGreen(QObject::tr(" - Gx : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));
                if (spline.splineY.vecG.size() != 0)
                    HTMLText += line(textGreen(QObject::tr(" - Gy : %1").arg(stringForLocal(spline.splineY.vecG.at(thetaIdx)))));
                if (spline.splineZ.vecG.size() != 0)
                    HTMLText += line(textGreen(QObject::tr(" - Gz : %1").arg(stringForLocal(spline.splineZ.vecG.at(thetaIdx)))));
            }*/

            const auto& sX = model->mSpline.splineX.vecThetaReduced;
            auto it = std::find(sX.begin(), sX.end(), event->mThetaReduced);
            if (it != sX.end()) {
                const MCMCSpline& spline =  model->mSpline;
                size_t thetaIdx = std::distance(sX.begin(), it);

                if (model->mCurveSettings.mProcessType == CurveSettings::eProcess_Univariate) {
                    HTMLText += line(textGreen(QObject::tr(" - G : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));

                } else if (model->mCurveSettings.mProcessType == CurveSettings::eProcess_Depth) {
                    HTMLText += line(textGreen(QObject::tr(" - Depth : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));

                } else {
                    HTMLText += line(textGreen(QObject::tr(" - Gx : %1").arg(stringForLocal(spline.splineX.vecG.at(thetaIdx)))));
                    if (spline.splineY.vecG.size() != 0)
                        HTMLText += line(textGreen(QObject::tr(" - Gy : %1").arg(stringForLocal(spline.splineY.vecG.at(thetaIdx)))));
                    if (spline.splineZ.vecG.size() != 0)
                        HTMLText += line(textGreen(QObject::tr(" - Gz : %1").arg(stringForLocal(spline.splineZ.vecG.at(thetaIdx)))));
                }
            }
#if DEBUG
            else {
                HTMLText += line(textGreen("errror in model Description"));
                qDebug()<<"[modelStateDescriptionHTML] errror";
            }
#endif

        }
        int j = 0;
        for (auto& date : event->mDates) {
            ++j;
            HTMLText += "<br>";

            HTMLText += line(textBold(textBlack(QObject::tr("Data ( %1 / %2 ) : %3").arg(QString::number(j), QString::number(event->mDates.size()), date.getQStringName()))));
            HTMLText += line(textBlack(QObject::tr(" - ti : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(date.mTi.mX), DateUtils::getAppSettingsFormatStr())));
            if (date.mTi.mSamplerProposal == MHVariable::eMHPrior) {
                if (date.mTi.mLastAccepts.size()>2) {
                    const auto acceptRate = date.mTi.getCurrentAcceptRate();
                    const auto samplerType = date.mTi.mSamplerProposal;
                    if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                        HTMLText += line(textBlack(QObject::tr(". . . . . Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                    else
                        HTMLText += line(textBlack(QObject::tr(". . . . . Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.) +" %")) + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");

                }
                HTMLText += line(textBlack(QObject::tr(" - Sigma_MH on ti : %1").arg(stringForLocal(date.mTi.mSigmaMH))));
            }

            HTMLText += line(textBlack(QObject::tr(" - Sigma_i : %1").arg(stringForLocal(date.mSigmaTi.mX))));
            if (date.mSigmaTi.mLastAccepts.size()>2) {
                const auto acceptRate = date.mSigmaTi.getCurrentAcceptRate();
                const auto samplerType = date.mSigmaTi.mSamplerProposal;
                if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                    HTMLText += line(textBlack(QObject::tr(". . . . . Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                else
                    HTMLText += line(textBlack(QObject::tr(". . . . . Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.) +" %")) + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");

            }
            HTMLText += line(textBlack(QObject::tr(" - Sigma_MH on Sigma_i : %1").arg(stringForLocal(date.mSigmaTi.mSigmaMH))));
            if (date.mDeltaType != Date::eDeltaNone)
                HTMLText += line(textBlack(QObject::tr(" - Delta_i : %1").arg(stringForLocal(date.mDelta))));

        }
    }

    if (model->mPhases.size() > 0) {
        HTMLText += "<hr><br>";
        HTMLText += textBold(QObject::tr("Phases"))+"<br>";
        HTMLText += "<hr>";

        int i = 0;
        for (auto& phase : model->mPhases) {
            ++i;
            HTMLText += line(textBold(textOrange(QObject::tr("Phase ( %1 / %2 ) : %3").arg(QString::number(i), QString::number(model->mPhases.size()), phase->getQStringName()))));
            HTMLText += line(textOrange(QObject::tr(" - Begin : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mAlpha.mX), DateUtils::getAppSettingsFormatStr())));
            HTMLText += line(textOrange(QObject::tr(" - End : %1 %2").arg(DateUtils::convertToAppSettingsFormatStr(phase->mBeta.mX), DateUtils::getAppSettingsFormatStr())));
            HTMLText += line(textOrange(QObject::tr(" - Tau : %1").arg(stringForLocal(phase->mTau.mX))));
            HTMLText += "<hr>";

        }
    }

    if (model->mPhaseConstraints.size() > 0) {
        HTMLText += "<br>";
        HTMLText += textBold(textGreen(QObject::tr("Phases Constraints"))) + "<br>";
        HTMLText += "<hr>";

        int i = 0;
        for (auto& constraint : model->mPhaseConstraints) {
            ++i;
            HTMLText += line(textGreen(QObject::tr("Succession ( %1 / %2) : from %3 to %4").arg(QString::number(i), QString::number(model->mPhaseConstraints.size()),constraint->mPhaseFrom->getQStringName(), constraint->mPhaseTo->getQStringName())));
            HTMLText += line(textGreen(QObject::tr(" - Gamma : %1").arg(stringForLocal(constraint->mGamma))));
            //HTMLText += "<br>";
        }
    }

    if (curveModel) {
        HTMLText += "<hr><br>";
        HTMLText += textBold(textGreen(QObject::tr("Curve"))) + "<br>";
        HTMLText += "<hr>";
        if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe) {
            HTMLText +=  line(textGreen(QObject::tr("Fixed Smoothing : 10E%1").arg(QLocale().toString(log10(model->mLambdaSpline.mX), 'G', 2))));

        } else {
            HTMLText +=  line(textGreen(QObject::tr("Log10 Smoothing : %1").arg(QLocale().toString(log10(model->mLambdaSpline.mX), 'G', 2))));
            if (model->mLambdaSpline.mLastAccepts.size() > 2) {
                const auto acceptRate = model->mLambdaSpline.getCurrentAcceptRate();
                const auto samplerType = model->mLambdaSpline.mSamplerProposal;
                if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                    HTMLText += line(textGreen(QObject::tr(". . . . . Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
                else
                    HTMLText += line(textGreen(QObject::tr(". . . . . Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.)+" %"))  + " (" + MHVariable::getSamplerProposalText(samplerType)) + ")");

               HTMLText +=  line(textGreen(QObject::tr(" - Sigma_MH on Smoothing : %1").arg(stringForLocal(model->mLambdaSpline.mSigmaMH))));
            }
        }
        HTMLText += "<br>";
        HTMLText +=  line(textGreen(QObject::tr("sqrt Curve Shrinkage : %1").arg(QLocale().toString(sqrt(model->mS02Vg.mX), 'G', 2))));
        if (model->mS02Vg.mLastAccepts.size() > 2) {
            const auto acceptRate = model->mS02Vg.getCurrentAcceptRate();
            const auto samplerType = model->mS02Vg.mSamplerProposal;
            if (acceptRate < 0.46 &&  acceptRate > 0.42 )
                HTMLText += line(textGreen(QObject::tr(". . . . . Current Acceptance Rate : %1 % (%2)").arg(stringForLocal(acceptRate*100.), MHVariable::getSamplerProposalText(samplerType))));
            else
                HTMLText += line(textGreen(QObject::tr(". . . . . Current Acceptance Rate : ") + textBold(textRed(stringForLocal(acceptRate*100.)+" %"))  + " ("+ MHVariable::getSamplerProposalText(samplerType)) + ")");

            HTMLText +=  line(textGreen(QObject::tr(" - Sigma_MH on Shrinkage param. : %1").arg(stringForLocal(model->mS02Vg.mSigmaMH))));
        }
        HTMLText += "<br>";
        HTMLText +=  line(textGreen(QObject::tr("Curve beta parameter : %1").arg(QLocale().toString(model->mSO2_beta, 'G', 2))));
    }

    return HTMLText;
}



QString ModelUtilities::dateResultsHTML(const Date* d, const std::shared_ptr<ModelCurve> &model)
{
    Q_ASSERT(d);
    QString text = line(textBold(textBlack(QObject::tr("Data : %1").arg(d->getQStringName())))) + "<br>";
    text += line(textBold(textBlack(QObject::tr("Posterior calib. date"))));

    if (d->mTi.mSamplerProposal != MHVariable::eFixe && model != nullptr) {

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
    text += line(textBlack(d->mTi.resultsString("", DateUtils::getAppSettingsFormatStr()))) ;

    return text;
}

QString ModelUtilities::dateResultsHTML(const Date* d, const double tmin_formated, const double tmax_formated)
{
    Q_ASSERT(d);
    QString text = line(textBold(textBlack(QObject::tr("Data : %1").arg(d->getQStringName())))) + "<br>";
    text += line(textBold(textBlack(QObject::tr("Posterior calib. date"))));

    if (d->mTi.mSamplerProposal != MHVariable::eFixe) {

        short position = ModelUtilities::HPDOutsideSudyPeriod(d->mTi.mFormatedHPD, tmin_formated, tmax_formated);
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
    text += line(textBlack(d->mTi.resultsString("", DateUtils::getAppSettingsFormatStr()))) ;

    text +=  "<br>" + line(textBold(textBlack(QObject::tr("Posterior Std ti"))));
    text += line(textBlack(d->mSigmaTi.resultsString()));

    return text;
}


QString ModelUtilities::sigmaTiResultsHTML(const Date* d)
{
    Q_ASSERT(d);
    QString text;
    text += line(textBold(textBlack(QObject::tr("Data : %1").arg(d->getQStringName())))) + "<br>";

    text += line(textBold(textBlack(QObject::tr("Posterior Std ti"))));
    text += line(textBlack(d->mSigmaTi.resultsString()));
    return text;
}

QString ModelUtilities::eventResultsHTML(const std::shared_ptr<Event> e, const bool withDates, const std::shared_ptr<ModelCurve> model)
{
    Q_ASSERT(e);
    QString text;
    if (e->mType == Event::eBound) {
        text += line(textBold(textRed(QObject::tr("Bound : %1").arg(e->getQStringName())))) + "<br>";
        text += line(textBold(textRed(QObject::tr("Posterior Bound Date"))));
        text += line(textRed(e->mTheta.resultsString("", DateUtils::getAppSettingsFormatStr())));

    }
    else {
        text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->getQStringName())))) + "<br>";
        text += line(textBold(textBlue(QObject::tr("Posterior Event Date"))));
        text += line(textBlue(e->mTheta.resultsString("", DateUtils::getAppSettingsFormatStr())));


        if (withDates) {
            text += "<br>" + EventS02ResultsHTML(e); // Pour version 4
            for (auto&& date : e->mDates)
                text += "<br><br>" + dateResultsHTML(&(date), model);
        }

    }

    if (model && model->is_curve) {
        if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
            text = "<br>" + line(textBold(textGreen(QObject::tr("Curve : Std gi"))));
            text += line(textGreen(QObject::tr("Fixed value : %1").arg(QString::number(e->mVg.mRawTrace->at(0)))));

        } else {
            text += "<br>" + line(textBold(textGreen(QObject::tr("Curve : Posterior Std gi"))));
            text += line(textGreen(e->mVg.resultsString("", nullptr)));
        }
    }
    return text;
}

QString ModelUtilities::eventResultsHTML(const std::shared_ptr<Event> e, const bool withDates, const double tmin_formated, const double tmax_formated, bool with_curve)
{
    Q_ASSERT(e);
    QString text;
    if (e->mType == Event::eBound) {
        text += line(textBold(textRed(QObject::tr("Bound : %1").arg(e->getQStringName())))) + "<br>";
        text += line(textBold(textRed(QObject::tr("Posterior Bound Date"))));
        text += line(textRed(e->mTheta.resultsString("", DateUtils::getAppSettingsFormatStr())));

    }
    else {
        text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->getQStringName())))) + "<br>";
        text += line(textBold(textBlue(QObject::tr("Posterior Event Date"))));
        text += line(textBlue(e->mTheta.resultsString("", DateUtils::getAppSettingsFormatStr())));


        if (withDates) {
            //text += "<br>" +EventS02ResultsHTML(e); Pour version 4
            for (auto&& date : e->mDates)
                text += "<br><br>" + dateResultsHTML(&(date), tmin_formated, tmax_formated);
        }

    }

    if (with_curve) {
        if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
            text = "<br>" + line(textBold(textGreen(QObject::tr("Curve : Std gi"))));
            text += line(textGreen(QObject::tr("Fixed value : %1").arg(QString::number(e->mVg.mRawTrace->at(0)))));

        } else {
            text += "<br>" + line(textBold(textGreen(QObject::tr("Curve : Posterior Std gi"))));
            text += line(textGreen(e->mVg.resultsString("", nullptr)));
        }
    }
    return text;
}

QString ModelUtilities::EventS02ResultsHTML(const std::shared_ptr<Event> e)
{
    Q_ASSERT(e);
    QString text;

    if (e->mS02Theta.mSamplerProposal == MHVariable::eFixe) {
        text = line(textBold(textBlue(QObject::tr("Event Shrinkage = harmonic mean"))));

    } else {
        text += line(textBold(textBlue(QObject::tr("Posterior Event Shrinkage"))));
        text += line(textBlue(e->mS02Theta.resultsString("", nullptr)));
    }
    return text;
}

QString ModelUtilities::VgResultsHTML(const std::shared_ptr<Event> e)
{
    Q_ASSERT(e);
    QString text;

    if (e->mType == Event::eBound) {
        text += line(textBold(textRed(QObject::tr("Bound : %1").arg(e->getQStringName()))));

    } else {
        text += line(textBold(textBlue(QObject::tr("Event : %1").arg(e->getQStringName()))));
    }

    if (e->mVg.mSamplerProposal == MHVariable::eFixe) {
        text = line(textBold(textGreen(QObject::tr("Curve : Std gi"))));
        text += line(textGreen(QObject::tr("Fixed value : %1").arg(QString::number(e->mVg.mRawTrace->at(0)))));

    } else {
        text += line(textBold(textGreen(QObject::tr("Curve : Posterior Std gi"))));
        text += line(textGreen(e->mVg.resultsString("", nullptr)));
    }
    return text;
}

QString ModelUtilities::phaseResultsHTML(const std::shared_ptr<Phase> p)
{
    QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->getQStringName()))));
    text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("Begin (posterior distrib.)"))));
    text += line(textOrange(p->mAlpha.resultsString("", DateUtils::getAppSettingsFormatStr())));

    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("End (posterior distrib.)"))));
    text += line(textOrange(p->mBeta.resultsString("", DateUtils::getAppSettingsFormatStr())));

    if (p->mTimeRange != std::pair<double, double>(- INFINITY, +INFINITY)) {
        text += "<br>";
        // we suppose it's the same mThreshohdUsed than alpha
        const QString result = QObject::tr("Phase Time Range") + QString(" ( %1 %) : [ %2 ; %3 ]").arg(stringForLocal(p->mAlpha.mThresholdUsed),
                                                                                            stringForLocal(p->getFormatedTimeRange().first),
                                                                                            stringForLocal(p->getFormatedTimeRange().second));
        text += line(textBold(textOrange(result)));
    }

    return text;
}

QString ModelUtilities::durationResultsHTML(const std::shared_ptr<Phase> p)
{
   QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->getQStringName()))));
   text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

   text += "<br>";
   text += line(textBold(textOrange(QObject::tr("Duration (posterior distrib.)"))));
   text += line(textOrange(p->mDuration.resultsString(QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"), QObject::tr("Years"))));

   return text;
}

QString ModelUtilities::tempoResultsHTML(const std::shared_ptr<Phase> p)
{
    Q_ASSERT(p);
    QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->getQStringName()))));
    text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    return text;
}

QString ModelUtilities::activityResultsHTML(const std::shared_ptr<Phase> p)
{
    QString text = line(textBold(textOrange(QObject::tr("Phase : %1").arg(p->getQStringName()))));
    text += line(textOrange(QObject::tr("Number of Events : %1").arg(p->mEvents.size())));

    text += "<br>";

    if (p->mEvents.size()<2) {
        text += line(textOrange("<i>" + QObject::tr("No Stat.")  + "</i>"));
        return text;
    }

    double t1 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("t_min"));
    double t2 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("t_max"));

    if (t1>t2)
        std::swap(t1, t2);
    text += line(textOrange("<i>" + QObject::tr("Trace Stat.")  + "</i>"));
    text += line(textOrange("Theta min = " + stringForLocal(t1) + " " + DateUtils::getAppSettingsFormatStr()));
    text += line(textOrange("Theta max = " + stringForLocal(t2) + " " + DateUtils::getAppSettingsFormatStr()));

    text += "<hr>";
    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("Activity"))));
    t1 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_TimeRange_min"));
    t2 = DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_TimeRange_max"));

    if (t1>t2)
        std::swap(t1, t2);

    text += line(textOrange(QObject::tr("Activity Time Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg( stringForLocal(p->mValueStack.at("Activity_TimeRange_Level")),
                                                                                        stringForLocal(t1),
                                                                                        stringForLocal(t2),
                                                                                        DateUtils::getAppSettingsFormatStr())));

    text += line(textOrange("Activity Span = " + stringForLocal(t2-t1) ));
    text += line(textOrange("Activity Theta mean = " + stringForLocal(DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_mean95"))) + " " + DateUtils::getAppSettingsFormatStr()));
    text += line(textOrange("Activity Theta std = " + stringForLocal(p->mValueStack.at("Activity_std95")) ));

    text += "<br>";
    text += line(textBold(textOrange(QObject::tr("h Estimation"))));

    QString textValue = stringForLocal(p->mValueStack.at("Activity_Significance_Score"), true);
    const double threshold = p->mValueStack.at("Activity_Threshold");
    const auto hActi = p->mValueStack.at("Activity_h");
    text += line(textOrange("Significance Score"  + QString(" ( %1 %) = ").arg(threshold) + textValue + QString(" with h = %1").arg(hActi)));

    text += line(""); 
    text += line(textOrange("Activity max"  + QString(" = %1").arg(p->mValueStack.at("Activity_max")) ));
    text += line(textOrange("Activity mode"  + QString(" = %1").arg(DateUtils::convertToAppSettingsFormat(p->mValueStack.at("Activity_mode"))) + " " + DateUtils::getAppSettingsFormatStr() )) ;

    const double hUnif = (3.686*p->mValueStack.at("Activity_std95"))/pow(p->mEvents.size(), 1./5.);
    text += "<br>";
    text += line(textOrange("Optimal h (Uniform kernel) = " + stringForLocal(hUnif)));

    return text;
}

QString ModelUtilities::constraintResultsHTML(const std::shared_ptr<PhaseConstraint> p)
{
    QString text = line(textBold(textOrange(QObject::tr("Succession : from %1 to %2").arg(p->mPhaseFrom->getQStringName(), p->mPhaseTo->getQStringName()))));

    switch(p->mGammaType) {
    case PhaseConstraint::eGammaFixed :
        text += textOrange(QObject::tr("Min Hiatus fixed = %1").arg(p->mGammaFixed));
        break;
    case PhaseConstraint::eGammaUnknown :
        text += textOrange(QObject::tr("Min Hiatus unknown")) ;
        break;
    case PhaseConstraint::eGammaRange : // no longer used
        text += textOrange(QObject::tr("Min Hiatus between %1 and %2").arg(p->mGammaMin, p->mGammaMax));
        break;
    default:

        break;
    }

    if (p->mTransitionRange != QPair<double,double>()) {
        text += "<br>";
        // we suppose it's the same mThreshohdUsed than alpha
        const QString result = textOrange(QObject::tr("Transition Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                                            stringForLocal(p->getFormatedTransitionRange().first),
                                                                                            stringForLocal(p->getFormatedTransitionRange().second),
                                                                                            DateUtils::getAppSettingsFormatStr()));

        text += line(textOrange(result));
    }

    if (p->mGapRange != QPair<double, double>()) {
        QString result;
        if (std::isinf(p->getFormatedGapRange().first) || std::isinf(p->getFormatedGapRange().second))
           result = textOrange(QObject::tr("No Gap") );

        else
            result = textOrange(QObject::tr("Gap Range") + QString(" ( %1 %) : [ %2 ; %3 ] %4").arg(stringForLocal(p->mPhaseFrom->mAlpha.mThresholdUsed),
                                                                           stringForLocal(p->getFormatedGapRange().first),
                                                                           stringForLocal(p->getFormatedGapRange().second),
                                                                           DateUtils::getAppSettingsFormatStr()));

        text += line(textOrange(result + "<br>"));
    }

    return text;
}

QString ModelUtilities::curveResultsHTML(const std::shared_ptr<ModelCurve> model)
{
    QString text;

    text += line( textBold(textGreen(QObject::tr("Curve"))) );
    //text += "<br>";
    text += line(textGreen(QObject::tr(" - Number of Ref. points = %1").arg(model->mEvents.size())));

    if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe  && model->mCurveSettings.mLambdaSplineType != CurveSettings::eInterpolation) {
        text += line(textGreen(QObject::tr("Lambda Spline; Fixed value = 10E%1").arg(QString::number(model->mLambdaSpline.mRawTrace->at(0)))));

    } else if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe && model->mCurveSettings.mLambdaSplineType == CurveSettings::eInterpolation) {
        text += line(textGreen(QObject::tr("- Lambda Spline; Interpolation Fixed value = %1").arg(QString::number(0))));

    }else {
        //text += line(textGreen(QObject::tr("- Mean of the log10 of Smoothing = %1").arg(stringForLocal(model->mLambdaSpline.mResults.funcAnalysis.mean))));
        text += "<br>";
        text += lambdaResultsHTML(model);
    }

    if (model->mS02Vg.mSamplerProposal == MHVariable::eFixe) {
        text += line(textGreen(QObject::tr("- Curve Shrinkage ; Fixed value = %1").arg(QString::number(pow( model->mS02Vg.mRawTrace->at(0), 2.)))));

    } else {
        //text += line(textGreen(QObject::tr("- Mean of the sqrt of Curve Shrinkage = %1").arg(stringForLocal(model->mS02Vg.mResults.funcAnalysis.mean))));
        text += "<br>";
        text += S02ResultsHTML(model);
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

QString ModelUtilities::lambdaResultsHTML(const std::shared_ptr<ModelCurve> model)
{
    QString text;
    if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe  && model->mCurveSettings.mLambdaSplineType != CurveSettings::eInterpolation) {
        text = line(textBold(textGreen(QObject::tr("Smoothing"))));
        text += line(textGreen(QObject::tr("Fixed value : 10E%1").arg(QString::number( model->mLambdaSpline.mRawTrace->at(0)))));

    } else if (model->mLambdaSpline.mSamplerProposal == MHVariable::eFixe && model->mCurveSettings.mLambdaSplineType == CurveSettings::eInterpolation) {
        text = line(textBold(textGreen(QObject::tr("Smoothing"))));
        text += line(textGreen(QObject::tr("Interpolation Fixed value : %1").arg(QString::number(0))));

    }else {
        text = line(textBold(textGreen(QObject::tr("Stat. on the log10 of Smoothing"))));
        text += line(textGreen(model->mLambdaSpline.resultsString("", nullptr)));
    }
    return text;
}

QString ModelUtilities::S02ResultsHTML(const std::shared_ptr<ModelCurve> model)
{
    QString text;
    if (model->mS02Vg.mSamplerProposal == MHVariable::eFixe) {
        text = line(textBold(textGreen(QObject::tr("S02 Vg"))));
        text += line(textGreen(QObject::tr("Fixed value : %1").arg(QString::number(pow( model->mS02Vg.mRawTrace->at(0), 2.)))));

    } else {
        text = line(textBold(textGreen(QObject::tr("Stat. on the sqrt(S02 Vg)"))));
        text += line(textGreen(model->mS02Vg.resultsString("", nullptr)));
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
short ModelUtilities::HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const std::shared_ptr<ModelCurve> model)
{
    QMap<double, double>::const_iterator iter(hpd.constBegin());
    short answer = 0;
    const double tmin = model->mSettings.getTminFormated();
    const double tmax = model->mSettings.getTmaxFormated();
    // We suppose QMap is sort <
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

short ModelUtilities::HPDOutsideSudyPeriod(const std::map<double, double> &hpd, const std::shared_ptr<ModelCurve> model)
{
    std::map<double, double>::const_iterator iter(hpd.begin());
    short answer = 0;
    const double tmin = model->mSettings.getTminFormated();
    const double tmax = model->mSettings.getTmaxFormated();
    // We suppose QMap is sort <
    while (iter != hpd.end()) {
        const double v = iter->second;
        if (v > 0) {
            const double t = iter->first;
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

short ModelUtilities::HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const double tmin_formated, const double tmax_formated)
{
    QMap<double, double>::const_iterator iter(hpd.constBegin());
    short answer = 0;
    // We suppose QMap is sort <
    while (iter != hpd.constEnd()) {
        const double v = iter.value();
        if (v > 0) {
            const double t = iter.key();
            if (t<tmin_formated)
                answer = -1;

            else if (t>tmax_formated && answer == -1)
                return 2;

            else if (t>tmax_formated)
                return 1;

        }

        ++iter;
    }
    return answer;
}

short ModelUtilities::HPDOutsideSudyPeriod(const std::map<double, double> &hpd, const double tmin_formated, const double tmax_formated)
{
    std::map<double, double>::const_iterator iter(hpd.begin());
    short answer = 0;
    // We suppose QMap is sort <
    while (iter != hpd.end()) {
        const double v = iter->second;
        if (v > 0) {
            const double t = iter->first;
            if (t<tmin_formated)
                answer = -1;

            else if (t>tmax_formated && answer == -1)
                return 2;

            else if (t>tmax_formated)
                return 1;

        }

        ++iter;
    }
    return answer;
}


double sample_in_repartition (std::shared_ptr<CalibrationCurve> calibrateCurve, const double min, const double max)
{
    const double unionTmin = calibrateCurve->mTmin;
    const double unionTmax = calibrateCurve->mTmax;
    const double unionStep = (unionTmax-unionTmin)/ (calibrateCurve->mRepartition.size()-1);

    // Early exit conditions: completely outside range
    if (unionTmax < min || max < unionTmin) {
        return Generator::gaussByDoubleExp(
            (unionTmax + unionTmin) / 2.,
            std::max(unionStep, (unionTmax - unionTmin) / 2.),
            min, max
            );
    }

     else {
        // Interpolate repartition values
        double minRepartition = calibrateCurve->repartition_interpolate(std::max(unionTmin, min));
        double maxRepartition = calibrateCurve->repartition_interpolate(std::min(unionTmax, max));
        const int rep_idx_max = static_cast<int>(calibrateCurve->mRepartition.size()) - 1;


        // Check for zero density region. We are between two peaks, the density is zero.
        if (minRepartition >= maxRepartition) {
           // qDebug() <<" [sample_in_repartition] minRepartition >= maxRepartition"<<minRepartition<<maxRepartition<< std::max(unionTmin, min)<<std::min(unionTmax, max);
            return Generator::randomUniform(min, max);
        }
        // Generate random value in repartition range
        const double value = Generator::randomUniform(minRepartition, maxRepartition);

        auto clampProp = [&](double val) {
            return std::clamp(
                (val - unionTmin) / (unionTmax - unionTmin) * rep_idx_max,
                0.,
                static_cast<double>(rep_idx_max)
                );
        };

        const int idxUnder = floor(clampProp(min));
        const int idxUpper = floor(clampProp(max));

        double idx = vector_interpolate_idx_for_value(value, calibrateCurve->mRepartition, idxUnder, idxUpper);

        double t = unionTmin + idx * unionStep;
        // Du fait de l'arrondi de idxUnder et idxUpper, la dichotomie peut donner une valeur de t en dehors de l'intervale.
        // Cela arrive souvent quant l'espace entre les deux index est faible


        // Final range validation
        if (t >= min && t <= max) {
            return t;
        }

#ifdef DEBUG
        if (t > max) {
            qDebug() <<" [sample_in_repartition] t>max"<<t<<max<<"[sample_in_repartition] Generator::randomUniform(min, max)";

        } else if (t<min) {
            qDebug() <<" [sample_in_repartition] t<min"<<t<<min<<"[sample_in_repartition] Generator::randomUniform(min, max)";

        }
#endif
        // Fallback to uniform sampling if interpolation fails
        return Generator::randomUniform(min, max);

    }
}


void sampleInCumulatedRepartition_thetaFixe (std::shared_ptr<Event> event, const StudyPeriodSettings& settings)
{
    // Création de la distribution cumulative des dates
    auto calib =  generate_mixingCalibration(event->mDates, "Mixing Theta Fixed");

    // Utilisation de références pour éviter des copies inutiles
    const auto& repartition = calib.mRepartition;

    const double maxRepartition = *repartition.crbegin();
    const double minRepartition = *repartition.begin();

    const long double t_min_long = calib.mTmin;
    const long double t_max_long = calib.mTmax;

    // Vérification de la taille de la répartition
    const size_t repartitionSize = repartition.size();
    if (repartitionSize > 1) {
        const long double step_long = (t_max_long - t_min_long) / (repartitionSize - 1);

        // Calcul de l'indice d'interpolation
        const double targetValue = 0.5 * (maxRepartition - minRepartition) + minRepartition;
        const double idx = vector_interpolate_idx_for_value(targetValue, repartition);
        event->mTheta.mX = t_min_long + idx * step_long;

    } else {
        event->mTheta.mX = Generator::randomUniform(settings.mTmin, settings.mTmax);
    }

}

std::string html_to_plain_text(const std::string &html)
{
    // Remplace les balises <br> et <p> par des retours à la ligne
    std::string text = std::regex_replace(html, std::regex("<br\\s*/?>"), "\n");

    // Remplace <p> par un retour à la ligne
    text = std::regex_replace(text, std::regex("<p[^>]*>"), "\n");

    // Supprime les balises HTML
    text = std::regex_replace(text, std::regex("<[^>]*>"), " ");

    // Remplace les entités HTML par leurs caractères correspondants
    std::unordered_map<std::string, std::string> htmlEntities = {
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&amp;", "&"},
        {"&quot;", "\""},
        {"&apos;", "'"},
        // Ajoutez d'autres entités HTML si nécessaire
    };

    for (const auto& entity : htmlEntities) {
        size_t pos = 0;
        while ((pos = text.find(entity.first, pos)) != std::string::npos) {
            text.replace(pos, entity.first.length(), entity.second);
            pos += entity.second.length();
        }
    }

    // Supprime les espaces supplémentaires tout en préservant les retours à la ligne
    text = std::regex_replace(text, std::regex("[ ]+"), " ");  // Remplace les espaces multiples par un seul espace
    text = std::regex_replace(text, std::regex("\n +"), "\n"); // Supprime les espaces après un retour à la ligne
    text = std::regex_replace(text, std::regex(" +\n"), "\n"); // Supprime les espaces avant un retour à la ligne

    // Supprime les espaces en début et fin de chaîne
    text = std::regex_replace(text, std::regex("^\\s+|\\s+$"), "");

    return text;
}


QString html_to_plain_text(const QString &html)
{
   return QString::fromStdString(html_to_plain_text(html.toStdString()));
}
