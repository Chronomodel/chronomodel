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

#include "EventDialog.h"

#include "ColorPicker.h"
#include "LineEdit.h"
#include "QtUtilities.h"
#include <QGridLayout>
#include <QDialogButtonBox>


EventDialog::EventDialog(QWidget* parent, const QString &title, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(title);

    // -----------

    QLabel* nameLab = new QLabel(tr("Name"), this);
    nameLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    QLabel* colorLab = new QLabel(tr("Color"), this);
    colorLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    mNameEdit = new LineEdit(this);
    mNameEdit->setText(tr("No name"));
   // mNameEdit->setStyleSheet("QLineEdit { border-radius: 5px; }");
    mNameEdit->selectAll();
    mNameEdit->setFocus();

    mColorPicker = new ColorPicker();
    mColorPicker->setColor(randomColor());

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addWidget(nameLab, 0, 0);
    gridLayout->addWidget(mNameEdit, 0, 1);
    gridLayout->addWidget(colorLab, 1, 0);
    gridLayout->addWidget(mColorPicker, 1, 1);

    // ----------

    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &EventDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EventDialog::reject);

    // ----------

    QFont font;
    font.setWeight(QFont::Bold);

    QLabel* titleLab = new QLabel(title, this);
    titleLab->setFont(font);
    titleLab->setAlignment(Qt::AlignCenter);

    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    // ----------

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(titleLab);
    layout->addWidget(separator);
    layout->addLayout(gridLayout);
    layout->addWidget(buttonBox);
    setLayout(layout);

    setFixedWidth(300);
}

EventDialog::~EventDialog()
{

}

QString EventDialog::getName() const
{
    return mNameEdit->text();
}

QColor EventDialog::getColor() const
{
    return mColorPicker->getColor();
}
