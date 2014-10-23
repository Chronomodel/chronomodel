#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "Settings.h"

class CheckBox;
class QCheckBox;
class Label;
class LineEdit;
class Button;


class SettingsDialog: public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~SettingsDialog();

    void setSettings(const Settings& settings);
    Settings getSettings();
    
private:
    QCheckBox* mAutoSaveCheck;
    Label* mAutoSaveDelayLab;
    LineEdit* mAutoSaveDelayEdit;
    
    Button* mOkBut;
    Button* mCancelBut;
};

#endif
