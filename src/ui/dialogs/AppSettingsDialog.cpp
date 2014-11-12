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
    
    mAutoSaveCheck = new QCheckBox(tr("Auto save project every") + " :");
    mAutoSaveDelayLab = new Label(tr("minutes"));
    mAutoSaveDelayEdit = new LineEdit();
    
    mAutoSaveDelayLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    QIntValidator* positiveValidator = new QIntValidator();
    positiveValidator->setBottom(1);
    mAutoSaveDelayEdit->setValidator(positiveValidator);
    
    mOkBut = new Button(tr("OK"));
    mCancelBut = new Button(tr("Cancel"));
    
    mOkBut->setFixedSize(80, 25);
    mCancelBut->setFixedSize(80, 25);
    
    QHBoxLayout* saveLayout = new QHBoxLayout();
    saveLayout->setContentsMargins(0, 0, 0, 0);
    saveLayout->setSpacing(5);
    saveLayout->addWidget(mAutoSaveCheck);
    saveLayout->addWidget(mAutoSaveDelayEdit);
    saveLayout->addWidget(mAutoSaveDelayLab);
    
    QHBoxLayout* butLayout = new QHBoxLayout();
    butLayout->setContentsMargins(0, 0, 0, 0);
    butLayout->setSpacing(5);
    butLayout->addWidget(mOkBut);
    butLayout->addWidget(mCancelBut);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(5, 5, 5, 5);
    layout->addStretch();
    layout->addLayout(saveLayout);
    layout->addLayout(butLayout);
    setLayout(layout);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    setFixedWidth(300);
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

