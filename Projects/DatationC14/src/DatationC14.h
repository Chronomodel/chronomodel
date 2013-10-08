#ifndef DATATIONC14_H
#define DATATIONC14_H

#include "datationc14_global.h"
#include "PluginInterface.h"

class DATATIONC14SHARED_EXPORT DatationC14 : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "chronomodel.PluginInterface.DatationC14")
    Q_INTERFACES(PluginInterface)
public:
    DatationC14();

    const QString getMenuName() const;
    void showDialog();

    const QVector<double> getDensity() const;
};

#endif // DATATIONC14_H
