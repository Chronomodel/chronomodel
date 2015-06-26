#ifndef PLUGINMagSETTINGSVIEW_H
#define PLUGINMagSETTINGSVIEW_H

#if USE_PLUGIN_Mag

#include "PluginSettingsViewAbstract.h"

class PluginMag;


class PluginMagSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginMagSettingsView(PluginMag* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginMagSettingsView();
};

#endif
#endif

