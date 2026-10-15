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

#include "PluginUniformForm.h"
#if USE_PLUGIN_UNIFORM

#include "PluginUniform.h"

#include <QJsonObject>
#include <QtWidgets>

PluginUniformForm::PluginUniformForm(PluginUniform* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Uniform Prior"), parent, flags)
{
    mMinLab = new QLabel(tr("Lower date (BC/AD)"), this);
    mMaxLab = new QLabel(tr("Upper date (BC/AD)"), this);

    mMinEdit = new QLineEdit(this);
    mMinEdit->setAlignment(Qt::AlignHCenter);
    mMinEdit->setText("0");
    mMinEdit->setStyleSheet("QLineEdit { border: 0px; }");

    mMaxEdit = new QLineEdit(this);
    mMaxEdit->setAlignment(Qt::AlignHCenter);
    mMaxEdit->setText("100");
    setTabOrder(mMinEdit, mMaxEdit);
    mMaxEdit->setStyleSheet("QLineEdit { border: 0px; }");

    connect(mMinEdit, &QLineEdit::textChanged, this, &PluginUniformForm::valuesAreValid);
    connect(mMaxEdit, &QLineEdit::textChanged, this, &PluginUniformForm::valuesAreValid);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 3, 0, 0);

    grid->addWidget(mMinLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mMinEdit, 0, 1);

    grid->addWidget(mMaxLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mMaxEdit, 1, 1);

    setLayout(grid);
}

PluginUniformForm::~PluginUniformForm()
{

}

void PluginUniformForm::setData(const QJsonObject& data, bool isCombined)
{
    mMinEdit->setEnabled(!isCombined);
    mMaxEdit->setEnabled(!isCombined);

    if (!isCombined) {
        const QLocale locale;
        const double min = data.value(DATE_UNIFORM_MIN_STR).toDouble();
        const double max = data.value(DATE_UNIFORM_MAX_STR).toDouble();

        mMinEdit->setText(locale.toString(min));
        mMaxEdit->setText(locale.toString(max));
    }
}

QJsonObject PluginUniformForm::getData()
{
    QJsonObject data;
    const QLocale locale;

    const double min = locale.toDouble(mMinEdit->text());
    const double max = locale.toDouble(mMaxEdit->text());

    data.insert(DATE_UNIFORM_MIN_STR, min);
    data.insert(DATE_UNIFORM_MAX_STR, max);

    return data;
}

void PluginUniformForm::valuesAreValid(QString str)
{
    (void) str;
    bool oka, okb;
    const QLocale locale;
    const double a = locale.toDouble(mMinEdit->text(), &oka);
    const double b = locale.toDouble(mMaxEdit->text(), &okb);

    emit PluginFormAbstract::OkEnabled(oka && okb && (a<b) );
}

bool PluginUniformForm::isValid()
{
    const QLocale locale;
    const double min = locale.toDouble(mMinEdit->text());
    const double max = locale.toDouble(mMaxEdit->text());
    if (min >= max)
        mError = tr("Forbidden : lower date must be < upper date");

    return min < max;
}

#endif
