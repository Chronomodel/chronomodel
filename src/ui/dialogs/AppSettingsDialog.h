#ifndef APPSETTINGSDIALOG_H
#define APPSETTINGSDIALOG_H

#include <QDialog>
#include "AppSettings.h"


class CheckBox;
class Label;
class LineEdit;
class Button;
class QSpinBox;
class QComboBox;

class AppSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    AppSettingsDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~AppSettingsDialog();

    void setSettings(const AppSettings& settings);
    AppSettings getSettings();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
private slots:
    void reset();
    
private:
    Label* mTitleLab;
    
    Label* mAutoSaveLab;
    CheckBox* mAutoSaveCheck;
    Label* mAutoSaveDelayLab;
    LineEdit* mAutoSaveDelayEdit;
    
    Label* mCSVCellSepLab;
    LineEdit* mCSVCellSepEdit;
    
    Label* mCSVDecSepLab;
    LineEdit* mCSVDecSepEdit;
    
    Label* mOpenLastProjectLab;
    CheckBox* mOpenLastProjectCheck;
    
    Label* mPixelRatioLab;
    QSpinBox* mPixelRatio;
    
    Label* mDpmLab;
    QComboBox* mDpm;
    
    Label* mImageQualityLab;
    QSpinBox* mImageQuality;
    
    Label* mFormatDateLab;
    QComboBox* mFormatDate;
    
    Label* mPrecisionLab;
    QSpinBox* mPrecision;
    
    Button* mResetBut;
    Button* mOkBut;
    Button* mCancelBut;
};

#endif
