/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE
	Komlan NOUKPOAPE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */
#if USE_PLUGIN_AM

#include "PluginMagSettingsView.h"

#include "PluginMag.h"
#include "PluginRefCurveSettingsView.h"

#include <QLayout>

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
