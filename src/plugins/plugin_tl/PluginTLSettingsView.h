#ifndef PLUGINTLSETTINGSVIEW_H
#define PLUGINTLSETTINGSVIEW_H

#if USE_PLUGIN_TL

#include "../PluginSettingsViewAbstract.h"

class PluginTL;


class PluginTLSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginTLSettingsView(PluginTL* plugin, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~PluginTLSettingsView();
};

#endif
#endif

