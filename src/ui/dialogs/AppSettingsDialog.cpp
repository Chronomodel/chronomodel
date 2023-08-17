/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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

#include "AppSettingsDialog.h"
#include "AppSettingsDialogItemDelegate.h"
#include "PluginSettingsViewAbstract.h"
#include "AppSettings.h"

#include <QtWidgets>

AppSettingsDialog::AppSettingsDialog(QWidget* parent, Qt::WindowFlags flags): QDialog(parent, flags)
{
    setWindowTitle(tr("Application Settings"));
    filesChanged = false;
    // -----------------------------
    //  General View
    // -----------------------------
    mGeneralView = new QWidget();

    mLangHelpLab = new QLabel(tr("Language is used to define how number input should be typed (using comma or dot as decimal separator). This is not related to the application translation which is not available yet!"), this);

    mLangHelpLab->setAlignment(Qt::AlignCenter);
    mLangHelpLab->setWordWrap(true);

    mLanguageLab = new QLabel(tr("Keyboard Language"), this);
    mLanguageCombo = new QComboBox(this);
//    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);

    for (int i=0; i<339; i++)
        mLanguageCombo->addItem(QLocale::languageToString(QLocale::Language (i)),QVariant(QLocale::Language(i)));

    mIconSizeLab = new QLabel(tr("Model Icons Size"), this);
    mIconSize = new QSpinBox(this);
    mIconSize->setRange(1, 5);
    mIconSize->setSingleStep(1);

    mAutoSaveLab = new QLabel(tr("Auto Save Project"), this);
    mAutoSaveCheck = new QCheckBox(this);
    mAutoSaveDelayLab = new QLabel(tr("Auto Save Interval (in minutes)"), this);
    mAutoSaveDelayEdit = new QLineEdit(this);

    QIntValidator* positiveValidator = new QIntValidator();
    positiveValidator->setBottom(1);
    mAutoSaveDelayEdit->setValidator(positiveValidator);

    mCSVCellSepLab = new QLabel(tr("CSV Cell Separator"), this);
    mCSVCellSepEdit = new QLineEdit(this);

    mCSVDecSepLab = new QLabel(tr("CSV Decimal Separator"), this);
    mCSVDecSepCombo = new QComboBox(this);
    mCSVDecSepCombo->addItem(", (comma)", QVariant(","));
    mCSVDecSepCombo->addItem(". (dot)", QVariant("."));

    if (QLocale::system().language()==QLocale::French) {
        mCSVCellSepEdit->setText(";");
        mCSVDecSepCombo->setCurrentIndex(0);

    } else {
        mCSVCellSepEdit->setText(",");
        mCSVDecSepCombo->setCurrentIndex(1);
    }

    mOpenLastProjectLab = new QLabel(tr("Open Last Project at Launch"), this);
    mOpenLastProjectCheck = new QCheckBox(this);

    mPixelRatioLab = new QLabel(tr("Images Export Pixel Ratio"), this);
    mPixelRatio = new QSpinBox(this);
    mPixelRatio->setRange(1, 5);
    mPixelRatio->setSingleStep(1);

    // Not use because modify the size of the text in the scene, disable since version: v 1.6.4_alpha
    /*
    mDpmLab = new QLabel(tr("Image Export DPM"), this);
    mDpm = new QComboBox(this);
    mDpm->addItems(QStringList() << "72" << "96" << "100" << "150" << "200" << "300");
    */

    //Specify 0 to obtain small compressed files, 100 for large uncompressed files
    mImageQualityLab = new QLabel(tr("Image Export Quality (0 to 100)"), this);
    mImageQuality = new QSpinBox(this);
    mImageQuality->setRange(1, 100);
    mImageQuality->setSingleStep(1);
    mImageQuality->setToolTip(tr("0 for small compressed files, 100 for large uncompressed files"));

    mFormatDateLab = new QLabel(tr("Time Scale"), this);
    mFormatDateLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mFormatDate = new QComboBox(this);
    for (int i=0; i<6; ++i) // until 8 to use Age Ma and ka
        mFormatDate->addItem(DateUtils::dateFormatToString(DateUtils::FormatDate (i)));

    mFormatDate->setCurrentIndex(1);
    mFormatDate->setVisible(true);

    mPrecisionLab = new QLabel(tr("Decimal Precision"), this);
    mPrecision = new QSpinBox(this);
    mPrecision->setRange(0, 6);
    mPrecision->setSingleStep(1);


    connect(mAutoSaveCheck, &QCheckBox::toggled, mAutoSaveDelayEdit, &QLineEdit::setEnabled);

    mRestoreBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults);
    connect(mRestoreBox, &QDialogButtonBox::clicked, this, &AppSettingsDialog::buttonClicked);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    int row = -1;
    grid->addWidget(mLanguageLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLanguageCombo, row, 1);

    grid->addWidget(mLangHelpLab, ++row, 0, 1, 2);

    grid->addWidget(mIconSizeLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mIconSize, row, 1);

    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    grid->addWidget(line1, ++row, 0, 1, 2);

    grid->addWidget(mAutoSaveLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAutoSaveCheck, row, 1);
    grid->addWidget(mAutoSaveDelayLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAutoSaveDelayEdit, row, 1);
    grid->addWidget(mOpenLastProjectLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mOpenLastProjectCheck, row, 1);

    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    grid->addWidget(line2, ++row, 0, 1, 2);

    grid->addWidget(mCSVCellSepLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCSVCellSepEdit, row, 1);
    grid->addWidget(mCSVDecSepLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCSVDecSepCombo, row, 1);

    QFrame* line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);
    grid->addWidget(line3, ++row, 0, 1, 2);

    grid->addWidget(mPixelRatioLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mPixelRatio, row, 1);

    grid->addWidget(mImageQualityLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mImageQuality, row, 1);

    QFrame* line4 = new QFrame();
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);
    grid->addWidget(line4, ++row, 0, 1, 2);

    grid->addWidget(mFormatDateLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mFormatDate, row, 1);
    grid->addWidget(mPrecisionLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mPrecision, row, 1);

    QVBoxLayout* mainLayout = new QVBoxLayout();

    mainLayout->addWidget(mRestoreBox);
    mainLayout->addLayout(grid);
    

    mGeneralView->setLayout(mainLayout);

    connect(mLanguageCombo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppSettingsDialog::changeSettings);
    //connect(mCountryCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSettings()));
    //connect(mFontBut, &Button::clicked, this, &AppSettingsDialog::fontButtonClicked);
    connect(mIconSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);

    connect(mAutoSaveCheck, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this,  &AppSettingsDialog::changeSettings);
    connect(mAutoSaveDelayEdit, &QLineEdit::editingFinished, this,  &AppSettingsDialog::changeSettings);
    connect(mOpenLastProjectCheck, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this,  &AppSettingsDialog::changeSettings);

    connect(mCSVCellSepEdit, &QLineEdit::editingFinished, this,  &AppSettingsDialog::changeSettings);
    connect(mCSVDecSepCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppSettingsDialog::changeSettings);

    connect(mImageQuality, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);
    connect(mPixelRatio, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);

    connect(mFormatDate, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppSettingsDialog::changeSettings);
    connect(mPrecision, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);

    // -----------------------------
    //  List & Stack
    // -----------------------------
    mList = new QListWidget();
    AppSettingsDialogItemDelegate* delegate = new AppSettingsDialogItemDelegate();
    mList->setItemDelegate(delegate);
    mList->setFixedWidth(180);

    mStack = new QStackedWidget();

    // General settings
    QListWidgetItem* item = new QListWidgetItem();
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
    item->setText(tr("General"));
    item->setData(0x0101, tr("General"));
    mList->addItem(item);
    mStack->addWidget(mGeneralView);

    // Plugins specific settings
    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    for (auto && plug : plugins) {
        PluginSettingsViewAbstract* view = plug->getSettingsView();
        if (view){
            QListWidgetItem* item = new QListWidgetItem();
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
            item->setText(plug->getName());
            item->setData(0x0101, plug->getName());
            item->setData(0x0102, plug->getId());
            mList->addItem(item);
            mStack->addWidget(view);
            connect(view, &PluginSettingsViewAbstract::calibrationNeeded, this, &AppSettingsDialog::needCalibration);
        }
    }

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(mList);
    layout->addWidget(mStack);
    setLayout(layout);

    connect(mList, &QListWidget::currentRowChanged, mStack, &QStackedWidget::setCurrentIndex);

    mList->setCurrentRow(0);
    mStack->setCurrentIndex(0);
}

AppSettingsDialog::~AppSettingsDialog()
{
    qDebug()<<"end AppSettingsDialog::~AppSettingsDialog()";
    getSettings();

    if (filesChanged)
        emit settingsFilesChanged();
    else
        emit settingsChanged();

    filesChanged = false;
}

void AppSettingsDialog::needCalibration()
{
    filesChanged = true;
}

void AppSettingsDialog::setSettings()
{
    mLanguageCombo->setCurrentText(QLocale::languageToString(AppSettings::mLanguage));
    //mCountryCombo->setCurrentText(QLocale::countryToString(settings.mCountry)); // keep in memory
    mIconSize->setValue(AppSettings::mIconSize);

    mAutoSaveCheck->setChecked(AppSettings::mAutoSave);
    mAutoSaveDelayEdit->setText(QString::number(AppSettings::mAutoSaveDelay / 60));
    mAutoSaveDelayEdit->setEnabled(AppSettings::mAutoSave);

    mCSVCellSepEdit->setText(AppSettings::mCSVCellSeparator);

    if (AppSettings::mCSVDecSeparator==",")
        mCSVDecSepCombo->setCurrentIndex(0);

    else mCSVDecSepCombo->setCurrentIndex(1);

    mOpenLastProjectCheck->setChecked(AppSettings::mOpenLastProjectAtLaunch);

    mPixelRatio->setValue(AppSettings::mPixelRatio);
    mImageQuality->setValue(AppSettings::mImageQuality);
    mFormatDate->setCurrentIndex(int (AppSettings::mFormatDate));
    mPrecision->setValue(AppSettings::mPrecision);
 }

void AppSettingsDialog::getSettings()
{
    AppSettings::mLanguage = QLocale::Language (mLanguageCombo->currentData().toInt());
    AppSettings::mCountry = locale().country();
    //AppSettings::setFont(mFont);
    AppSettings::mIconSize = mIconSize->value();

    AppSettings::mAutoSave = mAutoSaveCheck->isChecked();
    AppSettings::mAutoSaveDelay = mAutoSaveDelayEdit->text().toInt() * 60;
    AppSettings::mCSVCellSeparator = mCSVCellSepEdit->text();
    AppSettings::mCSVDecSeparator = mCSVDecSepCombo->currentData().toString();
    AppSettings::mOpenLastProjectAtLaunch = mOpenLastProjectCheck->isChecked();
    AppSettings::mPixelRatio = mPixelRatio->value();
    //settings.mDpm = mDpm->currentText().toShort();
    AppSettings::mImageQuality = mImageQuality->value();
    AppSettings::mFormatDate = DateUtils::FormatDate (mFormatDate->currentIndex());
    AppSettings::mPrecision = mPrecision->value();
}

void AppSettingsDialog::changeSettings()
{
    QLocale::Language newLanguage = AppSettings::mLanguage;
    QLocale::Country newCountry= AppSettings::mCountry;

    QLocale newLoc = QLocale(newLanguage, newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);
}

/**
 * @brief AppSettingsDialog::buttonClicked Corresponding to the restore Default Button
 * @param button
 */
void AppSettingsDialog::buttonClicked(QAbstractButton* button)
{
    (void) button;

    mIconSize->setValue(APP_SETTINGS_DEFAULT_ICON_SIZE);

    mLanguageCombo->setCurrentText(QLocale::languageToString(QLocale::system().language()));

    mAutoSaveCheck->setChecked(APP_SETTINGS_DEFAULT_AUTO_SAVE);
    mAutoSaveDelayEdit->setText(locale().toString(APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC / 60));
    mAutoSaveDelayEdit->setEnabled(true);

    if (QLocale::system().decimalPoint()==',') {
        mCSVCellSepEdit->setText(";");
        mCSVDecSepCombo->setCurrentIndex(0);
    } else {
        mCSVCellSepEdit->setText(",");
        mCSVDecSepCombo->setCurrentIndex(1);
    }
    mOpenLastProjectCheck->setChecked(APP_SETTINGS_DEFAULT_OPEN_PROJ);

    mPixelRatio->setValue(APP_SETTINGS_DEFAULT_PIXELRATIO);
    //mDpm->setCurrentText(QString(APP_SETTINGS_DEFAULT_DPM));
    mImageQuality->setValue(APP_SETTINGS_DEFAULT_IMAGE_QUALITY);
    mFormatDate->setCurrentIndex(int (APP_SETTINGS_DEFAULT_FORMATDATE));
    mPrecision->setValue(APP_SETTINGS_DEFAULT_PRECISION);

}
