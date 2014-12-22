#include "Model.h"
#include "Date.h"
#include "Project.h"
#include "EventKnown.h"
#include "MCMCLoopMain.h"
#include "MCMCProgressDialog.h"
#include "ModelUtilities.h"
#include <QJsonArray>
#include <QtWidgets>


Model::Model():QObject()
{
    
}

/*Model::Model(const Model& model):QObject()
{
    copyFrom(model);
}

Model& Model::operator=(const Model& model)
{
    copyFrom(model);
    return *this;
}

void Model::copyFrom(const Model& model)
{
    mSettings = model.mSettings;
    mMCMCSettings = model.mMCMCSettings;
    
    mEvents = model.mEvents;
    mPhases = model.mPhases;
    mEventConstraints = model.mEventConstraints;
    mPhaseConstraints = model.mPhaseConstraints;
}*/

Model::~Model()
{
    
}

bool sortEvents(Event* e1, Event* e2)
{
    return (e1->mItemY < e2->mItemY);
}

Model* Model::fromJson(const QJsonObject& json)
{
    Model* model = new Model();
    
    if(json.contains(STATE_SETTINGS))
    {
        QJsonObject settings = json[STATE_SETTINGS].toObject();
        model->mSettings = ProjectSettings::fromJson(settings);
    }
    
    if(json.contains(STATE_MCMC))
    {
        QJsonObject mcmc = json[STATE_MCMC].toObject();
        model->mMCMCSettings = MCMCSettings::fromJson(mcmc);
    }
    
    if(json.contains(STATE_PHASES))
    {
        QJsonArray phases = json[STATE_PHASES].toArray();
        for(int i=0; i<phases.size(); ++i)
        {
            QJsonObject phase = phases[i].toObject();
            Phase* p = new Phase(Phase::fromJson(phase));
            model->mPhases.append(p);
        }
    }
    
    if(json.contains(STATE_EVENTS))
    {
        QJsonArray events = json[STATE_EVENTS].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                Event* e = new Event(Event::fromJson(event));
                model->mEvents.append(e);
            }
            else
            {
                EventKnown* e = new EventKnown(EventKnown::fromJson(event));
                e->updateValues(model->mSettings.mTmin, model->mSettings.mTmax, model->mSettings.mStep);
                model->mEvents.append(e);
            }
        }
    }
    
    // Sort events based on items y position
    std::sort(model->mEvents.begin(), model->mEvents.end(), sortEvents);
    
    if(json.contains(STATE_EVENTS_CONSTRAINTS))
    {
        QJsonArray constraints = json[STATE_EVENTS_CONSTRAINTS].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            EventConstraint* c = new EventConstraint(EventConstraint::fromJson(constraint));
            model->mEventConstraints.append(c);
        }
    }
    
    if(json.contains(STATE_PHASES_CONSTRAINTS))
    {
        QJsonArray constraints = json[STATE_PHASES_CONSTRAINTS].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            qDebug() << constraint;
            PhaseConstraint* c = new PhaseConstraint(PhaseConstraint::fromJson(constraint));
            model->mPhaseConstraints.append(c);
        }
    }
    
    // ------------------------------------------------------------
    //  Link objects to each other
    //  Must be done here !
    //  nb : Les data sont déjà linkées aux events à leur création
    // ------------------------------------------------------------
    for(int i=0; i<model->mEvents.size(); ++i)
    {
        int eventId = model->mEvents[i]->mId;
        QList<int> phasesIds = model->mEvents[i]->mPhasesIds;
        
        // Link des events / phases
        for(int j=0; j<model->mPhases.size(); ++j)
        {
            int phaseId = model->mPhases[j]->mId;
            if(phasesIds.contains(phaseId))
            {
                model->mEvents[i]->mPhases.append(model->mPhases[j]);
                model->mPhases[j]->mEvents.append(model->mEvents[i]);
            }
        }
        
        // Link des events / contraintes d'event
        for(int j=0; j<model->mEventConstraints.size(); ++j)
        {
            if(model->mEventConstraints[j]->mFromId == eventId)
            {
                model->mEventConstraints[j]->mEventFrom = model->mEvents[i];
                model->mEvents[i]->mConstraintsFwd.append(model->mEventConstraints[j]);
            }
            else if(model->mEventConstraints[j]->mToId == eventId)
            {
                model->mEventConstraints[j]->mEventTo = model->mEvents[i];
                model->mEvents[i]->mConstraintsBwd.append(model->mEventConstraints[j]);
            }
        }
    }
    // Link des phases / contraintes de phase
    for(int i=0; i<model->mPhases.size(); ++i)
    {
        int phaseId = model->mPhases[i]->mId;
        qDebug() << "Phase " << phaseId;
        for(int j=0; j<model->mPhaseConstraints.size(); ++j)
        {
            qDebug() << "constr. from " << model->mPhaseConstraints[j]->mFromId;
            qDebug() << "constr. to " << model->mPhaseConstraints[j]->mToId;
            
            if(model->mPhaseConstraints[j]->mFromId == phaseId)
            {
                model->mPhaseConstraints[j]->mPhaseFrom = model->mPhases[i];
                model->mPhases[i]->mConstraintsFwd.append(model->mPhaseConstraints[j]);
            }
            else if(model->mPhaseConstraints[j]->mToId == phaseId)
            {
                model->mPhaseConstraints[j]->mPhaseTo = model->mPhases[i];
                model->mPhases[i]->mConstraintsBwd.append(model->mPhaseConstraints[j]);
            }
        }
    }
    
    
    
    qDebug() << "===========================================";
    qDebug() << "MODEL CREATED";
    qDebug() << "===========================================";
    qDebug() << "=> Events : " << model->mEvents.size();
    for(int i=0; i<model->mEvents.size(); ++i)
    {
        QString objType = "Event";
        if(model->mEvents[i]->type() == Event::eKnown)
            objType = "Bound";
            
        qDebug() << "  => " << objType << " (id: " << model->mEvents[i]->mId << ", name: "<< model->mEvents[i]->mName <<") : " << model->mEvents[i]->mPhases.size() << " phases"
            << ", " << model->mEvents[i]->mDates.size() << " dates"
            << ", " << model->mEvents[i]->mConstraintsBwd.size() << " const. back."
            << ", " << model->mEvents[i]->mConstraintsFwd.size() << " const. fwd.";
    }
    qDebug() << "=> Phases : " << model->mPhases.size();
    for(int i=0; i<model->mPhases.size(); ++i)
    {
        qDebug() << "  => Phase " << model->mPhases[i]->mId << " : " << model->mPhases[i]->mEvents.size() << " events"
            << " : " << model->mPhases[i]->mConstraintsBwd.size() << " const. back."
            << " : " << model->mPhases[i]->mConstraintsFwd.size() << " const. fwd.";
    }
    qDebug() << "=> Event Constraints : " << model->mEventConstraints.size();
    for(int i=0; i<model->mEventConstraints.size(); ++i)
    {
        qDebug() << "  => E. Const. " << model->mEventConstraints[i]->mId
            << " : event " << model->mEventConstraints[i]->mEventFrom->mId << "(" + model->mEventConstraints[i]->mEventFrom->mName + ")"
            << " to " << model->mEventConstraints[i]->mEventTo->mId << "(" + model->mEventConstraints[i]->mEventTo->mName + ")";
    }
    qDebug() << "=> Phase Constraints : " << model->mPhaseConstraints.size();
    for(int i=0; i<model->mPhaseConstraints.size(); ++i)
    {
        qDebug() << "  => P. Const. " << model->mPhaseConstraints[i]->mId
        << " : phase " << model->mPhaseConstraints[i]->mPhaseFrom->mId
        << " to " << model->mPhaseConstraints[i]->mPhaseTo->mId;
    }
    qDebug() << "===========================================";
    
    return model;
}

QJsonObject Model::toJson() const
{
    QJsonObject json;
    
    json["settings"] = mSettings.toJson();
    json["mcmc"] = mMCMCSettings.toJson();
    
    QJsonArray events;
    for(int i=0; i<mEvents.size(); ++i)
        events.append(mEvents[i]->toJson());
    json["events"] = events;
    
    QJsonArray phases;
    for(int i=0; i<mPhases.size(); ++i)
        phases.append(mPhases[i]->toJson());
    json["phases"] = phases;
    
    QJsonArray event_constraints;
    for(int i=0; i<mEventConstraints.size(); ++i)
        event_constraints.append(mEventConstraints[i]->toJson());
    json["event_constraints"] = event_constraints;
    
    QJsonArray phase_constraints;
    for(int i=0; i<mPhaseConstraints.size(); ++i)
        phase_constraints.append(mPhaseConstraints[i]->toJson());
    json["phase_constraints"] = phase_constraints;
    
    return json;
}

bool Model::isValid()
{
    // 1 - Au moins 1 fait dans le modèle
    if(mEvents.size() == 0)
        throw tr("At least one event is required");
    
    // 2 - Au moins 1 data dans chaque event
    for(int i=0; i<mEvents.size(); ++i)
    {
        if(mEvents[i]->type() == Event::eDefault)
        {
            if(mEvents[i]->mDates.size() == 0)
                throw tr(" The event") + " \"" + mEvents[i]->mName + "\" " + tr("must contain at least 1 data");
        }
    }
    
    // 3 - Au moins 1 event dans chaque phase
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i]->mEvents.size() == 0)
            throw tr("The phase") + " \"" + mPhases[i]->mName + "\" " + tr("must contain at least 1 event");
    }
    
    // 4 - Pas de circularité sur les contraintes de faits
    QVector<QVector<Event*>> eventBranches;
    try{
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);
    }catch(QString error){
        throw error;
    }
    
    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    QVector<QVector<Phase*>> phaseBranches;
    try{
        phaseBranches = ModelUtilities::getAllPhasesBranches(mPhases, mSettings.mTmax - mSettings.mTmin);
    }catch(QString error){
        throw error;
    }
    
    // 7 - Un fait ne paut pas appartenir à 2 phases en contrainte
    for(int i=0; i<phaseBranches.size(); ++i)
    {
        QVector<Event*> branchEvents;
        for(int j=0; j<phaseBranches[i].size(); ++j)
        {
            Phase* phase = phaseBranches[i][j];
            for(int k=0; k<phase->mEvents.size(); ++k)
            {
                if(!branchEvents.contains(phase->mEvents[k]))
                {
                    branchEvents.append(phase->mEvents[k]);
                    qDebug() << phase->mEvents[k]->mName << " in " << phase->mName;
                }
                else
                    throw QString("The event \"" + phase->mEvents[k]->mName + "\" cannot belong to several phases in a same branch!");
            }
        }
    }
    
    // 8 - Bounds : verifier cohérence des bornes en fonction des contraintes de faits (page 2)
    //  => Modifier les bornes des intervalles des bounds !! (juste dans le modèle servant pour le calcul)
    for(int i=0; i<eventBranches.size(); ++i)
    {
        for(int j=0; j<eventBranches[i].size(); ++j)
        {
            Event* event = eventBranches[i][j];
            if(event->mType == Event::eKnown)
            {
                EventKnown* bound = dynamic_cast<EventKnown*>(event);
                
                // --------------------
                // Check bound interval lower value
                // --------------------
                
                // On vérifie toutes les bornes avant et on prend le max
                // de leurs valeurs fixes ou du début de leur intervalle :
                float lower = mSettings.mTmin;
                for(int k=0; k<j; ++k)
                {
                    Event* evt = eventBranches[i][k];
                    if(evt->mType == Event::eKnown)
                    {
                        EventKnown* bd = dynamic_cast<EventKnown*>(evt);
                        if(bd->mKnownType == EventKnown::eFixed)
                            lower = qMax(lower, bd->mFixed);
                        else if(bd->mKnownType == EventKnown::eUniform)
                            lower = qMax(lower, bd->mUniformStart);
                    }
                }
                // Update bound interval
                if(bound->mKnownType == EventKnown::eFixed && bound->mFixed < lower)
                {
                    throw QString("The bound \"" + bound->mName + "\" has a fixed value inconsistent with previous bounds in chain!");
                }
                else if(bound->mKnownType == EventKnown::eUniform)
                {
                    bound->mUniformStart = qMax(bound->mUniformStart, lower);
                }
                
                // --------------------
                // Check bound interval upper value
                // --------------------
                float upper = mSettings.mTmax;
                for(int k=j+1; k<eventBranches[i].size(); ++k)
                {
                    Event* evt = eventBranches[i][k];
                    if(evt->mType == Event::eKnown)
                    {
                        EventKnown* bd = dynamic_cast<EventKnown*>(evt);
                        if(bd->mKnownType == EventKnown::eFixed)
                            upper = qMin(upper, bd->mFixed);
                        else if(bd->mKnownType == EventKnown::eUniform)
                            upper = qMin(upper, bd->mUniformEnd);
                    }
                }
                // Update bound interval
                if(bound->mKnownType == EventKnown::eFixed && bound->mFixed > upper)
                {
                    throw QString("The bound \"" + bound->mName + "\" has a fixed value inconsistent with next bounds in chain!");
                }
                else if(bound->mKnownType == EventKnown::eUniform)
                {
                    bound->mUniformEnd = qMin(bound->mUniformEnd, upper);
                    if(bound->mUniformStart >= bound->mUniformEnd)
                    {
                        throw QString("The bound \"" + bound->mName + "\" has an inconsistent range with other related bounds!");
                    }
                }
            }
        }
    }
    
    // 9 - Gamma min (ou fixe) entre 2 phases doit être inférieur à la différence entre : le min des sups des intervalles des bornes de la phase suivante ET le max des infs des intervalles des bornes de la phase précédente
    for(int i=0; i<mPhaseConstraints.size(); ++i)
    {
        float gammaMin = 0;
        PhaseConstraint::GammaType gType = mPhaseConstraints[i]->mGammaType;
        if(gType == PhaseConstraint::eGammaFixed)
            gammaMin = mPhaseConstraints[i]->mGammaFixed;
        else if(gType == PhaseConstraint::eGammaRange)
            gammaMin = mPhaseConstraints[i]->mGammaMin;
        
        float lower = mSettings.mTmin;
        Phase* phaseFrom = mPhaseConstraints[i]->mPhaseFrom;
        for(int j=0; j<phaseFrom->mEvents.size(); ++j)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseFrom->mEvents[j]);
            if(bound)
            {
                if(bound->mKnownType == EventKnown::eFixed)
                    lower = qMax(lower, bound->mFixed);
                else if(bound->mKnownType == EventKnown::eUniform)
                    lower = qMax(lower, bound->mUniformStart);
            }
        }
        float upper = mSettings.mTmax;
        Phase* phaseTo = mPhaseConstraints[i]->mPhaseTo;
        for(int j=0; j<phaseTo->mEvents.size(); ++j)
        {
            EventKnown* bound = dynamic_cast<EventKnown*>(phaseTo->mEvents[j]);
            if(bound)
            {
                if(bound->mKnownType == EventKnown::eFixed)
                    upper = qMin(upper, bound->mFixed);
                else if(bound->mKnownType == EventKnown::eUniform)
                    upper = qMin(upper, bound->mUniformEnd);
            }
        }
        if(gammaMin >= (upper - lower))
        {
            throw QString("The constraint between phases \"" + phaseFrom->mName + "\" and \"" + phaseTo->mName + "\" is not consistent with the bounds they contain!");
        }
    }
    
    // 10 - Au sein d'une phase, tau max (ou fixe) doit être supérieur à la différence entre le max des infs des intervalles des bornes et le min des sups des intervalles des bornes.
    //  => Modifier les intervalles des bornes:
    //      - L'inf est le max entre : sa valeur courante ou (le max des infs des intervalles des bornes - tau max ou fixe)
    //      - Le sup est le min entre : sa valeur courante ou (le min des sups des intervalles des bornes + tau max ou fixe)
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        if(mPhases[i]->mTauType != Phase::eTauUnknown)
        {
            float tauMax = mPhases[i]->mTauFixed;
            if(mPhases[i]->mTauType == Phase::eTauRange)
                tauMax = mPhases[i]->mTauMax;
            
            float min = mSettings.mTmin;
            float max = mSettings.mTmax;
            
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                if(mPhases[i]->mEvents[j]->mType == Event::eKnown)
                {
                    EventKnown* bound = dynamic_cast<EventKnown*>(mPhases[i]->mEvents[j]);
                    if(bound)
                    {
                        if(bound->mKnownType == EventKnown::eFixed)
                        {
                            min = qMax(min, bound->mFixed);
                            max = qMin(max, bound->mFixed);
                        }
                        else if(bound->mKnownType == EventKnown::eUniform)
                        {
                            min = qMax(min, bound->mUniformEnd);
                            max = qMin(max, bound->mUniformStart);
                        }
                    }
                }
            }
            if(tauMax < (max - min))
            {
                throw QString("The phase \"" + mPhases[i]->mName + "\" has a duration inconsistent with the bounds it contains!");
            }
            // Modify bounds intervals to match max phase duration
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                if(mPhases[i]->mEvents[j]->mType == Event::eKnown)
                {
                    EventKnown* bound = dynamic_cast<EventKnown*>(mPhases[i]->mEvents[j]);
                    if(bound)
                    {
                        if(bound->mKnownType == EventKnown::eUniform)
                        {
                            bound->mUniformStart = qMax(bound->mUniformStart, max - tauMax);
                            bound->mUniformEnd = qMin(bound->mUniformEnd, min + tauMax);
                            
                            min = qMax(min, bound->mUniformEnd);
                            max = qMin(max, bound->mUniformStart);
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

void Model::generateCorrelations(const QList<Chain>& chains)
{
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents[i];
        event->mTheta.generateCorrelations(chains);
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            date.mTheta.generateCorrelations(chains);
            date.mSigma.generateCorrelations(chains);
        }
    }
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        Phase* phase = mPhases[i];
        phase->mAlpha.generateCorrelations(chains);
        phase->mBeta.generateCorrelations(chains);
    }
}

void Model::generatePosteriorDensities(const QList<Chain>& chains, int fftLen, float hFactor)
{
    float tmin = mSettings.mTmin;
    float tmax = mSettings.mTmax;
    
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents[i];
        
        bool generateEventHistos = true;
        if(event->type() == Event::eKnown)
        {
            EventKnown* ek = dynamic_cast<EventKnown*>(event);
            if(ek && ek->knownType() == EventKnown::eFixed)
            {
                generateEventHistos = false;
                qDebug() << "Nothing todo : this is just a Dirac !";
            }
            qDebug() << "=> Generating post. density for bound " << i << "/" << mEvents.size() << " : " << event->mName;
        }
        else
            qDebug() << "=> Generating post. density for event " << i << "/" << mEvents.size() << " : " << event->mName;
        
        if(generateEventHistos)
        {
            event->mTheta.generateHistos(chains, fftLen, hFactor, tmin, tmax);
        }
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
            qDebug() << " -> Generate post. density for date " << j << "/" << event->mDates.size() << " : " << date.mName;
            
            date.mTheta.generateHistos(chains, fftLen, hFactor, tmin, tmax);
            date.mSigma.generateHistos(chains, fftLen, hFactor, 0, tmax - tmin);
            
            if(!(date.mDeltaType == Date::eDeltaFixed && date.mDeltaFixed == 0))
                date.mWiggle.generateHistos(chains, fftLen, hFactor, tmin, tmax);
        }
    }
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        Phase* phase = mPhases[i];
        
        phase->mAlpha.generateHistos(chains, fftLen, hFactor, tmin, tmax);
        phase->mBeta.generateHistos(chains, fftLen, hFactor, tmin, tmax);
    }
}

void Model::generateNumericalResults(const QList<Chain>& chains)
{
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents[i];
        event->mTheta.generateNumericalResults(chains);
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            date.mTheta.generateNumericalResults(chains);
            date.mSigma.generateNumericalResults(chains);
        }
    }
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        Phase* phase = mPhases[i];
        phase->mAlpha.generateNumericalResults(chains);
        phase->mBeta.generateNumericalResults(chains);
        phase->generateDurationCredibility();
    }
}

void Model::generateCredibilityAndHPD(const QList<Chain>& chains, int threshold)
{
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents[i];
        
        bool isFixedBound = false;
        if(event->type() == Event::eKnown)
        {
            EventKnown* ek = dynamic_cast<EventKnown*>(event);
            if(ek->knownType() == EventKnown::eFixed)
                isFixedBound = true;
        }
        
        if(!isFixedBound)
        {
            event->mTheta.generateHPD(threshold);
            event->mTheta.generateCredibility(chains, threshold);
            QList<Date>& dates = event->mDates;
            
            for(int j=0; j<dates.size(); ++j)
            {
                Date& date = dates[j];
                date.mTheta.generateHPD(threshold);
                date.mSigma.generateHPD(threshold);
                
                date.mTheta.generateCredibility(chains, threshold);
                date.mSigma.generateCredibility(chains, threshold);
            }
        }
    }
    for(int i=0; i<mPhases.size(); ++i)
    {
        Phase* phase = mPhases[i];
        phase->mAlpha.generateHPD(threshold);
        phase->mBeta.generateHPD(threshold);
        
        phase->mAlpha.generateCredibility(chains, threshold);
        phase->mBeta.generateCredibility(chains, threshold);
    }
}

