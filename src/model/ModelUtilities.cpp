#include "ModelUtilities.h"
#include <QObject>


QString ModelUtilities::getEventMethodText(Event::Method method)
{
    switch(method)
    {
        case Event::eMHAdaptGauss:
        {
            return QObject::tr("MH Gaussian Adaptative");
        }
        case Event::eBoxMuller:
        {
            return QObject::tr("Box Muller");
        }
        case Event::eDoubleExp:
        {
            return QObject::tr("Double Exp");
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
            return QObject::tr("MH Independant");
        }
        case Date::eMHSymGaussAdapt:
        {
            return QObject::tr("Sym Gauss Adapt");
        }
        case Date::eInversion:
        {
            return QObject::tr("Inversion");
        }
        default:
        {
            return QObject::tr("Unknown");
        }
    }
}



