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

#include "DateDialog.h"

#include "PluginFormAbstract.h"
#include "HelpWidget.h"
#include "PluginAbstract.h"

#include <QtWidgets>


DateDialog::DateDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
    mWiggleIsValid(false),
    mPluginDataAreValid(false),
    mForm(nullptr),
    mWidth(600),
    mMargin(5),
    mLineH(20),
    mButW(80),
    mButH(25),
    mWiggleEnabled(false)
{
    setWindowTitle(tr("Create / Modify Data"));
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

    // -----------

    mNameLab = new QLabel(tr("Name"), this);
    mNameEdit = new QLineEdit(this);
    mNameEdit->setAlignment(Qt::AlignHCenter);
    mNameEdit->setText("New Data");
    mNameEdit->QWidget::setStyleSheet("QLineEdit { border: 0px; }");
    mNameEdit->selectAll();
    mNameEdit->setFocus();

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(mNameLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mNameEdit, 0, 1);

    // ----------

    mAdvancedCheck = new QCheckBox(tr("Advanced"));
    mAdvancedWidget = new QGroupBox();
    //mAdvancedWidget->setStyleSheet("QGroupBox {     font: bold;     font-size: 23px;     border: 1px solid silver;     border-radius: 6px;     margin-top: 15px; }");
    mAdvancedWidget->setStyleSheet("QGroupBox {border: 0px}");

    mAdvancedWidget->setCheckable(false);
    mAdvancedWidget->setVisible(false);
    mAdvancedWidget->setFlat(false);
    connect(mAdvancedCheck, &QCheckBox::toggled, this, &DateDialog::setAdvancedVisible);

    mMethodLab = new QLabel(tr("MCMC"), mAdvancedWidget);
    mMethodCombo = new QComboBox(mAdvancedWidget);
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eMHPrior));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eInversion));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eMHAdaptGauss));

    mWiggleLab = new QLabel(tr("Wiggle Matching"), mAdvancedWidget);

    mDeltaNoneRadio = new QRadioButton(tr("None"), mAdvancedWidget);
    mDeltaFixedRadio = new QRadioButton(tr("Fixed"), mAdvancedWidget);
    mDeltaRangeRadio = new QRadioButton(tr("Range"), mAdvancedWidget);
    mDeltaGaussRadio = new QRadioButton(tr("Gaussian"), mAdvancedWidget);
    mDeltaNoneRadio->setChecked(true);

    connect(mDeltaNoneRadio, &QRadioButton::toggled, this, &DateDialog::updateVisibleControls);
    connect(mDeltaFixedRadio, &QRadioButton::toggled, this, &DateDialog::updateVisibleControls);
    connect(mDeltaRangeRadio, &QRadioButton::toggled, this,  &DateDialog::updateVisibleControls);
    connect(mDeltaGaussRadio, &QRadioButton::toggled, this,  &DateDialog::updateVisibleControls);

    connect(mDeltaFixedRadio, &QRadioButton::toggled, this, &DateDialog::checkWiggle);
    connect(mDeltaRangeRadio, &QRadioButton::toggled, this,  &DateDialog::checkWiggle);
    connect(mDeltaGaussRadio, &QRadioButton::toggled, this,  &DateDialog::checkWiggle);

    mDeltaHelp = new HelpWidget(tr("Wiggle Sign : \n \"+\" if data ≤ event,\n \"-\" if data ≥ event"), mAdvancedWidget);
    mDeltaHelp->setFixedHeight(70);
    mDeltaHelp->setLink("https://chronomodel.com/storage/medias/83_chronomodel_v32_user_manual_2024_05_13_min.pdf#page=22");

    mDeltaFixedLab = new QLabel(tr("Value"), mAdvancedWidget);
    mDeltaMinLab = new QLabel(tr("Min"), mAdvancedWidget);
    mDeltaMaxLab = new QLabel(tr("Max"), mAdvancedWidget);
    mDeltaAverageLab = new QLabel(tr("Mean"), mAdvancedWidget);
    mDeltaErrorLab = new QLabel(tr("Error (sd)"), mAdvancedWidget);

    mDeltaFixedEdit = new QLineEdit(mAdvancedWidget);
    mDeltaFixedEdit->setAlignment(Qt::AlignHCenter);
    mDeltaMinEdit = new QLineEdit(mAdvancedWidget);
    mDeltaMinEdit->setAlignment(Qt::AlignHCenter);
    mDeltaMaxEdit = new QLineEdit(mAdvancedWidget);
    mDeltaMaxEdit->setAlignment(Qt::AlignHCenter);
    mDeltaAverageEdit = new QLineEdit(mAdvancedWidget);
    mDeltaAverageEdit->setAlignment(Qt::AlignHCenter);
    mDeltaErrorEdit = new QLineEdit(mAdvancedWidget);
    mDeltaErrorEdit->setAlignment(Qt::AlignHCenter);

    mDeltaFixedEdit->setText(QString::number(0));
    mDeltaMinEdit->setText(QString::number(0));
    mDeltaMaxEdit->setText(QString::number(0));
    mDeltaAverageEdit->setText(QString::number(0));
    mDeltaErrorEdit->setText(QString::number(0));

    connect(mDeltaFixedEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaMinEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaMaxEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaAverageEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaErrorEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);

    QGridLayout* advGrid = new QGridLayout();

    advGrid->addWidget(mMethodLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mMethodCombo, 0, 1);
    advGrid->addWidget(mWiggleLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaNoneRadio, 1, 1);
    advGrid->addWidget(mDeltaFixedRadio, 2, 1);
    advGrid->addWidget(mDeltaRangeRadio, 3, 1);
    advGrid->addWidget(mDeltaGaussRadio, 4, 1);
    advGrid->addWidget(mDeltaHelp, 5, 1);

    advGrid->addWidget(mDeltaFixedLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaFixedEdit, 6, 1);

    advGrid->addWidget(mDeltaMinLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaMinEdit, 6, 1);
    advGrid->addWidget(mDeltaMaxLab, 7, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaMaxEdit, 7, 1);

    advGrid->addWidget(mDeltaAverageLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaAverageEdit, 6, 1);
    advGrid->addWidget(mDeltaErrorLab, 7, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaErrorEdit, 7, 1);

    mAdvancedWidget->setLayout(advGrid);
    mAdvancedWidget->layout()->setContentsMargins(20, 20, 20, 20);

    // ----------

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &DateDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &DateDialog::reject);

    mLayout = new QVBoxLayout();
    mLayout->setContentsMargins(5, 5, 5, 5);
    mLayout->setSpacing(5);

    mLayout->addLayout(grid);

    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    mLayout->addWidget(line1);

    // Plugin form will be put here when ready!

    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    mLayout->addWidget(line2);

    mLayout->addWidget(mAdvancedCheck);
    mLayout->addWidget(mAdvancedWidget);
    mLayout->addWidget(mButtonBox);
    mLayout->addStretch(); 

    setLayout(mLayout);

    updateVisibleControls();
}

DateDialog::~DateDialog()
{
    if (mForm) {
        disconnect(mForm, &PluginFormAbstract::OkEnabled, this, &DateDialog::setOkEnabled);
        disconnect(mForm, &PluginFormAbstract::sizeChanged , this, &DateDialog::updateVisibleControls);
    }
}

void DateDialog::setForm(PluginFormAbstract* form)
{
    if (mForm) {
        mForm->setVisible(false);
        mForm->setParent(nullptr);
    }
    if (form) {

        mForm = form;
        connect(mForm, &PluginFormAbstract::OkEnabled, this, &DateDialog::setPluginDataValid);
        connect(mForm, &PluginFormAbstract::sizeChanged , this, &DateDialog::updateVisibleControls);
        mForm->layout()->setContentsMargins(10, 10, 10, 10);
        mForm->setFlat(true);
        mForm->setStyleSheet("QGroupBox {border: 0px}");
        mLayout->insertWidget(2, mForm);

        PluginAbstract* plugin = form->mPlugin;

        setWindowTitle(tr("Create %1 Data").arg(plugin->getName()));
        // Check if wiggle is allowed by plugin
        mWiggleIsValid = true;
        mPluginDataAreValid = true;

        if (plugin->wiggleAllowed())
            setWiggleEnabled(true);

        else
            setWiggleEnabled(false);


        // Disable methods forbidden by plugin
        const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(mMethodCombo->model());

        const QList<MHVariable::SamplerProposal> allowMetho = plugin->allowedDataMethods();
        MHVariable::SamplerProposal spTest;
        for (int i=0; i<mMethodCombo->count(); ++i) {
            QStandardItem* item = model->item(i);

            switch (i) {
            case 0 :
                spTest = MHVariable::eMHPrior;
                break;
            case 1 :
                spTest = MHVariable::eInversion;
                break;
            case 2 :
                spTest = MHVariable::eMHAdaptGauss;
                break;
            default :
                spTest = MHVariable::eInversion;
                break;
            }

            const bool allowed = allowMetho.contains(spTest);

            item->setFlags(!allowed ? item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled)
                           : Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            // visually disable by greying out - works only if combobox has been painted already and palette returns the wanted color
            item->setData(!allowed ? mMethodCombo->palette().color(QPalette::Disabled, QPalette::Text)
                          : QVariant(), // clear item data in order to use default color
                          Qt::ForegroundRole);

        }
    }
}

void DateDialog::setPluginDataValid(bool valid)
{
    mPluginDataAreValid = valid;
    setOkEnabled();
}

void DateDialog::checkWiggle()
{
    if (mDeltaFixedRadio->isChecked()) {
        bool ok1 = true;
        mDeltaFixedEdit->text().toInt(&ok1);
        mWiggleIsValid = ok1;

    } else if (mDeltaRangeRadio->isChecked()) {
        bool ok1 = true;
        bool ok2 = true;
        const int dmin = mDeltaMinEdit->text().toInt(&ok1);
        const int dmax = mDeltaMaxEdit->text().toInt(&ok2);
        mWiggleIsValid = ( ok1 && ok2 &&  (dmax>dmin) );

    } else if(mDeltaGaussRadio->isChecked()) {
        bool ok1 = true;
        bool ok2 = true;
        //const double a =
        mDeltaAverageEdit->text().toDouble(&ok1);
        const double e = mDeltaErrorEdit->text().toDouble(&ok2);
        mWiggleIsValid = ( ok1 && ok2 &&  (e>0) );

    } else
        mWiggleIsValid = true;

    setOkEnabled();
}

void DateDialog::setOkEnabled()
{
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(mWiggleIsValid && mPluginDataAreValid);
}

void DateDialog::setWiggleEnabled(bool enabled)
{
    mWiggleEnabled = enabled;

    mWiggleLab->setVisible(enabled);
    mDeltaHelp->setVisible(enabled);

    mDeltaNoneRadio->setVisible(enabled);
    mDeltaFixedRadio->setVisible(enabled);
    mDeltaRangeRadio->setVisible(enabled);
    mDeltaGaussRadio->setVisible(enabled);

    mDeltaFixedLab->setVisible(enabled);
    mDeltaMinLab->setVisible(enabled);
    mDeltaMaxLab->setVisible(enabled);
    mDeltaAverageLab->setVisible(enabled);
    mDeltaErrorLab->setVisible(enabled);

    mDeltaFixedEdit->setVisible(enabled);
    mDeltaMinEdit->setVisible(enabled);
    mDeltaMaxEdit->setVisible(enabled);
    mDeltaAverageEdit->setVisible(enabled);
    mDeltaErrorEdit->setVisible(enabled);

    adjustSize();
}

void DateDialog::updateVisibleControls()
{
    mDeltaFixedLab->setVisible(mWiggleEnabled && mDeltaFixedRadio->isChecked());
    mDeltaFixedEdit->setVisible(mWiggleEnabled && mDeltaFixedRadio->isChecked());

    mDeltaMinLab->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());
    mDeltaMinEdit->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());
    mDeltaMaxLab->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());
    mDeltaMaxEdit->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());

    mDeltaAverageLab->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());
    mDeltaAverageEdit->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());
    mDeltaErrorLab->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());
    mDeltaErrorEdit->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());

    adjustSize();
}

void DateDialog::setAdvancedVisible(bool visible)
{
    mAdvancedWidget->setVisible(visible);
    if (visible)
        updateVisibleControls();
    else
        adjustSize();
}

void DateDialog::setDataMethod(MHVariable::SamplerProposal sp)
{
    int index;
    switch (sp) {
    case MHVariable::eMHPrior :
        index = 0;
        break;
    case MHVariable::eInversion:
        index = 1;
        break;
    case MHVariable::eMHAdaptGauss:
        index = 2;
        break;
    // The following cases are not for data Method
    case MHVariable::eFixe:
    case MHVariable::eDoubleExp:
    case MHVariable::eBoxMuller:
    //case MHVariable::eMHAdaptGauss:
    default:
        index = -1;
        break;
    }

    mMethodCombo->setCurrentIndex(index);
}

void DateDialog::setDate(const QJsonObject& date)
{
    mNameEdit->setText(date.value(STATE_NAME).toString());
    setDataMethod( (MHVariable::SamplerProposal)date.value(STATE_DATE_SAMPLER).toInt());

    Date::DeltaType deltaType = Date::DeltaType (date.value(STATE_DATE_DELTA_TYPE).toInt());

    mDeltaNoneRadio->setChecked(deltaType == Date::eDeltaNone);
    mDeltaFixedRadio->setChecked(deltaType == Date::eDeltaFixed);
    mDeltaRangeRadio->setChecked(deltaType == Date::eDeltaRange);
    mDeltaGaussRadio->setChecked(deltaType == Date::eDeltaGaussian);

    // by convention all delta parameter are integer, so we don't need to convert with the preference setting Decimal format
    // with a QLocale
    mDeltaFixedEdit->setText(QString::number(date.value(STATE_DATE_DELTA_FIXED).toDouble()));
    mDeltaMinEdit->setText(QString::number(date.value(STATE_DATE_DELTA_MIN).toDouble()));
    mDeltaMaxEdit->setText(QString::number(date.value(STATE_DATE_DELTA_MAX).toDouble()));
    mDeltaAverageEdit->setText(QString::number(date.value(STATE_DATE_DELTA_AVERAGE).toDouble()));
    mDeltaErrorEdit->setText(QString::number(date.value(STATE_DATE_DELTA_ERROR).toDouble()));

//    if (deltaType == Date::eDeltaNone) {
//        mDeltaFixedRadio->setChecked(deltaType == Date::eDeltaFixed);
//        mDeltaFixedEdit->setText(QString::number(0));
//    }
    // if data are in the JSON they must be valid
    mPluginDataAreValid = true;
    mWiggleIsValid = true;
    mAdvancedCheck->setEnabled(date.value(STATE_DATE_ORIGIN).toInt() != Date::eCombination);

    setOkEnabled();
    // open the display panel if there is wiggle parameter
    if ( (mDeltaFixedRadio->isChecked() && mDeltaFixedEdit->text().toDouble() != 0.) ||
        (mDeltaRangeRadio->isChecked() && (mDeltaMinEdit->text().toDouble() != 0. || mDeltaMaxEdit->text().toDouble() != 0.) ) ||
        (mDeltaGaussRadio->isChecked() && mDeltaErrorEdit->text().toDouble()>0) ) {

        mAdvancedCheck->setChecked(true);
    }

    mNameEdit->selectAll();
    mNameEdit->setFocus();

    if (mForm)
        mForm->setData(date.value(STATE_DATE_DATA).toObject(), date.value(STATE_DATE_SUB_DATES).toArray().size() > 0);

}

QString DateDialog::getName() const {return mNameEdit->text();}
double DateDialog::getDeltaFixed() const {return mDeltaFixedEdit->text().toDouble();}
double DateDialog::getDeltaMin() const {return mDeltaMinEdit->text().toDouble();}
double DateDialog::getDeltaMax() const {return mDeltaMaxEdit->text().toDouble();}
double DateDialog::getDeltaAverage() const {return mDeltaAverageEdit->text().toDouble();}
double DateDialog::getDeltaError() const {return mDeltaErrorEdit->text().toDouble();}

MHVariable::SamplerProposal DateDialog::getMethod() const
{
    MHVariable::SamplerProposal sampler = MHVariable::eMHPrior;
    if (mMethodCombo->currentIndex() == 1)
        sampler = MHVariable::eInversion;

    else if (mMethodCombo->currentIndex() == 2)
        sampler = MHVariable::eMHAdaptGauss;

    return sampler;
}

Date::DeltaType DateDialog::getDeltaType() const
{
    if (!mWiggleIsValid)
        return Date::eDeltaNone;

    else if (mDeltaFixedRadio->isChecked() && mDeltaFixedEdit->text().toDouble() != 0. )
        return Date::eDeltaFixed;

    else if (mDeltaRangeRadio->isChecked() && (mDeltaMaxEdit->text().toDouble()-mDeltaMinEdit->text().toDouble()) > 0. )
        return Date::eDeltaRange;

    else if (mDeltaGaussRadio->isChecked() && mDeltaErrorEdit->text().toDouble() > 0. )
        return Date::eDeltaGaussian;

    else
        return Date::eDeltaNone;
}
