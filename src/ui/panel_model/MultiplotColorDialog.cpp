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
#include "MultiplotColorDialog.h"


MultiplotColorDialog::MultiplotColorDialog(QWidget *parent)
{
    MultiplotColorDialog( false, false, Qt::gray, parent);
}

MultiplotColorDialog::MultiplotColorDialog(bool usePluginColor, bool useEventColor, QColor selectedColor, QWidget *parent):
    QDialog (parent),
    mSelectedColor (selectedColor),
    mUsePluginColor (usePluginColor),
    mUseEventColor (useEventColor)
{
    // Création des boutons
    colorDialogButton = new QPushButton(this);

    colorDialogButton->setStyleSheet(QString("background-color: %1;").arg(mSelectedColor.name()));
    connect(colorDialogButton, &QPushButton::clicked, this, &MultiplotColorDialog::selectColor);

    mPluginRadio = new QRadioButton(tr("Data Plugin Color"));
    mEventRadio = new QRadioButton(tr("Event Color"));
    mCustomRadio = new QRadioButton(tr("Custom Color"));

    mRadioGroup = new QButtonGroup(this);
    mRadioGroup->addButton(mPluginRadio);
    mRadioGroup->addButton(mEventRadio);
    mRadioGroup->addButton(mCustomRadio);


    mPluginRadio->setChecked(mUsePluginColor);
    mEventRadio->setChecked(mUseEventColor);
    mCustomRadio->setChecked(!mUsePluginColor && !mUseEventColor);

    colorDialogButton->setVisible(!mUsePluginColor && !mUseEventColor);

    connect(mRadioGroup, &QButtonGroup::buttonClicked, this, &MultiplotColorDialog::updateOptions);

    okButton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(okButton, &QDialogButtonBox::accepted, this, &MultiplotColorDialog::accept);
    connect(okButton, &QDialogButtonBox::rejected, this, &MultiplotColorDialog::reject);

    // Layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mPluginRadio);
    layout->addWidget(mEventRadio);
    layout->addWidget(mCustomRadio);
    layout->addWidget(colorDialogButton);

    layout->addWidget(okButton);

    setWindowTitle("Data Color");
    resize(200, 200);
}

void MultiplotColorDialog::selectColor()
{
    QColor newColor = QColorDialog::getColor(mSelectedColor);
    if (newColor.isValid()) {
        mSelectedColor = newColor;
        colorDialogButton->setStyleSheet(QString("background-color: %1;").arg(mSelectedColor.name()));
    }
}

void MultiplotColorDialog::updateOptions()
{
    QAbstractButton *checkedButton = mRadioGroup->checkedButton();
    if (checkedButton == mPluginRadio) {
        // couleur des Data
        mUsePluginColor = true;
        mUseEventColor = false;
        colorDialogButton->setVisible(false);
    }
    else if (checkedButton == mEventRadio) {
        // couleur des Events
        mUsePluginColor = false;
        mUseEventColor = true;
        colorDialogButton->setVisible(false);
    }
    else if (checkedButton == mCustomRadio) {
        // couleur personnalisée
        mUsePluginColor = false;
        mUseEventColor = false;
        // Met à jour la couleur affichée
        colorDialogButton->setStyleSheet(
            QString("background-color: %1;").arg(mSelectedColor.name())
            );
        colorDialogButton->setVisible(true);
    }


}
