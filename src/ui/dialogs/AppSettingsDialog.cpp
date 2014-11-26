#include "AppSettingsDialog.h"
#include "Button.h"
#include "LineEdit.h"
#include "Label.h"
#include "CheckBox.h"
#include "Painting.h"
#include <QtWidgets>


AppSettingsDialog::AppSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("Settings"));
    
    mAutoSaveCheck = new CheckBox(tr("Auto save project"), this);
    mAutoSaveDelayLab = new Label(tr("Auto save interval (in minutes)"), this);
    mAutoSaveDelayEdit = new LineEdit(this);
    
    mAutoSaveDelayLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    QIntValidator* positiveValidator = new QIntValidator();
    positiveValidator->setBottom(1);
    mAutoSaveDelayEdit->setValidator(positiveValidator);
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    mCSVSepLab = new Label(tr("CSV cell separator") + " : ", this);
    mCSVSepEdit = new LineEdit(this);
    
    mOkBut->setFixedSize(80, 25);
    mCancelBut->setFixedSize(80, 25);
    
    mOkBut->setAutoDefault(true);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    setFixedSize(500, 180);
}

AppSettingsDialog::~AppSettingsDialog()
{

}

void AppSettingsDialog::setSettings(const AppSettings& settings)
{
    mAutoSaveCheck->setChecked(settings.mAutoSave);
    mAutoSaveDelayEdit->setText(QString::number(settings.mAutoSaveDelay / 60));
    mAutoSaveDelayEdit->setEnabled(settings.mAutoSave);
}

AppSettings AppSettingsDialog::getSettings()
{
    AppSettings settings;
    settings.mAutoSave = mAutoSaveCheck->isChecked();
    settings.mAutoSaveDelay = mAutoSaveDelayEdit->text().toInt() * 60;
    return settings;
}

void AppSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    int lineH = 25;
    int wl = 200;
    int wr = width() - 3*m - wl;
    int butW = 100;
    int butH = 30;
    
    mAutoSaveCheck->setGeometry(m, m, width() - 2*m, lineH);
    mAutoSaveDelayLab->setGeometry(m, 2*m + lineH, wl, lineH);
    mAutoSaveDelayEdit->setGeometry(wl + 2*m, 2*m + lineH, wr, lineH);
    
    mCSVSepLab->setGeometry(m, 3*m + 2*lineH, wl, lineH);
    mCSVSepEdit->setGeometry(wl + 2*m, 3*m + 2*lineH, wr, lineH);
    
    mOkBut->setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}
