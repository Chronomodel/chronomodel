#ifndef PLUGINMAGSETTINGSVIEW_H
#define PLUGINMAGSETTINGSVIEW_H

#if USE_PLUGIN_AM

#include "../PluginSettingsViewAbstract.h"
#include <QMap>

class PluginMag;
class PluginRefCurveSettingsView;

class PluginMagSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginMagSettingsView(PluginMag* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginMagSettingsView();
    
private:
    PluginRefCurveSettingsView* mRefView;
};

#endif
#endif

