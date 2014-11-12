#ifndef APPSETTINGSDIALOG_H
#define APPSETTINGSDIALOG_H

#include <QDialog>
#include "AppSettings.h"

class CheckBox;
class QCheckBox;
class Label;
class LineEdit;
class Button;


class AppSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    AppSettingsDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~AppSettingsDialog();

    void setSettings(const AppSettings& settings);
    AppSettings getSettings();
    
private:
    QCheckBox* mAutoSaveCheck;
    Label* mAutoSaveDelayLab;
    LineEdit* mAutoSaveDelayEdit;
    
    Button* mOkBut;
    Button* mCancelBut;
};

#endif
