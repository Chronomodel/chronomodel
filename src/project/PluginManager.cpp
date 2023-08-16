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

#include "PluginManager.h"

#include "PluginMag.h"
#include "PluginTL.h"
#include "Plugin14C.h"
#include "PluginUniform.h"
#include "PluginGauss.h"
#include "PluginF14C.h"
#include "PluginDensity.h"

#include <QDebug>

QList<PluginAbstract*> PluginManager::mPlugins = QList<PluginAbstract*>();

PluginManager::~PluginManager()
{

}

void PluginManager::clearPlugins()
{
    for(auto& pl : mPlugins) {
        delete pl;
        pl = nullptr;
    }
    mPlugins.clear();
}

void PluginManager::loadPlugins()
{

#if USE_PLUGIN_14C
    PluginAbstract* plugin14C = new Plugin14C();
    mPlugins.append(plugin14C);
#endif

#if USE_PLUGIN_AM
    PluginAbstract* pluginMag = new PluginMag();
    mPlugins.append(pluginMag);
#endif

#if USE_PLUGIN_TL
    PluginAbstract* pluginTL = new PluginTL();
    mPlugins.append(pluginTL);
#endif

#if USE_PLUGIN_UNIFORM
    PluginAbstract* pluginUniform = new PluginUniform();
    mPlugins.append(pluginUniform);
#endif

#if USE_PLUGIN_GAUSS
    PluginAbstract* pluginGauss = new PluginGauss();
    mPlugins.append(pluginGauss);
#endif
    
#if USE_PLUGIN_F14C
    PluginAbstract* pluginF14C = new PluginF14C();
    mPlugins.append(pluginF14C);
#endif

#if USE_PLUGIN_DENSITY
    PluginAbstract* pluginDensity = new PluginDensity();
    mPlugins.append(pluginDensity);
#endif
}


PluginAbstract* PluginManager::getPluginFromId(const QString &pluginId)
{
    for (auto &pl : mPlugins)
        if (pl->getId() == pluginId)
            return pl;

    return nullptr;
}

PluginAbstract* PluginManager::getPluginFromName(const QString& pluginName)
{
    for (auto &pl : mPlugins)
        if (pl->getName().toLower() == pluginName.toLower())
            return pl;

    return nullptr;
}

const QList<PluginAbstract*>& PluginManager::getPlugins()
{
    return mPlugins;
}

QStringList PluginManager::getPluginsNames()
{
    QStringList names;
    for (auto &pl : mPlugins)
        names << pl->getName();

    return names;
}
