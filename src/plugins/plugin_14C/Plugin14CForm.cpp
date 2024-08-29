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

#include "Plugin14CForm.h"
#if USE_PLUGIN_14C

#include "Plugin14C.h"

#include <QJsonObject>
#include <QtWidgets>

QString Plugin14CForm::mSelectedRefCurve = QString();

Plugin14CForm::Plugin14CForm(Plugin14C* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("14C Measurements"), parent, flags)
{
    mAverageLab = new QLabel(tr("Age (BP)"), this);
    mErrorLab = new QLabel(tr("Error (sd)"), this);
    mRLab = new QLabel(tr("Reservoir Effect (ΔR)"), this);
    mRErrorLab = new QLabel(tr("ΔR Error"), this);
    mRefLab = new QLabel(tr("Reference curve"), this);

    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setText("0");
    mAverageEdit->setAlignment(Qt::AlignHCenter);
    QDoubleValidator* RValidator = new QDoubleValidator(this);
    mAverageEdit->setValidator(RValidator);

    mErrorEdit = new QLineEdit(this);
    mErrorEdit->setText("30");
    mErrorEdit->setAlignment(Qt::AlignHCenter);
    QDoubleValidator* RplusValidator = new QDoubleValidator(this);
    RplusValidator->setBottom(0.000001);
    mErrorEdit->setValidator(RplusValidator);
    setTabOrder(mAverageEdit, mErrorEdit);

    connect(mErrorEdit, &QLineEdit::textChanged, this, &Plugin14CForm::errorIsValid);

    mREdit = new QLineEdit(this);
    mREdit->setText("0");
    mREdit->setAlignment(Qt::AlignHCenter);

    mRErrorEdit = new QLineEdit(this);
    mRErrorEdit->setText("0");
    mRErrorEdit->setAlignment(Qt::AlignHCenter);
    mRErrorEdit->setValidator(RValidator);
    setTabOrder(mREdit, mRErrorEdit);

    mRefCombo = new QComboBox(this);
    QStringList refCurves = mPlugin->getRefsNames();
    for (int i = 0; i<refCurves.size(); ++i)
        mRefCombo->addItem(refCurves.at(i));

    QString defCurve = QString("intcal20.14c").toLower();
    if (mSelectedRefCurve.isEmpty() && refCurves.contains(defCurve, Qt::CaseInsensitive))
       mSelectedRefCurve = defCurve;

    mRefCombo->setCurrentText(mSelectedRefCurve);


    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 3, 0, 0);

    grid->addWidget(mAverageLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAverageEdit, 0, 1);

    grid->addWidget(mErrorLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mErrorEdit, 1, 1);

    grid->addWidget(mRLab, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mREdit, 2, 1);

    grid->addWidget(mRErrorLab, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mRErrorEdit, 3, 1);

    grid->addWidget(mRefLab, 4, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mRefCombo, 4, 1);

    setLayout(grid);
}

Plugin14CForm::~Plugin14CForm()
{

}

void Plugin14CForm::setData(const QJsonObject& data, bool isCombined)
{
    const QLocale locale = QLocale();
    const double a = data.value(DATE_14C_AGE_STR).toDouble();
    const double e = data.value(DATE_14C_ERROR_STR).toDouble();
    const double r = data.value(DATE_14C_DELTA_R_STR).toDouble();
    const double re = data.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();
    const QString c = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();

    mAverageEdit->setText(locale.toString(a));
    mErrorEdit->setText(locale.toString(e));
    mREdit->setText(locale.toString(r));
    mRErrorEdit->setText(locale.toString(re));
    mRefCombo->setCurrentText(c);

    mAverageEdit->setEnabled(!isCombined);
    mErrorEdit->setEnabled(!isCombined);
    mREdit->setEnabled(!isCombined);
    mRErrorEdit->setEnabled(!isCombined);
    mRefCombo->setEnabled(!isCombined);
}

QJsonObject Plugin14CForm::getData()
{
    QJsonObject data;
    const QLocale locale = QLocale();
    const double a = locale.toDouble(mAverageEdit->text());
    const double e = locale.toDouble(mErrorEdit->text());
    const double r = locale.toDouble(mREdit->text());
    const double re = locale.toDouble(mRErrorEdit->text());
    const QString c = mRefCombo->currentText();

    data.insert(DATE_14C_AGE_STR, a);
    data.insert(DATE_14C_ERROR_STR, e);
    data.insert(DATE_14C_DELTA_R_STR, r);
    data.insert(DATE_14C_DELTA_R_ERROR_STR, re);
    data.insert(DATE_14C_REF_CURVE_STR, c);

    mSelectedRefCurve = c;

    return data;
}

void Plugin14CForm::errorIsValid(QString str)
{
    (void) str;
    emit PluginFormAbstract::OkEnabled(mErrorEdit->hasAcceptableInput());

}

bool Plugin14CForm::isValid()
{
    const QString refCurve = mRefCombo->currentText();
    if (refCurve.isEmpty())
        mError = tr("Ref. curve is empty!");
    return !refCurve.isEmpty();
}

#endif
