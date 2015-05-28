#include "ModelUtilities.h"
#include "Date.h"
#include "EventConstraint.h"
#include "../PluginAbstract.h"
#include "QtUtilities.h"
#include <QObject>
#include <QString>


bool sortEvents(Event* e1, Event* e2){return (e1->mItemY < e2->mItemY);}
bool sortPhases(Phase* p1, Phase* p2){return (p1->mItemY < p2->mItemY);}

QString ModelUtilities::getEventMethodText(Event::Method method)
{
    switch(method)
    {
        case Event::eMHAdaptGauss:
        {
            return QObject::tr("MH : proposal = adapt. Gaussian random walk");
        }
        case Event::eBoxMuller:
        {
            return QObject::tr("AR : proposal = Gaussian");
        }
        case Event::eDoubleExp:
        {
            return QObject::tr("AR : proposal = Double-Exponential");
        }
        default:
        {
            return QObject::tr("Unknown");
        }
    }
}

QString ModelUtilities::getDataMethodText(Date::DataMethod method)
{
    switch(method)
    {
        case Date::eMHIndependant:
        {
            return QObject::tr("MH : proposal = prior distribution");
        }
        case Date::eInversion:
        {
            return QObject::tr("MH : proposal = distribution of calibrated date");
        }
        case Date::eMHSymGaussAdapt:
        {
            return QObject::tr("MH : proposal = adapt. Gaussian random walk");
        }
        default:
        {
            return QObject::tr("Unknown");
        }
    }
}

QString ModelUtilities::getDeltaText(const Date& date)
{
    QString result;
    PluginAbstract* plugin = date.mPlugin;
    if(plugin && plugin->wiggleAllowed())
    {
        switch(date.mDeltaType)
        {
            case Date::eDeltaFixed:
                result = QObject::tr("Wiggle") + " : " + QString::number(date.mDeltaFixed);
                break;
            case Date::eDeltaRange:
                result = QObject::tr("Wiggle") + " : [" + QString::number(date.mDeltaMin) + ", " + QString::number(date.mDeltaMax) + "]";
                break;
            case Date::eDeltaGaussian:
                result = QObject::tr("Wiggle") + " : " + QString::number(date.mDeltaAverage) + " ± " + QString::number(date.mDeltaError);
                break;
                
            default:
                break;
        }
    }
    return result;
}

#pragma mark Events Branches
QVector<QVector<Event*> > ModelUtilities::getNextBranches(const QVector<Event*>& curBranch, Event* lastNode)
{
    QVector<QVector<Event*> > branches;
    QList<EventConstraint*> cts = lastNode->mConstraintsFwd;
    if(cts.size() > 0)
    {
        for(int i=0; i<cts.size(); ++i)
        {
            QVector<Event*> branch = curBranch;
            Event* newNode = cts[i]->mEventTo;
            
            if(newNode->mLevel <= lastNode->mLevel)
                newNode->mLevel = lastNode->mLevel + 1;
            
            if(!branch.contains(newNode))
            {
                branch.append(newNode);
                QVector<QVector<Event*> > nextBranches = getNextBranches(branch, cts[i]->mEventTo);
                for(int j=0; j<nextBranches.size(); ++j)
                    branches.append(nextBranches[j]);
            }
            else
            {
                QStringList evtNames;
                for(int j=0; j<branch.size(); ++j)
                    evtNames << branch[j]->mName;
                evtNames << newNode->mName;
                
                throw QObject::tr("Circularity found in events model !\nPlease correct this branch :\n") + evtNames.join(" -> ");
            }
        }
    }
    else
    {
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
    try{
        nextBranches = getNextBranches(startBranch, start);
    }catch(QString error){
        throw error;
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
    for(int i=0; i<events.size(); ++i)
    {
        events[i]->mLevel = 0;
        if(events[i]->mConstraintsBwd.size() == 0)
            starts.append(events[i]);
    }
    if(starts.size() == 0 && events.size() != 0)
    {
        throw QObject::tr("Circularity found in events model !");
    }
    else
    {
        for(int i=0; i<starts.size(); ++i)
        {
            QVector<QVector<Event*> > eventBranches;
            try{
                eventBranches = getBranchesFromEvent(starts[i]);
            }catch(QString error){
                throw error;
            }
            for(int j=0; j<eventBranches.size(); ++j)
                branches.append(eventBranches[j]);
        }
    }
    return branches;
}




#pragma mark Phases Branches
QVector<QVector<Phase*> > ModelUtilities::getNextBranches(const QVector<Phase*>& curBranch, Phase* lastNode, const double gammaSum, const double maxLength)
{
    QVector<QVector<Phase*> > branches;
    QList<PhaseConstraint*> cts = lastNode->mConstraintsFwd;
    if(cts.size() > 0)
    {
        for(int i=0; i<cts.size(); ++i)
        {
            QVector<Phase*> branch = curBranch;
            Phase* newNode = cts[i]->mPhaseTo;
            
            double gamma = gammaSum;
            if(cts[i]->mGammaType == PhaseConstraint::eGammaFixed)
                gamma += cts[i]->mGammaFixed;
            else if(cts[i]->mGammaType == PhaseConstraint::eGammaRange)
                gamma += cts[i]->mGammaMin;
            
            if(gamma < maxLength)
            {
                if(newNode->mLevel <= lastNode->mLevel)
                    newNode->mLevel = lastNode->mLevel + 1;
                
                if(!branch.contains(newNode))
                {
                    branch.append(newNode);
                    QVector<QVector<Phase*> > nextBranches = getNextBranches(branch, cts[i]->mPhaseTo, gamma, maxLength);
                    for(int j=0; j<nextBranches.size(); ++j)
                        branches.append(nextBranches[j]);
                }
                else
                {
                    QStringList names;
                    for(int j=0; j<branch.size(); ++j)
                        names << branch[j]->mName;
                    names << newNode->mName;
                    
                    throw QObject::tr("Circularity found in phases model !\nPlease correct this branch :\n") + names.join(" -> ");
                }
            }
            else
            {
                QStringList names;
                for(int j=0; j<curBranch.size(); ++j)
                    names << curBranch[j]->mName;
                names << newNode->mName;
                throw QObject::tr("Phases branch too long :\n") + names.join(" -> ");
            }
        }
    }
    else
    {
        branches.append(curBranch);
    }
    return branches;
}

QVector<QVector<Phase*> > ModelUtilities::getBranchesFromPhase(Phase* start, const double maxLength)
{
    QVector<Phase*> startBranch;
    start->mLevel = 0;
    startBranch.append(start);
    
    QVector<QVector<Phase*> > nextBranches;
    try{
        nextBranches = getNextBranches(startBranch, start, 0, maxLength);
    }catch(QString error){
        throw error;
    }
    
    return nextBranches;
}


QVector<QVector<Phase*> > ModelUtilities::getAllPhasesBranches(const QList<Phase*>& phases, const double maxLength)
{
    QVector<QVector<Phase*> > branches;
    
    QVector<Phase*> starts;
    for(int i=0; i<phases.size(); ++i)
    {
        phases[i]->mLevel = 0;
        if(phases[i]->mConstraintsBwd.size() == 0)
            starts.append(phases[i]);
    }
    if(starts.size() == 0 && phases.size() != 0)
    {
        throw QObject::tr("Circularity found in phases model !");
    }
    for(int i=0; i<starts.size(); ++i)
    {
        QVector<QVector<Phase*> > phaseBranches;
        try{
            phaseBranches = getBranchesFromPhase(starts[i], maxLength);
        }catch(QString error){
            throw error;
        }
        for(int j=0; j<phaseBranches.size(); ++j)
            branches.append(phaseBranches[j]);
    }
    return branches;
}


#pragma mark sort events by level
QVector<Event*> ModelUtilities::sortEventsByLevel(const QList<Event*>& events)
{
    int numSorted = 0;
    int curLevel = 0;
    QVector<Event*> results;
    
    while(numSorted < events.size())
    {
        for(int i=0; i<events.size(); ++i)
        {
            if(events[i]->mLevel == curLevel)
            {
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
    
    while(numSorted < phases.size())
    {
        for(int i=0; i<phases.size(); ++i)
        {
            if(phases[i]->mLevel == curLevel)
            {
                results.append(phases[i]);
                ++numSorted;
            }
        }
        ++curLevel;
    }
    return results;
}

QVector<Event*> ModelUtilities::unsortEvents(const QList<Event*>& events)
{
    QList<int> indexes;
    QVector<Event*> results;
    
    while(indexes.size() < events.size())
    {
        int index = rand() % events.size();
        if(!indexes.contains(index))
        {
            indexes.append(index);
            results.append(events[index]);
        }
    }
    return results;
    // PhD : Peut être juste recopier events et faire envents.size() swap d'éléments dans le tableau copié avant de le retourner !!
}

QString ModelUtilities::dateResultsText(Date* d)
{
    QString text;
    if(d)
    {
        text += line(textBold(textGreen("Data : " + d->mName)));
        text += "<br>";
        text += line(textBold(textGreen("Date :")));
        text += line(textGreen(d->mTheta.resultsText()));
        text += "<br>";
        text += line(textBold(textGreen("Std. Deviation :")));
        text += line(textGreen(d->mSigma.resultsText()));
        text += line(textGreen("----------------------"));
    }
    return text;
}

QString ModelUtilities::eventResultsText(Event* e, bool withDates)
{
    QString text;
    if(e)
    {
        if(e->mType == Event::eKnown)
        {
            text += line(textBold(textRed("Bound : " + e->mName))) + "<br>";
            text += line(textRed(e->mTheta.resultsText()));
            text += line(textRed("----------------------"));
        }
        else
        {
            text += line(textBold(textBlue("Event : " + e->mName))) + "<br>";
            text += line(textBlue(e->mTheta.resultsText()));
            text += line(textBlue("----------------------"));
            if(withDates)
            {
                for(int i=0; i<e->mDates.size(); ++i)
                {
                    text += dateResultsText(&(e->mDates[i]));
                }
            }
        }
    }
    return text;
}

QString ModelUtilities::phaseResultsText(Phase* p)
{
    QString text;
    if(p)
    {
        text += line(textBold(textPurple("Phase : " + p->mName)));
        
        text += "<br>";
        text += line(textBold(textPurple("Duration : ")));
        text += line(textPurple(p->mDuration.resultsText(QObject::tr("No duration estimated ! (normal if only 1 event in the phase)"))));
        
        text += "<br>";
        text += line(textBold(textPurple("Begin : ")));
        text += line(textPurple(p->mAlpha.resultsText()));
        
        text += "<br>";
        text += line(textBold(textPurple("End : ")));
        text += line(textPurple(p->mBeta.resultsText()));
        
        text += line(textPurple("----------------------"));
    }
    return text;
}