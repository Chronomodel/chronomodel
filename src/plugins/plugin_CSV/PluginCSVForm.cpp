/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include "PluginCSVForm.h"

#if USE_PLUGIN_CSV
#include "PluginCSV.h"

#include <QJsonObject>
#include <QtWidgets>


PluginCSVForm::PluginCSVForm(PluginCSV* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("CSV density file"), parent, flags)
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

PluginCSVForm::~PluginCSVForm()
{

}

void PluginCSVForm::setData(const QJsonObject& data, bool isCombined)
{

    if ( isCombined) {
        emit PluginFormAbstract::OkEnabled(true );
        
    } else {
        const QString curve = data.value(DATE_CSV_CURVE_STR).toString();
        mCurveCombo->setCurrentText(curve);
    }



    updateVisibleElements();
}

void PluginCSVForm::updateVisibleElements()
{
    mCurveCombo->setVisible(true);

}

QJsonObject PluginCSVForm::getData()
{
    QJsonObject data;
 /*   const QLocale locale = QLocale();

    const double age = locale.toDouble(mAverageEdit->text());
    const double error = locale.toDouble(mErrorEdit->text());
    const double a = locale.toDouble(mAEdit->text());
    const double b = locale.toDouble(mBEdit->text());
    const double c = locale.toDouble(mCEdit->text());

    QString mode = "";
    if(mCurveRadio->isChecked()) mode = DATE_GAUSS_MODE_CURVE;
    else if(mEquationRadio->isChecked()) mode = DATE_GAUSS_MODE_EQ;
    else if(mNoneRadio->isChecked()) mode = DATE_GAUSS_MODE_NONE;

    const QString curve = mCurveCombo->currentText();

    data.insert(DATE_GAUSS_AGE_STR, age);
    data.insert(DATE_GAUSS_ERROR_STR, error);
    data.insert(DATE_GAUSS_A_STR, a);
    data.insert(DATE_GAUSS_B_STR, b);
    data.insert(DATE_GAUSS_C_STR, c);
    data.insert(DATE_GAUSS_MODE_STR, mode);
    data.insert(DATE_GAUSS_CURVE_STR, curve);
*/

    const QString curve = mCurveCombo->currentText();
    data.insert(DATE_CSV_CURVE_STR, curve);
    return data;
}



void PluginCSVForm::validOK()
{
    emit PluginFormAbstract::OkEnabled(true);

}

bool PluginCSVForm::isValid()
{
    return true;
}

#endif
