#include "AppSettingsDialog.h"
#include "Button.h"
#include "LineEdit.h"
#include "Label.h"
#include "CheckBox.h"
#include "Painting.h"
#include <QtWidgets>

#include "AppSettings.h"

AppSettingsDialog::AppSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("Settings"));
    
    mTitleLab = new Label(tr("Application Settings"), this);
    mTitleLab->setIsTitle(true);
    
    mAutoSaveLab = new Label(tr("Auto save project") + " : ", this);
    mAutoSaveCheck = new CheckBox(this);
    mAutoSaveDelayLab = new Label(tr("Auto save interval (in minutes)") + " : ", this);
    mAutoSaveDelayEdit = new LineEdit(this);
    
    QIntValidator* positiveValidator = new QIntValidator();
    positiveValidator->setBottom(1);
    mAutoSaveDelayEdit->setValidator(positiveValidator);
    
    mCSVCellSepLab = new Label(tr("CSV cell separator") + " : ", this);
    mCSVCellSepEdit = new LineEdit(this);
    mCSVCellSepEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mCSVDecSepLab = new Label(tr("CSV decimal separator") + " : ", this);
    mCSVDecSepEdit = new LineEdit(this);
    
    mOpenLastProjectLab = new Label(tr("Open last project at launch") + " : ", this);
    mOpenLastProjectCheck = new CheckBox(this);
    
    mPixelRatioLab = new Label(tr("Pixel Ratio") + ": ", this);
    mPixelRatio = new QSpinBox(this);
    mPixelRatio->setRange(1, 5);
    mPixelRatio->setSingleStep(1);
    
    mDpmLab = new Label(tr("Images export DPM") + ": ", this);
    mDpm = new QComboBox(this);
    mDpm->addItems(QStringList() << "72" << "96" << "100" << "150" << "200" << "300");
    
    mImageQualityLab = new Label(tr("Image export quality (0 to 100)") + ": ", this);
    mImageQuality = new QSpinBox(this);
    mImageQuality->setRange(1, 100);
    mImageQuality->setSingleStep(1);
    
    mFormatDateLab = new Label(tr("Date Format") + " :", this);
    mFormatDateLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mFormatDate = new QComboBox(this);
    for(int i=0; i<3; ++i){
        mFormatDate->addItem(DateUtils::formatString((DateUtils::FormatDate)i));
    }
    mFormatDate->setCurrentIndex(0);
    mFormatDate->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mFormatDate->setVisible(true);
    
    mPrecisionLab = new Label(tr("Date Precision") + " :", this);
    mPrecision = new QSpinBox(this);
    mPrecision->setRange(1, 5);
    mPrecision->setSingleStep(1);
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    mResetBut = new Button(tr("Reset"), this);
    
    mOkBut->setAutoDefault(true);
    
    
    
    
    connect(mAutoSaveCheck, SIGNAL(toggled(bool)), mAutoSaveDelayEdit, SLOT(setEnabled(bool)));
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mResetBut, SIGNAL(clicked()), this, SLOT(reset()));
    
    setFixedSize(350, 325);
}

AppSettingsDialog::~AppSettingsDialog()
{

}

void AppSettingsDialog::setSettings(const AppSettings& settings)
{
    mAutoSaveCheck->setChecked(settings.mAutoSave);
    mAutoSaveDelayEdit->setText(QString::number(settings.mAutoSaveDelay / 60));
    mAutoSaveDelayEdit->setEnabled(settings.mAutoSave);
    
    mCSVCellSepEdit->setText(settings.mCSVCellSeparator);
    mCSVDecSepEdit->setText(settings.mCSVDecSeparator);
    
    mOpenLastProjectCheck->setChecked(settings.mOpenLastProjectAtLaunch);
    
    mPixelRatio->setValue(settings.mPixelRatio);
    mDpm->setCurrentText(QString(settings.mDpm));
    mImageQuality->setValue(settings.mImageQuality);
    mFormatDate->setCurrentIndex((int)settings.mFormatDate);
    mPrecision->setValue(settings.mPrecision);
}

AppSettings AppSettingsDialog::getSettings()
{
    AppSettings settings;
    settings.mAutoSave = mAutoSaveCheck->isChecked();
    settings.mAutoSaveDelay = mAutoSaveDelayEdit->text().toInt() * 60;
    settings.mCSVCellSeparator = mCSVCellSepEdit->text();
    settings.mCSVDecSeparator = mCSVDecSepEdit->text();
    settings.mOpenLastProjectAtLaunch = mOpenLastProjectCheck->isChecked();
    settings.mPixelRatio = mPixelRatio->value();
    settings.mDpm = mDpm->currentText().toShort();
    settings.mImageQuality = mImageQuality->value();
    settings.mFormatDate = (DateUtils::FormatDate)mFormatDate->currentIndex();
    settings.mPrecision = mPrecision->value();
    return settings;
}

void AppSettingsDialog::reset()
{
    mAutoSaveCheck->setChecked(APP_SETTINGS_DEFAULT_AUTO_SAVE);
    mAutoSaveDelayEdit->setText(QString(APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC / 60));
    mAutoSaveDelayEdit->setEnabled(true);
    
    mCSVCellSepEdit->setText(APP_SETTINGS_DEFAULT_CELL_SEP);
    mCSVDecSepEdit->setText(APP_SETTINGS_DEFAULT_DEC_SEP);
    
    mOpenLastProjectCheck->setChecked(APP_SETTINGS_DEFAULT_OPEN_PROJ);
    
    mPixelRatio->setValue(APP_SETTINGS_DEFAULT_PIXELRATIO);
    mDpm->setCurrentText(QString(APP_SETTINGS_DEFAULT_DPM));
    mImageQuality->setValue(APP_SETTINGS_DEFAULT_IMAGE_QUALITY);
    mFormatDate->setCurrentIndex((int)APP_SETTINGS_DEFAULT_FORMATDATE);
    mPrecision->setValue(APP_SETTINGS_DEFAULT_PRECISION);
}

void AppSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    int lineH = 20;
    int w1 = 200;
    int w2 = width() - 3*m - w1;
    int butW = 80;
    int butH = 25;
    
    int y = -lineH;
    
    mTitleLab->setGeometry(m, y += (lineH + m), width() - 2*m, lineH);
    
    mAutoSaveLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mAutoSaveCheck->setGeometry(2*m + w1, y, w2, lineH);
    
    mAutoSaveDelayLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mAutoSaveDelayEdit->setGeometry(2*m + w1, y, w2, lineH);
    
    mCSVCellSepLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mCSVCellSepEdit->setGeometry(2*m + w1, y, w2, lineH);
    
    mCSVDecSepLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mCSVDecSepEdit->setGeometry(2*m + w1, y, w2, lineH);
    
    mOpenLastProjectLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mOpenLastProjectCheck->setGeometry(2*m + w1, y, w2, lineH);
    
    mPixelRatioLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mPixelRatio->setGeometry(2*m + w1, y, w2, lineH);
    
    mDpmLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mDpm->setGeometry(2*m + w1, y, w2, lineH);
    
    mImageQualityLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mImageQuality->setGeometry(2*m + w1, y, w2, lineH);
    
    mFormatDateLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mFormatDate->setGeometry(2*m + w1, y, w2, lineH);
    
    mPrecisionLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mPrecision->setGeometry(2*m + w1, y, w2, lineH);
    
    
    mResetBut->setGeometry(width() - 3*m - 3*butW, height() - m - butH, butW, butH);
    mOkBut->setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

void AppSettingsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(180, 180, 180));
}

