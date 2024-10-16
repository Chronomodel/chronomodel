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

#include "StudyPeriodDialog.h"
#include "LineEdit.h"

#include <QtWidgets>


StudyPeriodDialog::StudyPeriodDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
    mMargin(5),
    mLineH(20),
    mButW(80),
    mButH(25)
{
   setWindowTitle(tr("Study Period Settings"));

    // -----------
   mMinLab = new QLabel(tr("Start (BC/AD)"), this);
   mMinEdit = new LineEdit(this);

   mMaxLab = new QLabel(tr("End (BC/AD)"), this);
   mMaxEdit = new LineEdit(this);

   QGridLayout* grid = new QGridLayout();
   grid->setContentsMargins(0, 0, 0, 0);
   grid->addWidget(mMinLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mMinEdit, 0, 1);
   grid->addWidget(mMaxLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mMaxEdit, 1, 1);

   connect(mMinEdit, &LineEdit::textChanged, this, &StudyPeriodDialog::setOkEnabled);
   connect(mMaxEdit, &LineEdit::textChanged, this, &StudyPeriodDialog::setOkEnabled);
   // ----------

    mAdvancedCheck = new QCheckBox(tr("Advanced"));
    mAdvancedWidget = new QGroupBox();
    mAdvancedWidget->setCheckable(false);
    mAdvancedWidget->setVisible(false);
    mAdvancedWidget->setFlat(true);
    connect(mAdvancedCheck, &QCheckBox::toggled, this, &StudyPeriodDialog::setAdvancedVisible);

    mForcedLab   = new QLabel(tr("Force resolution"), mAdvancedWidget);
    mForcedCheck = new QCheckBox(mAdvancedWidget);
    mStepLab     = new QLabel(tr("Resolution in years"), mAdvancedWidget);

    mStepSpin = new QDoubleSpinBox(mAdvancedWidget);
    mStepSpin -> setRange(0.001, 10000);
    mStepSpin -> setSingleStep(0.001);
    mStepSpin -> setDecimals(4);

    QGridLayout* advGrid = new QGridLayout();
    advGrid->setContentsMargins(0, 0, 0, 0);
    advGrid->addWidget(mForcedLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mForcedCheck, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    advGrid->addWidget(mStepLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mStepSpin, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

    connect(mForcedCheck, &QCheckBox::toggled, mStepSpin, &QDoubleSpinBox::setEnabled);
    //connect(mForcedCheck, &QCheckBox::toggled, this, &StudyPeriodDialog::showMessageStepForced);

    mAdvancedWidget->setLayout(advGrid);

    // ----------

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &StudyPeriodDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &StudyPeriodDialog::reject);

    mLayout = new QVBoxLayout();
    mLayout->addLayout(grid);

    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    mLayout->addWidget(line1);

    mLayout->addWidget(mAdvancedCheck);
    mLayout->addWidget(mAdvancedWidget);
    mLayout->addWidget(mButtonBox);
    mLayout->addStretch();
    setLayout(mLayout);

    setMinimumWidth(300);

    updateVisibleControls();
}

StudyPeriodDialog::~StudyPeriodDialog()
{
    disconnect(mForcedCheck, &QCheckBox::toggled, this, &StudyPeriodDialog::showMessageStepForced);
}

void StudyPeriodDialog::setSettings(const StudyPeriodSettings &s)
{
    mMinEdit->setText(locale().toString(s.mTmin));
    mMaxEdit->setText(locale().toString(s.mTmax));
    //const double suggested = s.getStep(s.mTmin, s.mTmax);
    // mForcedCheck -> setText(tr("(suggested/default value = %1 )").arg(QString::number(suggested) ) );
    mForcedCheck -> setChecked(s.mStepForced);
    mStepSpin    -> setEnabled(s.mStepForced);
    mStepSpin    -> setValue(s.mStep);

    mAdvancedCheck->setChecked(s.mStepForced);
    connect(mForcedCheck, &QCheckBox::toggled, this, &StudyPeriodDialog::showMessageStepForced);
}

void StudyPeriodDialog::setStep(double step, bool forced, double suggested)
{
    mForcedCheck -> setText(tr("(suggested/default value = %1 )").arg(QString::number(suggested) ) );
    mForcedCheck -> setChecked(forced);
    mStepSpin    -> setEnabled(forced);
    mStepSpin    -> setValue(step);
}

StudyPeriodSettings StudyPeriodDialog::getSettings() const
{
    StudyPeriodSettings s = StudyPeriodSettings();
    s.mTmin = locale().toDouble(mMinEdit->text());
    s.mTmax = locale().toDouble(mMaxEdit->text());
    if (mForcedCheck->isChecked())
        s.mStep = mStepSpin->value();
    else {
        const double suggested = s.getStep(s.mTmin, s.mTmax);
        s.mStep = suggested;
    }
    s.mStepForced = mForcedCheck -> isChecked();
    return s;
}

double StudyPeriodDialog::step() const
{
    return mStepSpin -> value();
}


void StudyPeriodDialog::setOkEnabled(const QString &text)
{
    (void) text;
    bool minOk (false);
    bool maxOk (false);
    const double min = locale().toDouble(mMinEdit->text(), &minOk);
    const double max = locale().toDouble(mMaxEdit->text(), &maxOk);
    const bool enable = ( min< max) && minOk && maxOk;
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}


void StudyPeriodDialog::updateVisibleControls()
{
    mForcedLab->setVisible(mAdvancedCheck->isChecked());
    mForcedCheck->setVisible(mAdvancedCheck->isChecked());

    mStepLab->setVisible(mAdvancedCheck->isChecked());
    mStepSpin->setVisible(mAdvancedCheck->isChecked());

    adjustSize();
}

void StudyPeriodDialog::setAdvancedVisible(bool visible)
{
    mAdvancedWidget->setVisible(visible);
    if (visible)
        updateVisibleControls();
    else
        adjustSize();
}
void StudyPeriodDialog::showMessageStepForced(bool forced)
{
    if (forced) {
        /*QMessageBox message(QMessageBox::Information,
                        tr("Study Period Step Forced"),
                        tr("Forcing the step affects all calibrations.\rCalculation time and results are also affected."),
                            QMessageBox::Ok,
                        this);*/
        QMessageBox message;
        message.setText("Study Period Step Forced");
        //message.setInformativeText("Do you want to save your changes?");
        message.setStandardButtons(QMessageBox::Ok );
        message.setDefaultButton(QMessageBox::Ok);

        //const QString text_show = tr("Details ...");
        //const QString text_hide = tr("Hide Details");

        QString detail = "All the calibrations will be recalculated with this same step.\r";
        detail += "In cases where reference curves are defined over large time ranges and when you choose a very fine step size, all the calculations will be much longer.";
        detail +=" \rConversely, if your step size is too large in relation to the definition of the reference curve,";
        detail += " the calculation of the calibration curves will be coarse and may miss time solutions...";
        //detail += "\rBy default, the algorithm defines a different step for each calibration curve, looking for the optimum step.";

        //QPushButton* bt_details = message.addButton( text_show, QMessageBox::ActionRole );
        message.setInformativeText(detail);
        message.exec();

        //message.setDetailedText(detail);
        //auto result = message.exec();
        //auto res_but = message.clickedButton();
        /*
        while (message.exec() != QMessageBox::Ok) {
            if (message.clickedButton() == bt_details) {
                if (message.informativeText().isEmpty()) {
                    message.setInformativeText(detail);

                    //message.setAccessibleDescription("titi");
                    //message.removeButton(bt_details);

                   // bt_details = message.addButton( text_hide, QMessageBox::ActionRole );

                } else {
                    message.setInformativeText("");
                    //bt_details->setText(text_show);
                    //bt_details->click();
                    //message.removeButton(bt_details);

                    //bt_details = message.addButton(text_show, QMessageBox::ActionRole );
                }
            }

            message.update();
           // result = message.exec();
        } ;
        */
    }
}
