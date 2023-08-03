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

#include "PluginF14CForm.h"
#if USE_PLUGIN_F14C

#include "PluginF14C.h"

#include <QJsonObject>
#include <QtWidgets>

QString PluginF14CForm::mSelectedRefCurve = QString();

PluginF14CForm::PluginF14CForm(PluginF14C* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("F14C Measurements"), parent, flags)
{
    PluginF14C* PluginF14C = static_cast<class PluginF14C*> (mPlugin);

    mAverageLab = new QLabel(tr("F14C"), this);
    mErrorLab = new QLabel(tr("Error (sd)"), this);
    mRefLab = new QLabel(tr("Reference curve"), this);

    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setText("1");
    mAverageEdit->setAlignment(Qt::AlignHCenter);
    mRValidator = new QDoubleValidator(0., 10., -1, this);
    mAverageEdit->setValidator(mRValidator);

    mErrorEdit = new QLineEdit(this);
    QLocale locale;
    mErrorEdit->setText(locale.toString(0.1));
    mErrorEdit->setAlignment(Qt::AlignHCenter);
    mRplusValidator = new QDoubleValidator(this);
    mRplusValidator->setBottom(0.0);
    mRplusValidator->setTop(10.0);
    mErrorEdit->setValidator(mRplusValidator);


    connect(mAverageEdit, &QLineEdit::textChanged, this, &PluginF14CForm::valueIsValid);
    connect(mErrorEdit, &QLineEdit::textChanged, this, &PluginF14CForm::errorIsValid);

    mRefCombo = new QComboBox(this);
    QStringList refCurves = PluginF14C->getRefsNames();
    for (int i = 0; i<refCurves.size(); ++i)
        mRefCombo->addItem(refCurves.at(i));

    QString defCurve = QString("bomb13nh1.14c").toLower();
    if (mSelectedRefCurve.isEmpty() && refCurves.contains(defCurve, Qt::CaseInsensitive))
       mSelectedRefCurve = defCurve;

    mRefCombo->setCurrentText(mSelectedRefCurve);


    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 5, 0, 0);

    grid->addWidget(mAverageLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAverageEdit, 0, 1);

    grid->addWidget(mErrorLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mErrorEdit, 1, 1);

    grid->addWidget(mRefLab, 4, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mRefCombo, 4, 1);

    setLayout(grid);
}

PluginF14CForm::~PluginF14CForm()
{

}

void PluginF14CForm::setData(const QJsonObject& data, bool isCombined)
{
    if ( isCombined) {
        mAverageEdit->setText("Combined data");
        mErrorEdit->setText("Combined data");
        emit PluginFormAbstract::OkEnabled(true );

    } else {
        const QLocale locale = QLocale();
        const double a = data.value(DATE_F14C_FRACTION_STR).toDouble();
        const double e = data.value(DATE_F14C_ERROR_STR).toDouble();
        const QString c = data.value(DATE_F14C_REF_CURVE_STR).toString().toLower();

        mAverageEdit->setText(locale.toString(a));
        mErrorEdit->setText(locale.toString(e));
        mRefCombo->setCurrentText(c);

        mAverageEdit->setEnabled(!isCombined);
        mErrorEdit->setEnabled(!isCombined);
        mRefCombo->setEnabled(!isCombined);
    }
}

QJsonObject PluginF14CForm::getData()
{
    QJsonObject data;
    const QLocale locale=QLocale();
    const double a = locale.toDouble(mAverageEdit->text());
    const double e = locale.toDouble(mErrorEdit->text());

    const QString c = mRefCombo->currentText();

    data.insert(DATE_F14C_FRACTION_STR, a);
    data.insert(DATE_F14C_ERROR_STR, e);
    data.insert(DATE_F14C_REF_CURVE_STR, c);

    mSelectedRefCurve = c;

    return data;
}

void PluginF14CForm::errorIsValid(QString str)
{
    (void) str;
    emit PluginFormAbstract::OkEnabled(mErrorEdit->hasAcceptableInput());

}

void PluginF14CForm::valueIsValid(QString str)
{
    (void) str;
    emit PluginFormAbstract::OkEnabled(mAverageEdit->hasAcceptableInput());
}

bool PluginF14CForm::isValid()
{
    const QString refCurve = mRefCombo->currentText();
    if (refCurve.isEmpty())
        mError = tr("Ref. curve is empty!");
    return !refCurve.isEmpty();
}

#endif
