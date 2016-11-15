#include "Plugin14CSettingsView.h"
#if USE_PLUGIN_14C

#include "Plugin14C.h"
#include "PluginRefCurveSettingsView.h"
#include <QtWidgets>


Plugin14CSettingsView::Plugin14CSettingsView(Plugin14C* plugin, QWidget* parent, Qt::WindowFlags flags):PluginSettingsViewAbstract(plugin, parent, flags){
    
    mRefView = new PluginRefCurveSettingsView(plugin);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mRefView);
    setLayout(layout);
}

Plugin14CSettingsView::~Plugin14CSettingsView(){

}

#endif
