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
    PluginMagSettingsView(PluginMag* plugin, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~PluginMagSettingsView();
    
private:
    PluginRefCurveSettingsView* mRefView;
};

#endif
#endif

