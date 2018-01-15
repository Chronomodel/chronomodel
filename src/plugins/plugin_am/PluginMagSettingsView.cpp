#include "PluginMagSettingsView.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include "PluginRefCurveSettingsView.h"
#include <QtWidgets>


PluginMagSettingsView::PluginMagSettingsView(PluginMag* plugin, QWidget* parent, Qt::WindowFlags flags):PluginSettingsViewAbstract(plugin, parent, flags)
{
    mRefView = new PluginRefCurveSettingsView(plugin);
    connect(mRefView, &PluginRefCurveSettingsView::listRefCurveChanged, this, &PluginMagSettingsView::calibrationNeeded);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mRefView);
    setLayout(layout);
}

PluginMagSettingsView::~PluginMagSettingsView()
{
    
}

#endif
