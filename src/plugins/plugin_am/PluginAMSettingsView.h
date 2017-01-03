#ifndef PluginAMSETTINGSVIEW_H
#define PluginAMSETTINGSVIEW_H

#if USE_PLUGIN_AM

#include "../PluginSettingsViewAbstract.h"
#include <QMap>

class PluginAM;
class PluginRefCurveSettingsView;

class PluginAMSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginAMSettingsView(PluginAM* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginAMSettingsView();
    
private:
    PluginRefCurveSettingsView* mRefView;
};

#endif
#endif

