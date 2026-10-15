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

#include "ProjectSettingsDialog.h"
#include "LineEdit.h"
#include <QtWidgets>


ProjectSettingsDialog::ProjectSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    QLabel* title = new QLabel(tr("Project Properties"));
    title->setAlignment(Qt::AlignCenter);
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);

    QLabel* intro = new QLabel(tr("<p>You can define here the project time boundaries.</p>\
                                  <p>Changing these boundaries will result in re-calibrating every dates in the project.</p>\
                                  <p>Then, you will have to re-launch the MCMC process manually.</p>"));
    intro->setTextFormat(Qt::RichText);
    intro->setWordWrap(true);

    QLabel* labelMin = new QLabel(tr("Start year") + ":");
    labelMin->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mMinEdit = new LineEdit();
    mMinEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");

    QLabel* labelMax = new QLabel(tr("End year") + ":");
    labelMax->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mMaxEdit = new LineEdit();
    mMaxEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");

    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(labelMin, 0, 0);
    gridLayout->addWidget(mMinEdit, 0, 1);
    gridLayout->addWidget(labelMax, 1, 0);
    gridLayout->addWidget(mMaxEdit, 1, 1);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->addWidget(title);
    main_layout->addWidget(intro);

    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    main_layout->addWidget(separator);

    main_layout->addLayout(gridLayout);
    main_layout->addWidget(buttonBox);
    setLayout(main_layout);

    setFixedWidth(600);
}

ProjectSettingsDialog::~ProjectSettingsDialog()
{

}

void ProjectSettingsDialog::setSettings(const ProjectSettings& settings)
{
    mMinEdit->setText(QString::number(settings.mTmin));
    mMaxEdit->setText(QString::number(settings.mTmax));
}

ProjectSettings ProjectSettingsDialog::getSettings()
{
    ProjectSettings settings;
    settings.mTmin = mMinEdit->text().toInt();
    settings.mTmax = mMaxEdit->text().toInt();

    return settings;
}
