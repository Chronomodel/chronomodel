/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "PluginManager.h"

#include "PluginAbstract.h"
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
    clearPlugins();
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

#if USE_PLUGIN_F14C
    PluginAbstract* pluginF14C = new PluginF14C();
    mPlugins.append(pluginF14C);
#endif

#if USE_PLUGIN_AM
    PluginAbstract* pluginMag = new PluginMag();
    mPlugins.append(pluginMag);
#endif

#if USE_PLUGIN_TL
    PluginAbstract* pluginTL = new PluginTL();
    mPlugins.append(pluginTL);
#endif

#if USE_PLUGIN_GAUSS
    PluginAbstract* pluginGauss = new PluginGauss();
    mPlugins.append(pluginGauss);
#endif

#if USE_PLUGIN_UNIFORM
    PluginAbstract* pluginUniform = new PluginUniform();
    mPlugins.append(pluginUniform);
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
