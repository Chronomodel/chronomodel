#include "SettingsDialog.h"
#include "Button.h"
#include "LineEdit.h"
#include "Label.h"
#include "CheckBox.h"
#include "Painting.h"
#include <QtWidgets>


SettingsDialog::SettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("Settings"));
    
    mAutoSaveCheck = new QCheckBox(tr("Auto save project every") + " :");
    mAutoSaveDelayLab = new Label(tr("seconds"));
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

SettingsDialog::~SettingsDialog()
{

}

void SettingsDialog::setSettings(const Settings& settings)
{
    mAutoSaveCheck->setChecked(settings.mAutoSave);
    mAutoSaveDelayEdit->setText(QString::number(settings.mAutoSaveDelay));
    mAutoSaveDelayEdit->setEnabled(settings.mAutoSave);
}

Settings SettingsDialog::getSettings()
{
    Settings settings;
    settings.mAutoSave = mAutoSaveCheck->isChecked();
    settings.mAutoSaveDelay = mAutoSaveDelayEdit->text().toInt();
    return settings;
}

