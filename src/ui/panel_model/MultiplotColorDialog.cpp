/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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
