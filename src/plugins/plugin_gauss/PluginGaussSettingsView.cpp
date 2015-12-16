#include "PluginGaussSettingsView.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "PluginRefCurveSettingsView.h"
#include <QtWidgets>


PluginGaussSettingsView::PluginGaussSettingsView(PluginGauss* plugin, QWidget* parent, Qt::WindowFlags flags):PluginSettingsViewAbstract(plugin, parent, flags)
{
    mRefView = new PluginRefCurveSettingsView(plugin);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mRefView);
    setLayout(layout);
}

PluginGaussSettingsView::~PluginGaussSettingsView()
{
    
}

#endif
