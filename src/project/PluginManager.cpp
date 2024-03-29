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

#include "PluginManager.h"
#include "../PluginAbstract.h"
#include <QDebug>

// Temp:
#include "PluginMag.h"
#include "PluginTL.h"
#include "Plugin14C.h"
#include "PluginUniform.h"
#include "PluginGauss.h"


QList<PluginAbstract*> PluginManager::mPlugins = QList<PluginAbstract*>();

PluginManager::~PluginManager()
{

}

void PluginManager::clearPlugins()
{
    for(int i=0; i<mPlugins.size(); ++i) {
        delete mPlugins[i];
        mPlugins[i] = nullptr;
    }
    mPlugins.clear();
}

void PluginManager::loadPlugins()
{
    /*QDir pluginsDir = QDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)

#elif defined(Q_OS_MAC)
    if(pluginsDir.dirName() == "MacOS")
    {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif

    foreach(QString fileName, pluginsDir.entryList(QDir::Files))
    {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject* plugin = loader.instance();
        if(plugin)
        {
            PluginAbstract* iPlugin = qobject_cast<PluginAbstract*>(plugin);
            if(iPlugin)
            {
                qDebug() << QString("Plugin ok : ") << plugin;
                mPlugins.append(iPlugin);
            }
            else
                qDebug() << "Plugin non conforme à l'interface : " << plugin;
        }
        else
            qDebug() << "Chargement échoué : " << plugin;
    }*/
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
}


PluginAbstract* PluginManager::getPluginFromId(const QString& pluginId)
{
    for (int i=0; i<mPlugins.size(); ++i)
        if (mPlugins.at(i)->getId() == pluginId)
            return mPlugins.at(i);
    return nullptr;
}

PluginAbstract* PluginManager::getPluginFromName(const QString& pluginName)
{
    for (int i=0; i<mPlugins.size(); ++i)
        if (mPlugins.at(i)->getName().toLower() == pluginName.toLower())
            return mPlugins.at(i);
    return nullptr;
}

const QList<PluginAbstract*>& PluginManager::getPlugins()
{
    return mPlugins;
}

QStringList PluginManager::getPluginsNames()
{
    QStringList names;
    for (int i=0; i<mPlugins.size(); ++i)
        names << mPlugins.at(i)->getName();
    return names;
}
