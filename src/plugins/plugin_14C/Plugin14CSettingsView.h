#ifndef PLUGIN14CSETTINGSVIEW_H
#define PLUGIN14CSETTINGSVIEW_H

#if USE_PLUGIN_14C

#include "../PluginSettingsViewAbstract.h"
#include <QMap>

class Plugin14C;
class PluginRefCurveSettingsView;

class Plugin14CSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    Plugin14CSettingsView(Plugin14C* plugin, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~Plugin14CSettingsView();
    
private:
    PluginRefCurveSettingsView* mRefView;
};

#endif
#endif

