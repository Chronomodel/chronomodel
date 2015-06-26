#include "PluginTLSettingsView.h"
#if USE_PLUGIN_TL

#include "PluginTL.h"
#include <QtWidgets>


PluginTLSettingsView::PluginTLSettingsView(PluginTL* plugin, QWidget* parent, Qt::WindowFlags flags):PluginSettingsViewAbstract(plugin, parent, flags){
    
}

PluginTLSettingsView::~PluginTLSettingsView(){
    
}

#endif