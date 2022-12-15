/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "PluginTLForm.h"
#if USE_PLUGIN_TL

#include "PluginTL.h"

#include <QJsonObject>
#include <QtWidgets>

PluginTLForm::PluginTLForm(PluginTL* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("TL Measurements"), parent, flags)
{
   // PluginTL* pluginTL = (PluginTL*)mPlugin;

    mAverageLab = new QLabel(tr("Age"), this);
    mErrorLab = new QLabel(tr("Error (sd)"), this);
    mYearLab = new QLabel(tr("Ref. year"), this);

    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setAlignment(Qt::AlignHCenter);
    mAverageEdit->setText("0");
    QDoubleValidator* RValidator = new QDoubleValidator();
    mAverageEdit->setValidator(RValidator);

    mErrorEdit = new QLineEdit(this);
    mErrorEdit->setAlignment(Qt::AlignHCenter);
    mErrorEdit->setText("30");
    QDoubleValidator* RplusValidator = new QDoubleValidator();
    RplusValidator->setBottom(0.0);
    mErrorEdit->setValidator(RplusValidator);

    connect(mErrorEdit, &QLineEdit::textChanged, this, &PluginTLForm::errorIsValid);

    mYearEdit = new QLineEdit(this);
    mYearEdit->setAlignment(Qt::AlignHCenter);
    mYearEdit->setText(QString::number(QDate::currentDate().year()));

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 5, 0, 0);

    grid->addWidget(mAverageLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAverageEdit, 0, 1);

    grid->addWidget(mErrorLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mErrorEdit, 1, 1);

    grid->addWidget(mYearLab, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mYearEdit, 2, 1);

    setLayout(grid);
}

PluginTLForm::~PluginTLForm()
{

}

void PluginTLForm::setData(const QJsonObject& data, bool isCombined)
{
    (void) isCombined;
    QLocale locale=QLocale();
    double a = data.value(DATE_TL_AGE_STR).toDouble();
    double e = data.value(DATE_TL_ERROR_STR).toDouble();
    double y = data.value(DATE_TL_REF_YEAR_STR).toDouble();

    mAverageEdit->setText(locale.toString(a));
    mErrorEdit->setText(locale.toString(e));
    mYearEdit->setText(locale.toString(y));
}

QJsonObject PluginTLForm::getData()
{
    QJsonObject data;

    double a = locale().toDouble(mAverageEdit->text());
    double e = locale().toDouble(mErrorEdit->text());
    double y = locale().toDouble(mYearEdit->text());

    data.insert(DATE_TL_AGE_STR, a);
    data.insert(DATE_TL_ERROR_STR, e);
    data.insert(DATE_TL_REF_YEAR_STR, y);

    return data;
}

void PluginTLForm::errorIsValid(QString str)
{
    bool ok;
    double value = locale().toDouble(str,&ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );

}

bool PluginTLForm::isValid()
{
    return true;
}
#endif
