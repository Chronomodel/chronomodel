#include "ModelUtilities.h"
#include "Date.h"
#include "../PluginAbstract.h"
#include <QObject>


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



