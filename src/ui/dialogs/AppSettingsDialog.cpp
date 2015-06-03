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
    
    mPixelRatioLab = new Label(tr("Pixel Ratio") + ": ", this);
    mPixelRatio = new QSpinBox(this);
    mPixelRatio->setRange(1, 5);
    mPixelRatio->setSingleStep(1);
    mPixelRatio->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mCSVCellSepLab = new Label(tr("CSV cell separator") + " : ", this);
    mCSVCellSepEdit = new LineEdit(this);
    mCSVCellSepEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    mCSVDecSepLab = new Label(tr("CSV decimal separator") + " : ", this);
    mCSVDecSepEdit = new LineEdit(this);
    
    mOpenLastProjectLab = new Label(tr("Open last project at launch") + " : ", this);
    mOpenLastProjectCheck = new CheckBox(this);
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    mResetBut = new Button(tr("Reset"), this);
    
    mOkBut->setAutoDefault(true);
    
    
    mFormatDateLab = new Label(tr("Date Format") + " :", this);
    mFormatDateLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mFormatDate = new QComboBox(this);
    mFormatDate->addItem("BC/AD");
    mFormatDate->addItem("Cal BP");
    mFormatDate->addItem("Cal B2K");
    mFormatDate->setCurrentText("BC/AC");
    mFormatDate->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mFormatDate->setVisible(true);
    
    
    
    
    connect(mAutoSaveCheck, SIGNAL(toggled(bool)), mAutoSaveDelayEdit, SLOT(setEnabled(bool)));
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mResetBut, SIGNAL(clicked()), this, SLOT(reset()));
    
    setFixedSize(350, 250);
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
    mFormatDate->setCurrentText(settings.mFormatDate);
    g_FormatDate = settings.mFormatDate;
    qDebug()<<"AppSettingsDialog::setSettings gformat"<<g_FormatDate;
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
    g_FormatDate = mFormatDate->currentText();
    settings.mFormatDate = g_FormatDate;
    return settings;
}

void AppSettingsDialog::reset()
{
    mAutoSaveCheck->setChecked(true);
    mAutoSaveDelayEdit->setText("5");
    mAutoSaveDelayEdit->setEnabled(true);
    
    mCSVCellSepEdit->setText(",");
    mCSVDecSepEdit->setText(".");
    
    mOpenLastProjectCheck->setChecked(true);
    
    mPixelRatio->setValue(1);
    g_FormatDate = APP_SETTINGS_DEFAULT_FORMATDATE;
    mFormatDate->setCurrentText(g_FormatDate);
    
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
    
    mFormatDateLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mFormatDate->setGeometry(2*m + w1, y, w2, lineH);
    
    
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

