#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QStringList>

class PluginAbstract;


class PluginManager
{
public:
    static void loadPlugins();
    static void clearPlugins();
    static PluginAbstract* getPluginFromId(const QString& pluginId);
    static PluginAbstract* getPluginFromName(const QString& pluginName);
    static const QList<PluginAbstract*>& getPlugins();
    static QStringList getPluginsNames();
    
private:
    static QList<PluginAbstract*> mPlugins;
    
private:
    PluginManager(){}
    ~PluginManager();
    
    Q_DISABLE_COPY(PluginManager)
};

#endif
