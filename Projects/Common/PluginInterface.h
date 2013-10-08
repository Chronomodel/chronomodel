#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QtPlugin>
#include <QString>

class PluginInterface
{
public:
    virtual ~PluginInterface() {}

    virtual const QString getMenuName() const = 0;
    virtual void showDialog() = 0;
    virtual const QVector<double> getDensity() const = 0;
};

#define PluginInterface_iid "chonomodel.PluginInterface"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H
