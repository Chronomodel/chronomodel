#include "DateDialog.h"
#include "../PluginFormAbstract.h"
#include "Collapsible.h"
#include "RadioButton.h"
#include "Label.h"
#include "HelpWidget.h"
#include "LineEdit.h"
#include "Painting.h"
#include "ModelUtilities.h"
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include <QtWidgets>


DateDialog::DateDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mWiggleIsValid(false),
mPluginDataAreValid(false),
mForm(0),
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
    
    mNameLab = new QLabel(tr("Name") + " :", this);
    mNameEdit = new QLineEdit(this);
    mNameEdit->setText("<New Date>");
    mNameEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mNameEdit->selectAll();
    mNameEdit->setFocus();
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(mNameLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mNameEdit, 0, 1);
    
    // ----------
    
    mAdvancedCheck = new QCheckBox(tr("Advanced"));
    mAdvancedWidget = new QGroupBox();
    mAdvancedWidget->setCheckable(false);
    mAdvancedWidget->setVisible(false);
    mAdvancedWidget->setFlat(true);
    connect(mAdvancedCheck, &QCheckBox::toggled, this, &DateDialog::setAdvancedVisible);
    
    mMethodLab = new QLabel(tr("Method") + " :", mAdvancedWidget);
    mMethodCombo = new QComboBox(mAdvancedWidget);
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eMHSymetric));
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eInversion));
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eMHSymGaussAdapt));
    
    mWiggleLab = new QLabel(tr("Wiggle Matching"), mAdvancedWidget);
    
    mDeltaFixedRadio = new QRadioButton(tr("Fixed"), mAdvancedWidget);
    mDeltaRangeRadio = new QRadioButton(tr("Range"), mAdvancedWidget);
    mDeltaGaussRadio = new QRadioButton(tr("Gaussian"), mAdvancedWidget);
    mDeltaFixedRadio->setChecked(true);
    
    connect(mDeltaFixedRadio, &QRadioButton::toggled, this, &DateDialog::updateVisibleControls);
    connect(mDeltaRangeRadio, &QRadioButton::toggled, this,  &DateDialog::updateVisibleControls);
    connect(mDeltaGaussRadio, &QRadioButton::toggled, this,  &DateDialog::updateVisibleControls);

    connect(mDeltaFixedRadio, &QRadioButton::toggled, this, &DateDialog::checkWiggle);
    connect(mDeltaRangeRadio, &QRadioButton::toggled, this,  &DateDialog::checkWiggle);
    connect(mDeltaGaussRadio, &QRadioButton::toggled, this,  &DateDialog::checkWiggle);
    
    mDeltaHelp = new HelpWidget(tr("Wiggle Sign : \"+\" if data ≤ event, \"-\" if data ≥ event"), mAdvancedWidget);
    mDeltaHelp->setFixedHeight(35);
    mDeltaHelp->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=11");
    
    mDeltaFixedLab   = new QLabel(tr("Value"), mAdvancedWidget);
    mDeltaMinLab     = new QLabel(tr("Min"), mAdvancedWidget);
    mDeltaMaxLab     = new QLabel(tr("Max"), mAdvancedWidget);
    mDeltaAverageLab = new QLabel(tr("Mean"), mAdvancedWidget);
    mDeltaErrorLab   = new QLabel(tr("Error (sd)"), mAdvancedWidget);
    
    mDeltaFixedEdit   = new QLineEdit(mAdvancedWidget);
    mDeltaMinEdit     = new QLineEdit(mAdvancedWidget);
    mDeltaMaxEdit     = new QLineEdit(mAdvancedWidget);
    mDeltaAverageEdit = new QLineEdit(mAdvancedWidget);
    mDeltaErrorEdit   = new QLineEdit(mAdvancedWidget);
    
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
    advGrid->setContentsMargins(0, 0, 0, 0);
    advGrid->addWidget(mMethodLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mMethodCombo, 0, 1);
    advGrid->addWidget(mWiggleLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaFixedRadio, 1, 1);
    advGrid->addWidget(mDeltaRangeRadio, 2, 1);
    advGrid->addWidget(mDeltaGaussRadio, 3, 1);
    advGrid->addWidget(mDeltaHelp, 4, 1);
    
    advGrid->addWidget(mDeltaFixedLab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaFixedEdit, 5, 1);
    advGrid->addWidget(mDeltaMinLab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaMinEdit, 5, 1);
    advGrid->addWidget(mDeltaMaxLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaMaxEdit, 6, 1);
    advGrid->addWidget(mDeltaAverageLab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaAverageEdit, 5, 1);
    advGrid->addWidget(mDeltaErrorLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaErrorEdit, 6, 1);

    mAdvancedWidget->setLayout(advGrid);
    
    // ----------
    
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &DateDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &DateDialog::reject);

    mLayout = new QVBoxLayout();
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
    
    setFixedWidth(600);
    
    updateVisibleControls();
}

DateDialog::~DateDialog()
{
   if (mForm)
    disconnect(mForm, &PluginFormAbstract::OkEnabled, this, &DateDialog::setOkEnabled);
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

        mLayout->insertWidget(2, mForm);
        PluginAbstract* plugin = form->mPlugin;

        setWindowTitle(tr("Create") + " " + plugin->getName() + " " + tr("Data"));
        // Check if wiggle is allowed by plugin
        mWiggleIsValid = true;
        mPluginDataAreValid = true;

        if (plugin->wiggleAllowed())
            setWiggleEnabled(true);

        else
            setWiggleEnabled(false);

        
        // Disable methods forbidden by plugin
        const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(mMethodCombo->model());
        for (int i=0; i<mMethodCombo->count(); ++i) {
            QStandardItem* item = model->item(i);
            bool allowed = plugin->allowedDataMethods().contains((Date::DataMethod)i);
            
            item->setFlags(!allowed ? item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled)
                           : Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            // visually disable by greying out - works only if combobox has been painted already and palette returns the wanted color
            item->setData(!allowed ? mMethodCombo->palette().color(QPalette::Disabled, QPalette::Text)
                          : QVariant(), // clear item data in order to use default color
                          Qt::TextColorRole);

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
        mWiggleIsValid = ( ok1 && ok2 && ( (dmax>dmin) || ( (dmin == 0) && (dmax == 0) ) ) );

    } else if(mDeltaGaussRadio->isChecked()) {
        bool ok1 = true;
        bool ok2 = true;
        const double a = mDeltaAverageEdit->text().toDouble(&ok1);
        const double e = mDeltaErrorEdit->text().toDouble(&ok2);
        mWiggleIsValid = ( ok1 && ok2 && ( (e>0) || ( (a == 0) && (e == 0) ) ) );
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

void DateDialog::setDataMethod(Date::DataMethod method)
{
    mMethodCombo->setCurrentIndex((int)method);
}

void DateDialog::setDate(const QJsonObject& date)
{
    mNameEdit->setText(date.value(STATE_NAME).toString());
    mMethodCombo->setCurrentIndex(date.value(STATE_DATE_METHOD).toInt());
    
    Date::DeltaType deltaType = (Date::DeltaType)date.value(STATE_DATE_DELTA_TYPE).toInt();
    
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

    if (deltaType == Date::eDeltaNone) {
        mDeltaFixedRadio->setChecked(deltaType == Date::eDeltaFixed);
        mDeltaFixedEdit->setText(QString::number(0));
    }
    // if data are in the JSON they must be valid
    mPluginDataAreValid = true;
    mWiggleIsValid = true;
    setOkEnabled();
    // open the display panel if there is wiggle parameter
    if ( (mDeltaFixedRadio->isChecked() && mDeltaFixedEdit->text().toDouble() != 0) ||
        (mDeltaRangeRadio->isChecked() && (mDeltaMinEdit->text().toDouble() != 0 || mDeltaMaxEdit->text().toDouble() != 0) ) ||
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

Date::DataMethod DateDialog::getMethod() const
{
    Date::DataMethod method = Date::eMHSymetric;
    if (mMethodCombo->currentIndex() == 1)
        method = Date::eInversion;
    else if (mMethodCombo->currentIndex() == 2)
        method = Date::eMHSymGaussAdapt;
    return method;
}

Date::DeltaType DateDialog::getDeltaType() const
{
    if (!mWiggleIsValid)
        return Date::eDeltaNone;

    else if (mDeltaFixedRadio->isChecked() && mDeltaFixedEdit->text().toDouble() != 0. )
        return Date::eDeltaFixed;

    else if (mDeltaRangeRadio->isChecked())
        return Date::eDeltaRange;

    else if (mDeltaGaussRadio->isChecked())
        return Date::eDeltaGaussian;

    else
        return Date::eDeltaNone;
}


