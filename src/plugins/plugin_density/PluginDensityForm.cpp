/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
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
