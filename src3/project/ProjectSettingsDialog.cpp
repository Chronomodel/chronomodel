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
