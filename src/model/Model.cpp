#include "Model.h"
#include "Date.h"
#include "Project.h"
#include "EventKnown.h"
#include "MCMCLoopMain.h"
#include "MCMCProgressDialog.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "StdUtilities.h"
#include "../PluginAbstract.h"
#include <QJsonArray>
#include <QtWidgets>


#pragma mark Constructor...
Model::Model():QObject()
{
    
}

Model::~Model()
{
  /*  qDebug() << "Deleting old project model";
    
    for(int i=mEvents.size()-1; i>=0; --i)
    {
        Event* item = mEvents[i];
        delete item;
        item = 0;
        mEvents.removeAt(i);
    }
    for(int i=mPhases.size()-1; i>=0; --i)
    {
        Phase* item = mPhases[i];
        delete item;
        item = 0;
        mPhases.removeAt(i);
    }
    for(int i=mEventConstraints.size()-1; i>=0; --i)
    {
        EventConstraint* item = mEventConstraints[i];
        delete item;
        item = 0;
        mEventConstraints.removeAt(i);
    }
    for(int i=mPhaseConstraints.size()-1; i>=0; --i)
    {
        PhaseConstraint* item = mPhaseConstraints[i];
        delete item;
        item = 0;
        mPhaseConstraints.removeAt(i);
    }*/
}

void Model::clear()
{
    qDebug() << "Clear model";

    mEvents.clear();
    mPhases.clear();

    mPhaseConstraints.clear();
    mEventConstraints.clear();

    mChains.clear();
    mLogModel.clear();
    mLogMCMC.clear();
    mLogResults.clear();
}

/*Model* Model::fromJson(const QJsonObject& json)
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
        model->mChains = model->mMCMCSettings.getChains();
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
    
    // Sort phases based on items y position
    std::sort(model->mPhases.begin(), model->mPhases.end(), sortPhases);
    
    if(json.contains(STATE_EVENTS))
    {
        QJsonArray events = json[STATE_EVENTS].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                try{
                    Event* e = new Event(Event::fromJson(event));
                    model->mEvents.append(e);
                }
                catch(QString error){
                    QMessageBox message(QMessageBox::Critical,
                                        qApp->applicationName() + " " + qApp->applicationVersion(),
                                        tr("Error : ") + error,
                                        QMessageBox::Ok,
                                        qApp->activeWindow(),
                                        Qt::Sheet);
                    message.exec();
                }
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
        for(int j=0; j<model->mPhaseConstraints.size(); ++j)
        {
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
    return model;
}*/

#pragma mark JSON conversion
void Model::fromJson(const QJsonObject& json)
{
    //Model* model = new Model();

    if(json.contains(STATE_SETTINGS))
    {
        QJsonObject settings = json[STATE_SETTINGS].toObject();
       mSettings = ProjectSettings::fromJson(settings);
    }

    if(json.contains(STATE_MCMC))
    {
        QJsonObject mcmc = json[STATE_MCMC].toObject();
        mMCMCSettings = MCMCSettings::fromJson(mcmc);
        mChains = mMCMCSettings.getChains();
    }

    if(json.contains(STATE_PHASES))
    {
        QJsonArray phases = json[STATE_PHASES].toArray();
        for(int i=0; i<phases.size(); ++i)
        {
            QJsonObject phase = phases[i].toObject();
            Phase* p = new Phase(Phase::fromJson(phase));
            mPhases.append(p);
        }
    }

    // Sort phases based on items y position
    std::sort(mPhases.begin(), mPhases.end(), sortPhases);

    if(json.contains(STATE_EVENTS))
    {
        QJsonArray events = json[STATE_EVENTS].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                try{
                    Event* e = new Event(Event::fromJson(event));
                    mEvents.append(e);
                }
                catch(QString error){
                    QMessageBox message(QMessageBox::Critical,
                                        qApp->applicationName() + " " + qApp->applicationVersion(),
                                        tr("Error : ") + error,
                                        QMessageBox::Ok,
                                        qApp->activeWindow(),
                                        Qt::Sheet);
                    message.exec();
                }
            }
            else
            {
                EventKnown* e = new EventKnown(EventKnown::fromJson(event));
                e->updateValues(mSettings.mTmin, mSettings.mTmax, mSettings.mStep);
                mEvents.append(e);
            }
        }
    }

    // Sort events based on items y position
    std::sort(mEvents.begin(), mEvents.end(), sortEvents);

    if(json.contains(STATE_EVENTS_CONSTRAINTS))
    {
        QJsonArray constraints = json[STATE_EVENTS_CONSTRAINTS].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            EventConstraint* c = new EventConstraint(EventConstraint::fromJson(constraint));
            mEventConstraints.append(c);
        }
    }

    if(json.contains(STATE_PHASES_CONSTRAINTS))
    {
        QJsonArray constraints = json[STATE_PHASES_CONSTRAINTS].toArray();
        for(int i=0; i<constraints.size(); ++i)
        {
            QJsonObject constraint = constraints[i].toObject();
            PhaseConstraint* c = new PhaseConstraint(PhaseConstraint::fromJson(constraint));
            mPhaseConstraints.append(c);
        }
    }

    // ------------------------------------------------------------
    //  Link objects to each other
    //  Must be done here !
    //  nb : Les data sont déjà linkées aux events à leur création
    // ------------------------------------------------------------
    for(int i=0; i<mEvents.size(); ++i)
    {
        int eventId = mEvents[i]->mId;
        QList<int> phasesIds = mEvents[i]->mPhasesIds;

        // Link des events / phases
        for(int j=0; j<mPhases.size(); ++j)
        {
            int phaseId = mPhases[j]->mId;
            if(phasesIds.contains(phaseId))
            {
                mEvents[i]->mPhases.append(mPhases[j]);
                mPhases[j]->mEvents.append(mEvents[i]);
            }
        }

        // Link des events / contraintes d'event
        for(int j=0; j<mEventConstraints.size(); ++j)
        {
            if(mEventConstraints[j]->mFromId == eventId)
            {
                mEventConstraints[j]->mEventFrom = mEvents[i];
                mEvents[i]->mConstraintsFwd.append(mEventConstraints[j]);
            }
            else if(mEventConstraints[j]->mToId == eventId)
            {
                mEventConstraints[j]->mEventTo = mEvents[i];
                mEvents[i]->mConstraintsBwd.append(mEventConstraints[j]);
            }
        }
    }
    // Link des phases / contraintes de phase
    for(int i=0; i<mPhases.size(); ++i)
    {
        int phaseId = mPhases[i]->mId;
        for(int j=0; j<mPhaseConstraints.size(); ++j)
        {
            if(mPhaseConstraints[j]->mFromId == phaseId)
            {
                mPhaseConstraints[j]->mPhaseFrom = mPhases[i];
                mPhases[i]->mConstraintsFwd.append(mPhaseConstraints[j]);
            }
            else if(mPhaseConstraints[j]->mToId == phaseId)
            {
                mPhaseConstraints[j]->mPhaseTo = mPhases[i];
                mPhases[i]->mConstraintsBwd.append(mPhaseConstraints[j]);
            }
        }
    }
    //return model;
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

#pragma mark Logs
QString Model::getMCMCLog() const{
    return mLogMCMC;
}

QString Model::getModelLog() const{
    return mLogModel;
}

/**
 * @brief Model::generateModelLog
 * @return Return a QString with the recall of the Model with the data MCMC Methode, and constraint
 */
void Model::generateModelLog()
{
    QString log;
    
    log += line(textBold(QString::number(mEvents.size()) + " events"));
    log += "<br>";
    
    for(int i=0; i<mEvents.size(); ++i)
    {
        QString objType = "Event";
        if(mEvents[i]->type() == Event::eKnown)
        {
            log += line(textRed("Bound (" + QString::number(i+1) + "/" + QString::number(mEvents.size()) + ") : " + mEvents[i]->mName + " (" +
                                 QString::number(mEvents[i]->mPhases.size()) + " phases, " +
                                 QString::number(mEvents[i]->mConstraintsBwd.size()) + " const. back., " +
                                 QString::number(mEvents[i]->mConstraintsFwd.size()) + " const. fwd.)"));
            log += line(textRed("--------"));
        }
        else
        {
            log += line(textBlue("Event (" + QString::number(i+1) + "/" + QString::number(mEvents.size()) + ") : " + mEvents[i]->mName + " (" +
                                 QString::number(mEvents[i]->mDates.size()) + " data, " +
                                 QString::number(mEvents[i]->mPhases.size()) + " phases, " +
                                 QString::number(mEvents[i]->mConstraintsBwd.size()) + " const. back., " +
                                 QString::number(mEvents[i]->mConstraintsFwd.size()) + " const. fwd.)"));
            log += line(textBlue("--------"));
        }
        
        for(int j=0; j<mEvents[i]->mDates.size(); ++j)
        {
            log += line(textGreen("Data (" + QString::number(j+1) + "/" + QString::number(mEvents[i]->mDates.size()) + ") : " + mEvents[i]->mDates[j].mName +
                                  "<br>- Type : " + mEvents[i]->mDates[j].mPlugin->getName() +
                                  "<br>- Method : " + ModelUtilities::getDataMethodText(mEvents[i]->mDates[j].mMethod) +
                                  "<br>- Params : " + mEvents[i]->mDates[j].getDesc()));
            log += line(textGreen("--------"));
        }
        log += "<br>";
    }
    
    log += "<br>";
    log += line(textBold(QString::number(mPhases.size()) + " phases"));
    log += "<br>";
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        log += line(textPurple("Phase (" + QString::number(i+1) + "/" + QString::number(mPhases.size()) + ") : " + mPhases[i]->mName + " (" + QString::number(mPhases[i]->mEvents.size()) + ")"));
        log += line(textPurple("--------"));
        
        for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
        {
            log += line(textBlue("Event : " + mPhases[i]->mEvents[j]->mName));
        }
    }
    mLogModel = log;
    
    
    /*qDebug() << "=> Phases : " << model->mPhases.size();
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
     qDebug() << "===========================================";*/
}

QString Model::getResultsLog() const{
    return mLogResults;
}

void Model::generateResultsLog()
{
    QString log;
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents[i];
        log += ModelUtilities::eventResultsText(event, true);
    }
    for(int i=0; i<mPhases.size(); ++i)
    {
        Phase* phase = mPhases[i];
        log += ModelUtilities::phaseResultsText(phase);
    }
    mLogResults = log;
}

#pragma mark Model validity
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
    QVector<QVector<Event*> > eventBranches;
    try{
        eventBranches = ModelUtilities::getAllEventsBranches(mEvents);
    }catch(QString error){
        throw error;
    }
    
    // 5 - Pas de circularité sur les contraintes de phases
    // 6 - Gammas : sur toutes les branches, la somme des gamma min < plage d'étude :
    QVector<QVector<Phase*> > phaseBranches;
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
                    //qDebug() << phase->mEvents[k]->mName << " in " << phase->mName;
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
                double lower = mSettings.mTmin;
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
                double upper = mSettings.mTmax;
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
        double gammaMin = 0;
        PhaseConstraint::GammaType gType = mPhaseConstraints[i]->mGammaType;
        if(gType == PhaseConstraint::eGammaFixed)
            gammaMin = mPhaseConstraints[i]->mGammaFixed;
        else if(gType == PhaseConstraint::eGammaRange)
            gammaMin = mPhaseConstraints[i]->mGammaMin;
        
        double lower = mSettings.mTmin;
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
        double upper = mSettings.mTmax;
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
            double tauMax = mPhases[i]->mTauFixed;
            if(mPhases[i]->mTauType == Phase::eTauRange)
                tauMax = mPhases[i]->mTauMax;
            
            double min = mSettings.mTmin;
            double max = mSettings.mTmax;
            bool boundFound = false;
            
            for(int j=0; j<mPhases[i]->mEvents.size(); ++j)
            {
                if(mPhases[i]->mEvents[j]->mType == Event::eKnown)
                {
                    EventKnown* bound = dynamic_cast<EventKnown*>(mPhases[i]->mEvents[j]);
                    if(bound)
                    {
                        boundFound = true;
                        if(bound->mKnownType == EventKnown::eFixed)
                        {
                            min = std::max(min, bound->mFixed);
                            max = std::min(max, bound->mFixed);
                        }
                        else if(bound->mKnownType == EventKnown::eUniform)
                        {
                            min = std::max(min, bound->mUniformEnd);
                            max = std::min(max, bound->mUniformStart);
                        }
                    }
                }
            }
            if(boundFound)
            {
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
                                bound->mUniformStart = std::max(bound->mUniformStart, max - tauMax);
                                bound->mUniformEnd = std::min(bound->mUniformEnd, min + tauMax);
                                
                                min = std::max(min, bound->mUniformEnd);
                                max = std::min(max, bound->mUniformStart);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 11 - Vérifier la cohérence entre les contraintes de faits et de phase
    for(int i=0; i<phaseBranches.size(); ++i)
    {
        for(int j=0; j<phaseBranches[i].size(); ++j)
        {
            Phase* phase = phaseBranches[i][j];
            for(int k=0; k<phase->mEvents.size(); ++k)
            {
                Event* event = phase->mEvents[k];
                
                bool phaseFound = false;
                
                // On réinspecte toutes les phases de la branche et on vérifie que le fait n'a pas de contrainte en contradiction avec les contraintes de phase !
                for(int l=0; l<phaseBranches[i].size(); ++l)
                {
                    Phase* p = phaseBranches[i][l];
                    if(p == phase)
                    {
                        phaseFound = true;
                    }
                    else
                    {
                        for(int m=0; m<p->mEvents.size(); ++m)
                        {
                            Event* e = p->mEvents[m];
                            
                            // Si on regarde l'élément d'un phase d'avant, le fait ne peut pas être en contrainte vers un fait de cette phase
                            if(!phaseFound)
                            {
                                for(int n=0; n<e->mConstraintsBwd.size(); ++n)
                                {
                                    if(e->mConstraintsBwd[n]->mEventFrom == event)
                                    {
                                        throw "The event " + event->mName + " (in phase " + phase->mName + ") is before the event " + e->mName + " (in phase " + p->mName + "), BUT the phase " + phase->mName + " is after the phase " + p->mName + ".\n=> Contradiction !";
                                    }
                                }
                            }
                            else
                            {
                                for(int n=0; n<e->mConstraintsFwd.size(); ++n)
                                {
                                    if(e->mConstraintsFwd[n]->mEventTo == event)
                                    {
                                        throw "The event " + event->mName + " (in phase " + phase->mName + ") is after the event " + e->mName + " (in phase " + p->mName + "), BUT the phase " + phase->mName + " is before the phase " + p->mName + ".\n=> Contradiction !";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

#pragma mark Generate model data
void Model::generateCorrelations(const QList<Chain>& chains)
{
    QTime t = QTime::currentTime();
    
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents[i];
        event->mTheta.generateCorrelations(chains);
        
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date* date = &(event->mDates[j]);
            date->mTheta.generateCorrelations(chains);
            date->mSigma.generateCorrelations(chains);
        }
    }
    
    for(int i=0; i<mPhases.size(); ++i)
    {
        //Phase* phase = mPhases[i];
        mPhases[i]->mAlpha.generateCorrelations(chains);
        mPhases[i]->mBeta.generateCorrelations(chains);
    }
    
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateCorrelations done in " + QString::number(timeDiff) + " ms";
}

void Model::generatePosteriorDensities(const QList<Chain>& chains, int fftLen, double hFactor)
{
    QTime t = QTime::currentTime();
    
    double tmin = mSettings.mTmin;
    double tmax = mSettings.mTmax;
    
    for(int i=0; i<mEvents.size(); ++i)
    {
        Event* event = mEvents[i];
        
        // Generate event histos for all events and all bounds except for bounds of type "fixed"
        bool generateEventHistos = true;
        if(event->type() == Event::eKnown)
        {
            EventKnown* ek = dynamic_cast<EventKnown*>(event);
            if(ek && (ek->knownType() == EventKnown::eFixed))
            {
                // "Nothing todo : this is just a Dirac !";
                generateEventHistos = false;
            }
        }
        if(generateEventHistos)
        {
            event->mTheta.generateHistos(chains, fftLen, hFactor, tmin, tmax);
        }
        
        // Generate dates histos
        for(int j=0; j<event->mDates.size(); ++j)
        {
            Date& date = event->mDates[j];
            
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
        phase->mDuration.generateHistos(chains, fftLen, hFactor, 0, tmax - tmin);
    }
    
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generatePosteriorDensities done in " + QString::number(timeDiff) + " ms";
}

void Model::generateNumericalResults(const QList<Chain>& chains)
{
    QTime t = QTime::currentTime();
    
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
        phase->mDuration.generateNumericalResults(chains);
    }
    
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateNumericalResults done in " + QString::number(timeDiff) + " ms";
}

void Model::generateCredibilityAndHPD(const QList<Chain>& chains, double thresh)
{
    QTime t = QTime::currentTime();
    
    /* double threshold = thresh;
    threshold = std::min(100.0, threshold);
    threshold = std::max(0.0, threshold); */
    double threshold = inRange(0.0,thresh,100.0);
    
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
        // if there is only one Event in the phase, there is no Duration
        phase->mDuration.generateHPD(threshold);
        
        phase->mAlpha.generateCredibility(chains, threshold);
        phase->mBeta.generateCredibility(chains, threshold);
        phase->mDuration.generateCredibility(chains, threshold);
    }
    
    QTime t2 = QTime::currentTime();
    qint64 timeDiff = t.msecsTo(t2);
    qDebug() <<  "=> Model::generateCredibilityAndHPD done in " + QString::number(timeDiff) + " ms";
}

#pragma mark Clear model data
void Model::clearPosteriorDensities()
{
    for(int i=0; i<mEvents.size(); ++i) {
        Event* event = mEvents[i];
        for(int j=0; j<mEvents[i]->mDates.size(); ++j) {
            Date& date = event->mDates[j];
            date.mTheta.mHisto.clear();
            date.mSigma.mHisto.clear();
            date.mTheta.mChainsHistos.clear();
            date.mSigma.mChainsHistos.clear();
        }
        event->mTheta.mHisto.clear();
        event->mTheta.mChainsHistos.clear();
    }
    
    for(int i=0; i<mPhases.size(); ++i) {
        Phase* phase = mPhases[i];
        phase->mAlpha.mHisto.clear();
        phase->mBeta.mHisto.clear();
        phase->mAlpha.mChainsHistos.clear();
        phase->mBeta.mChainsHistos.clear();
        
    }
}
void Model::clearCredibilityAndHPD()
{
    for(int i=0; i<mEvents.size(); ++i) {
        Event* event = mEvents[i];
        for(int j=0; j<mEvents[i]->mDates.size(); ++j) {
            Date& date = event->mDates[j];
            date.mTheta.mHPD.clear();
            date.mSigma.mHPD.clear();
            
        }
        event->mTheta.mHPD.clear();
    }
    for(int i=0; i<mPhases.size(); ++i) {
        Phase* phase = mPhases[i];
        phase->mAlpha.mHPD.clear();
        phase->mBeta.mHPD.clear();
    }
}




#pragma mark Dat files read / write
/** @Brief Save .dat file, the result of computation and compress it
 *
 * */
void Model::saveToFile(const QString& fileName)
{
    if(!mEvents.empty())
    //if(!mEvents[0]->mTheta.mTrace.empty())
    {
    // -----------------------------------------------------
    //  Create file
    // -----------------------------------------------------
    
    QFile file(fileName);
    QByteArray uncompresedData;
    if(file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&uncompresedData, QIODevice::WriteOnly);
        
        // -----------------------------------------------------
        //  Write info
        // -----------------------------------------------------
        out << (qint32)mPhases.size();
        out << (qint32)mEvents.size();

        qint32 numDates = 0;
        for(int i=0; i<mEvents.size(); ++i)
            numDates += mEvents[i]->mDates.size();
        out << numDates;
        // -----------------------------------------------------
        //  Write phases data
        // -----------------------------------------------------
        for(int i=0; i<mPhases.size(); ++i)
        {
           
            mPhases[i]->mAlpha.saveToStream(&out);
            mPhases[i]->mBeta.saveToStream(&out);
            mPhases[i]->mDuration.saveToStream(&out);
        }

        // -----------------------------------------------------
        //  Write events data
        // -----------------------------------------------------
        for(int i=0; i<mEvents.size(); ++i)
        {

           mEvents[i]->mTheta.saveToStream(&out);
        }

        // -----------------------------------------------------
        //  Write dates data
        // -----------------------------------------------------
        for(int i=0; i<mEvents.size(); ++i)
        {
            Event* event = mEvents[i];
            QList<Date>& dates = event->mDates;
            for(int j=0; j<dates.size(); ++j)
            {

                dates[j].mTheta.saveToStream(&out);
                dates[j].mSigma.saveToStream(&out);
                dates[j].mWiggle.saveToStream(&out);
                
               
                out << dates[j].mDeltaFixed;
                out << dates[j].mDeltaMin;
                out << dates[j].mDeltaMax;
                out << dates[j].mDeltaAverage;
                out << dates[j].mDeltaError;
                
                out << dates[j].mSettings.mTmin;
                out << dates[j].mSettings.mTmax;
                out << dates[j].mSettings.mStep;
                out << dates[j].mSettings.mStepForced;
                
              //  out << dates[j].mSubDates;
                
                out << dates[j].mCalibration;
                out << dates[j].mRepartition;
                out << dates[j].mCalibHPD;

            }
        }
        out << mLogModel;
        out << mLogMCMC;
        out << mLogResults;
        QByteArray compressedData = qCompress(uncompresedData);
        file.write(compressedData);
        file.close();
    }
  }
}
/** @Brief Read the .dat file, it's the result of the saved computation and uncompress it
 *
 * */
void Model::restoreFromFile(const QString& fileName)
{
    QFile file(fileName);

    if(file.exists() && file.open(QIODevice::ReadOnly))
    {
        QByteArray compressedData = file.readAll();

        QByteArray uncompresedData = qUncompress(compressedData);

/* #ifdef DEBUG
        qDebug() << "Lecture fichier :"<< fileName;
        qDebug() << "TAILLE compressedData :" << compressedData.size();
        qDebug() << "TAILLE uncompresedData :" << uncompresedData.size();
#endif */
        if(uncompresedData.size()!=0)
        {
            QDataStream in(&uncompresedData, QIODevice::ReadOnly);

            // -----------------------------------------------------
            //  Read info
            // -----------------------------------------------------

            qint32 numPhases = 0;
            in >> numPhases;

            qint32 numEvents = 0;
            in >> numEvents;

            qint32 numdates = 0;
            in >> numdates;

            // -----------------------------------------------------
            //  Read phases data
            // -----------------------------------------------------

            for(int i=0; i<mPhases.size(); ++i)
            {
                /*in >> mPhases[i]->mAlpha.mTrace;
                in >> mPhases[i]->mBeta.mTrace;
                in >> mPhases[i]->mDuration.mTrace;*/
                mPhases[i]->mAlpha.loadFromStream(&in);
                mPhases[i]->mBeta.loadFromStream(&in);
                mPhases[i]->mDuration.loadFromStream(&in);
            }

            // -----------------------------------------------------
            //  Read events data
            // -----------------------------------------------------

            for(int i=0; i<mEvents.size(); ++i)
            {
                /*in >> mEvents[i]->mTheta.mTrace;
                in >> mEvents[i]->mTheta.mHistoryAcceptRateMH;
                in >> mEvents[i]->mTheta.mAllAccepts;*/
                mEvents[i]->mTheta.loadFromStream(&in);
            }

            // -----------------------------------------------------
            //  Read dates data
            // -----------------------------------------------------

            for(int i=0; i<mEvents.size(); ++i)
            {
                for(int j=0; j<mEvents[i]->mDates.size(); ++j)
                {
                    /*in >> mEvents[i]->mDates[j].mTheta.mTrace;
                    in >> mEvents[i]->mDates[j].mTheta.mHistoryAcceptRateMH;
                    in >> mEvents[i]->mDates[j].mTheta.mAllAccepts;

                    in >> mEvents[i]->mDates[j].mSigma.mTrace;
                    in >> mEvents[i]->mDates[j].mSigma.mHistoryAcceptRateMH;
                    in >> mEvents[i]->mDates[j].mSigma.mAllAccepts;

                    in >> mEvents[i]->mDates[j].mWiggle.mTrace;
                    in >> mEvents[i]->mDates[j].mWiggle.mHistoryAcceptRateMH;
                    in >> mEvents[i]->mDates[j].mWiggle.mAllAccepts;*/

                    mEvents[i]->mDates[j].mTheta.loadFromStream(&in);
                    mEvents[i]->mDates[j].mSigma.loadFromStream(&in);
                    mEvents[i]->mDates[j].mWiggle.loadFromStream(&in);
                                                            
                    
                    in >> mEvents[i]->mDates[j].mDeltaFixed;
                    in >> mEvents[i]->mDates[j].mDeltaMin;
                    in >> mEvents[i]->mDates[j].mDeltaMax;
                    in >> mEvents[i]->mDates[j].mDeltaAverage;
                    in >> mEvents[i]->mDates[j].mDeltaError;
                    
                    in >> mEvents[i]->mDates[j].mSettings.mTmin;
                    in >> mEvents[i]->mDates[j].mSettings.mTmax;
                    in >> mEvents[i]->mDates[j].mSettings.mStep;
                    in >> mEvents[i]->mDates[j].mSettings.mStepForced;
                    
                   // in >> mEvents[i]->mDates[j].mSubDates;
                    
                    in >> mEvents[i]->mDates[j].mCalibration;
                    in >> mEvents[i]->mDates[j].mRepartition;
                    in >> mEvents[i]->mDates[j].mCalibHPD;
                    
                     if (mEvents[i]->mDates[j].mCalibration.isEmpty()) qDebug()<<"Model::restoreFromFile vide";
                    
                }
            }
            in >> mLogModel;
            in >> mLogMCMC;
            in >> mLogResults;
        
            generateCorrelations(mChains);
            generatePosteriorDensities(mChains, 1024, 1);
            generateNumericalResults(mChains);
        }
        file.close();
    }
    
}
