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
