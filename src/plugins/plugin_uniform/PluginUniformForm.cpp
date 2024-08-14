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

    mMaxEdit = new QLineEdit(this);
    mMaxEdit->setAlignment(Qt::AlignHCenter);
    mMaxEdit->setText("100");

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
