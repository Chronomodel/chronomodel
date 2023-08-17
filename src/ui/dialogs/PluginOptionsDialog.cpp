/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
