#ifndef PLUGINTLSETTINGSVIEW_H
#define PLUGINTLSETTINGSVIEW_H

#if USE_PLUGIN_TL

#include "../PluginSettingsViewAbstract.h"

class PluginTL;


class PluginTLSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginTLSettingsView(PluginTL* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginTLSettingsView();
};

#endif
#endif

