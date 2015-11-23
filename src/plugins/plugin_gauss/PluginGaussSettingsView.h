#ifndef PLUGINGAUSSSETTINGSVIEW_H
#define PLUGINGAUSSSETTINGSVIEW_H

#if USE_PLUGIN_GAUSS

#include "../PluginSettingsViewAbstract.h"
#include <QMap>

class PluginRefCurveSettingsView;
class PluginGauss;

class PluginGaussSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginGaussSettingsView(PluginGauss* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginGaussSettingsView();
    
protected:
    PluginRefCurveSettingsView* mRefView;
};

#endif
#endif

