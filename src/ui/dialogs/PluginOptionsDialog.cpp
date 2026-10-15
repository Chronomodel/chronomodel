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

#include "PluginOptionsDialog.h"
#include "Button.h"
#include "Label.h"
#include "PluginManager.h"
#include "Plugin14C.h"

#include <QtWidgets>

/**
 * @todo transform this dialog with autocharging plugin
 */
PluginOptionsDialog::PluginOptionsDialog(QWidget* parent, Qt::WindowFlags flags):
    QDialog(parent, flags)
{
    setWindowTitle(tr("Data Options"));

    mTitleLab = new Label(tr("Data Options"), this);
    mTitleLab->setIsTitle(true);

    mC14RefLab = new Label(tr("Change all 14C references to : "), this);
    mC14RefCombo = new QComboBox(this);
    mComboH = mC14RefCombo->sizeHint().height();
    Plugin14C* plugin14C = (Plugin14C*)PluginManager::getPluginFromName("14C");
    QStringList refCurves = plugin14C->getRefsNames();
    for (int i = 0; i<refCurves.size(); ++i)
        mC14RefCombo->addItem(refCurves[i]);

    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);

    mOkBut->setAutoDefault(true);

    connect(mOkBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this,  &PluginOptionsDialog::accept);
    connect(mCancelBut,  static_cast<void (Button::*)(bool)> (&Button::clicked), this,  &PluginOptionsDialog::reject);

    setFixedSize(500, 90);
}

PluginOptionsDialog::~PluginOptionsDialog()
{

}

QString PluginOptionsDialog::getC14Ref() const
{
    return mC14RefCombo->currentText();
}

void PluginOptionsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);

    const int m = 5;
    const int lineH = 20;
    const int w1 = 200;
    const int w2 = width() - 3*m - w1;
    const int butW = 80;
    const int butH = 25;

    int y = -lineH;

    mTitleLab->setGeometry(m, y += (lineH + m), width() - 2*m, lineH);

    mC14RefLab->setGeometry(m, y += (lineH + m), w1, mComboH);
    mC14RefCombo->setGeometry(2*m + w1, y, w2, mComboH);

    mOkBut->setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

void PluginOptionsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(180, 180, 180));
}
