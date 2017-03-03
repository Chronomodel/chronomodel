#include "AppSettingsDialog.h"
#include "AppSettingsDialogItemDelegate.h"
#include "PluginSettingsViewAbstract.h"
#include "Painting.h"
#include "Label.h"
#include "Button.h"
#include <QtWidgets>

#include "AppSettings.h"

AppSettingsDialog::AppSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setFont(qApp->font());

    setWindowTitle(tr("Application Settings"));

    // -----------------------------
    //  General View
    // -----------------------------
    mFont = qApp->font();

    mGeneralView = new QWidget();
    
    mLangHelpLab = new QLabel(tr("Language is used to define how number input should be typed (using comma or dot as decimal separator). This is not related to the application translation which is not available yet!"), this);

    mLangHelpLab->setFont(mFont);
    mLangHelpLab->setAlignment(Qt::AlignCenter);
    mLangHelpLab->setWordWrap(true);
    
    mLanguageLab = new QLabel(tr("Language"), this);
    mLanguageCombo = new QComboBox(this);
//    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);

    for (int i=0; i<339; i++)
        mLanguageCombo->addItem(QLocale::languageToString((QLocale::Language)i),QVariant((QLocale::Language)i));


    mFontLab = new Label(tr("Font"), this);

    mFontBut = new Button(this);
    mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));
    mFontBut->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    /*
    mCountryLab = new QLabel(tr("Country") + " : ", this);
    mCountryCombo = new QComboBox(this);
    mCountryCombo->addItem(QLocale::countryToString(QLocale::France), QVariant(QLocale::France));
    mCountryCombo->addItem(QLocale::countryToString(QLocale::UnitedKingdom), QVariant(QLocale::UnitedKingdom));
    */

    mAutoSaveLab = new QLabel(tr("Auto save project"), this);
    mAutoSaveCheck = new QCheckBox(this);
    mAutoSaveDelayLab = new QLabel(tr("Auto save interval (in minutes)"), this);
    mAutoSaveDelayEdit = new QLineEdit(this);
    mAutoSaveDelayEdit->setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    QIntValidator* positiveValidator = new QIntValidator();
    positiveValidator->setBottom(1);
    mAutoSaveDelayEdit->setValidator(positiveValidator);
    
    mCSVCellSepLab = new QLabel(tr("CSV cell separator"), this);
    mCSVCellSepEdit = new QLineEdit(this);
    mCSVCellSepEdit->setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mCSVDecSepLab = new QLabel(tr("CSV decimal separator"), this);
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


    mOpenLastProjectLab = new QLabel(tr("Open last project at launch"), this);
    mOpenLastProjectCheck = new QCheckBox(this);
    
    mPixelRatioLab = new QLabel(tr("Images export pixel ratio"), this);
    mPixelRatio = new QSpinBox(this);
    mPixelRatio->setRange(1, 5);
    mPixelRatio->setSingleStep(1);
    mPixelRatio->setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mDpmLab = new QLabel(tr("Image export DPM"), this);
    mDpm = new QComboBox(this);
    mDpm->addItems(QStringList() << "72" << "96" << "100" << "150" << "200" << "300");
    
    mImageQualityLab = new QLabel(tr("Image export quality (0 to 100)"), this);
    mImageQuality = new QSpinBox(this);
    mImageQuality->setRange(1, 100);
    mImageQuality->setSingleStep(1);
    mImageQuality->setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mFormatDateLab = new QLabel(tr("Date format"), this);
    mFormatDateLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mFormatDate = new QComboBox(this);
    for (int i=0; i<5; ++i)
        mFormatDate->addItem(DateUtils::formatString((DateUtils::FormatDate)i));
    
    mFormatDate->setCurrentIndex(1);
    mFormatDate->setStyleSheet("QLineEdit { border-radius: 5px; }");
    mFormatDate->setVisible(true);
    
    mPrecisionLab = new QLabel(tr("Graph display date precision"), this);
    mPrecision = new QSpinBox(this);
    mPrecision->setRange(0, 5);
    mPrecision->setSingleStep(1);
    mPrecision->setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    
    connect(mAutoSaveCheck, &QCheckBox::toggled, mAutoSaveDelayEdit, &QLineEdit::setEnabled);
    
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults);// QDialogButtonBox::Reset);
    connect(mButtonBox, &QDialogButtonBox::clicked, this, &AppSettingsDialog::buttonClicked);
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    int row = -1;
    grid->addWidget(mLanguageLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLanguageCombo, row, 1);

    grid->addWidget(mFontLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mFontBut, ++row, 0, 1, 2);
    grid->addWidget(mLangHelpLab, ++row, 0, 1, 2);

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
    grid->addWidget(mDpmLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mDpm, row, 1);
    grid->addWidget(mImageQualityLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mImageQuality, row, 1);
    
    QFrame* line4 = new QFrame();
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);
    grid->addWidget(line4, ++row, 0, 1, 2);

    mSheetLab = new QLabel(tr("Number of max graph per sheet"), this);
    mNbSheet = new QSpinBox(this);
    mNbSheet->setRange(5, 100);
    mNbSheet->setSingleStep(1);
    mNbSheet->setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    grid->addWidget(mFormatDateLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mFormatDate, row, 1);
    grid->addWidget(mPrecisionLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mPrecision, row, 1);
    grid->addWidget(mSheetLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mNbSheet, row, 1);
    
    QVBoxLayout* mainLayout = new QVBoxLayout();

    mainLayout->addWidget(mButtonBox);
    mainLayout->addLayout(grid);
    mainLayout->addSpacing(20);

    mGeneralView->setLayout(mainLayout);
    
    connect(mLanguageCombo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppSettingsDialog::changeSettings);
    //connect(mCountryCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSettings()));
    connect(mFontBut, &Button::clicked, this, &AppSettingsDialog::fontButtonClicked);

    connect(mAutoSaveCheck, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this,  &AppSettingsDialog::changeSettings);
    connect(mAutoSaveDelayEdit, &QLineEdit::editingFinished, this,  &AppSettingsDialog::changeSettings);
    connect(mOpenLastProjectCheck, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this,  &AppSettingsDialog::changeSettings);

    connect(mCSVCellSepEdit, &QLineEdit::editingFinished, this,  &AppSettingsDialog::changeSettings);
    connect(mCSVDecSepCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppSettingsDialog::changeSettings);

    connect(mImageQuality, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);
    connect(mPixelRatio, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);
    connect(mDpm,  static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppSettingsDialog::changeSettings);
    connect(mFormatDate, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppSettingsDialog::changeSettings);
    connect(mPrecision, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);
    connect(mNbSheet, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppSettingsDialog::changeSettings);
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
    
    setFixedWidth(700);
}

AppSettingsDialog::~AppSettingsDialog()
{
    qDebug()<<"fin AppSettingsDialog::~AppSettingsDialog()";
    AppSettings s = getSettings();
    emit settingsChanged(s);
}

void AppSettingsDialog::setSettings(const AppSettings& settings)
{
    mLanguageCombo->setCurrentText(QLocale::languageToString(settings.mLanguage));
    //mCountryCombo->setCurrentText(QLocale::countryToString(settings.mCountry)); // keep in memory
    mFont=settings.mFont;
    mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));

    mAutoSaveCheck->setChecked(settings.mAutoSave);
    mAutoSaveDelayEdit->setText(QString::number(settings.mAutoSaveDelay / 60));
    mAutoSaveDelayEdit->setEnabled(settings.mAutoSave);
    
    mCSVCellSepEdit->setText(settings.mCSVCellSeparator);

    if (settings.mCSVDecSeparator==",")
        mCSVDecSepCombo->setCurrentIndex(0);
    
    else mCSVDecSepCombo->setCurrentIndex(1);

    mOpenLastProjectCheck->setChecked(settings.mOpenLastProjectAtLaunch);
    
    mPixelRatio->setValue(settings.mPixelRatio);
    mDpm->setCurrentText(QString::number(settings.mDpm));
    mImageQuality->setValue(settings.mImageQuality);
    mFormatDate->setCurrentIndex((int)settings.mFormatDate);
    mPrecision->setValue(settings.mPrecision);
    mNbSheet->setValue(settings.mNbSheet);
}

AppSettings AppSettingsDialog::getSettings()
{
    AppSettings settings;
    settings.mLanguage = (QLocale::Language)mLanguageCombo->currentData().toInt();
    settings.mCountry = locale().country();
    settings.mFont = mFont;

    settings.mAutoSave = mAutoSaveCheck->isChecked();
    settings.mAutoSaveDelay = mAutoSaveDelayEdit->text().toInt() * 60;
    settings.mCSVCellSeparator = mCSVCellSepEdit->text();
    settings.mCSVDecSeparator = mCSVDecSepCombo->currentData().toString();
    settings.mOpenLastProjectAtLaunch = mOpenLastProjectCheck->isChecked();
    settings.mPixelRatio = mPixelRatio->value();
    settings.mDpm = mDpm->currentText().toShort();
    settings.mImageQuality = mImageQuality->value();
    settings.mFormatDate = (DateUtils::FormatDate)mFormatDate->currentIndex();
    settings.mPrecision = mPrecision->value();    
    settings.mNbSheet = mNbSheet->value();
    return settings;
}

void AppSettingsDialog::changeSettings()
{
    AppSettings s = getSettings();
    
    qApp->setFont(mFont);
    mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));

    QLocale::Language newLanguage = s.mLanguage;
    QLocale::Country newCountry= s.mCountry;
    
    QLocale newLoc = QLocale(newLanguage, newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);
    
   // emit settingsChanged(s);
}

void AppSettingsDialog::fontButtonClicked()
{
    QFontDialog dialog;
    dialog.setParent(qApp->activeWindow());
    dialog.setFont(mFont);

    bool ok;
    const QFont font = QFontDialog::getFont(&ok, mFont, this);
    if (ok) {
        mFont = font;
        mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));
       // qApp->setFont(mFont);
   }
}

/**
 * @brief AppSettingsDialog::buttonClicked Corresponding to the restore Default Button
 * @param button
 */
void AppSettingsDialog::buttonClicked(QAbstractButton* button)
{
    mFont = QFont(APP_SETTINGS_DEFAULT_FONT_FAMILY, APP_SETTINGS_DEFAULT_FONT_SIZE);
    mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));

    mLanguageCombo->setCurrentText(QLocale::languageToString(QLocale::system().language()));
    //mCountryCombo->setCurrentText(QLocale::countryToString(QLocale::system().country()));

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
    mDpm->setCurrentText(QString(APP_SETTINGS_DEFAULT_DPM));
    mImageQuality->setValue(APP_SETTINGS_DEFAULT_IMAGE_QUALITY);
    mFormatDate->setCurrentIndex((int)APP_SETTINGS_DEFAULT_FORMATDATE);
    mPrecision->setValue(APP_SETTINGS_DEFAULT_PRECISION);
    mNbSheet->setValue(APP_SETTINGS_DEFAULT_SHEET);

    AppSettings s = getSettings();
    emit settingsChanged(s);

}
