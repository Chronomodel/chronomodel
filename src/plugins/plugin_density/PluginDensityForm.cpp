/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

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

#include "PluginDensityForm.h"

#if USE_PLUGIN_DENSITY
#include "PluginDensity.h"

#include <QJsonObject>
#include <QtWidgets>


PluginDensityForm::PluginDensityForm(PluginDensity* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("CSV density file"), parent, flags)
{
    mCurveCombo = new QComboBox(this);
    const QStringList &refCurves = plugin->getRefsNames();
    for (auto & curve : refCurves)
        mCurveCombo->addItem(curve);

    QGridLayout* grid = new QGridLayout();
    grid->addWidget(mCurveCombo, 1, 1);

    setLayout(grid);

    updateVisibleElements();
}

PluginDensityForm::~PluginDensityForm()
{
}

void PluginDensityForm::setData(const QJsonObject& data, bool isCombined)
{
    mCurveCombo->setEnabled(!isCombined);
    if ( isCombined) {
        emit PluginFormAbstract::OkEnabled(true );
        
    } else {
        const QString curve = data.value(DATE_DENSITY_CURVE_STR).toString();
        mCurveCombo->setCurrentText(curve);
    }
    updateVisibleElements();
}

void PluginDensityForm::updateVisibleElements()
{
    mCurveCombo->setVisible(true);

}

QJsonObject PluginDensityForm::getData()
{
    QJsonObject data;

    const QString curve = mCurveCombo->currentText();
    data.insert(DATE_DENSITY_CURVE_STR, curve);
    return data;
}



void PluginDensityForm::validOK()
{
    emit PluginFormAbstract::OkEnabled(true);

}

bool PluginDensityForm::isValid()
{
    return true;
}

#endif
