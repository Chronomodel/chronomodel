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

#include "PluginTLForm.h"
#if USE_PLUGIN_TL

#include "PluginTL.h"

#include <QJsonObject>
#include <QtWidgets>

PluginTLForm::PluginTLForm(PluginTL* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("TL Measurements"), parent, flags)
{
    mAverageLab = new QLabel(tr("Age"), this);
    mErrorLab = new QLabel(tr("Error (sd)"), this);
    mYearLab = new QLabel(tr("Ref. year (BC/AD)"), this);

    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setAlignment(Qt::AlignHCenter);
    mAverageEdit->setText("0");
    QDoubleValidator* RValidator = new QDoubleValidator(this);
    mAverageEdit->setValidator(RValidator);
    mAverageEdit->setStyleSheet("QLineEdit { border: 0px; }");

    mErrorEdit = new QLineEdit(this);
    mErrorEdit->setAlignment(Qt::AlignHCenter);
    mErrorEdit->setText("30");
    QDoubleValidator* RplusValidator = new QDoubleValidator(this);
    RplusValidator->setBottom(0.0);
    mErrorEdit->setValidator(RplusValidator);
    mErrorEdit->setStyleSheet("QLineEdit { border: 0px; }");

    connect(mErrorEdit, &QLineEdit::textChanged, this, &PluginTLForm::errorIsValid);

    mYearEdit = new QLineEdit(this);
    mYearEdit->setAlignment(Qt::AlignHCenter);
    mYearEdit->setText(QString::number(QDate::currentDate().year()));
    mYearEdit->setStyleSheet("QLineEdit { border: 0px; }");
    setTabOrder(mAverageEdit, mErrorEdit);
    setTabOrder(mErrorEdit, mYearEdit);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 3, 0, 0);

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
    mAverageEdit->setEnabled(!isCombined);
    mErrorEdit->setEnabled(!isCombined);
    mYearEdit->setEnabled(!isCombined);

    if (!isCombined) {
        QLocale locale = QLocale();
        const double a = data.value(DATE_TL_AGE_STR).toDouble();
        const double e = data.value(DATE_TL_ERROR_STR).toDouble();
        const double y = data.value(DATE_TL_REF_YEAR_STR).toDouble();

        mAverageEdit->setText(locale.toString(a));
        mErrorEdit->setText(locale.toString(e));
        mYearEdit->setText(locale.toString(y));
    }

}

QJsonObject PluginTLForm::getData()
{
    QJsonObject data;

    const double a = QLocale().toDouble(mAverageEdit->text());
    const double e = QLocale().toDouble(mErrorEdit->text());
    const double y = QLocale().toDouble(mYearEdit->text());

    data.insert(DATE_TL_AGE_STR, a);
    data.insert(DATE_TL_ERROR_STR, e);
    data.insert(DATE_TL_REF_YEAR_STR, y);

    return data;
}

void PluginTLForm::errorIsValid(QString str)
{
    (void) str;
    emit PluginFormAbstract::OkEnabled(mErrorEdit->hasAcceptableInput());
}

bool PluginTLForm::isValid()
{
    return true;
}
#endif
