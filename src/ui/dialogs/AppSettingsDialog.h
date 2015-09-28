#ifndef APPSETTINGSDIALOG_H
#define APPSETTINGSDIALOG_H

#include <QDialog>
#include "AppSettings.h"


class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QComboBox;
class QAbstractButton;
class QDialogButtonBox;

class CheckBox;
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
    
protected:
    //void paintEvent(QPaintEvent* e);
    //void resizeEvent(QResizeEvent* e);
    
private slots:
    void buttonClicked(QAbstractButton*);
    
private:
    QLabel* mLangHelpLab;
    
    QLabel* mLanguageLab;
    QComboBox* mLanguageCombo;
    
    QLabel* mCountryLab;
    QComboBox* mCountryCombo;
    
    QLabel* mAutoSaveLab;
    QCheckBox* mAutoSaveCheck;
    QLabel* mAutoSaveDelayLab;
    QLineEdit* mAutoSaveDelayEdit;
    
    QLabel* mCSVCellSepLab;
    QLineEdit* mCSVCellSepEdit;
    
    QLabel* mCSVDecSepLab;
    QComboBox* mCSVDecSepCombo;
    
    QLabel* mOpenLastProjectLab;
    QCheckBox* mOpenLastProjectCheck;
    
    QLabel* mPixelRatioLab;
    QSpinBox* mPixelRatio;
    
    QLabel* mDpmLab;
    QComboBox* mDpm;
    
    QLabel* mImageQualityLab;
    QSpinBox* mImageQuality;
    
    QLabel* mFormatDateLab;
    QComboBox* mFormatDate;
    
    QLabel* mPrecisionLab;
    QSpinBox* mPrecision;
    
    QDialogButtonBox* mButtonBox;
};

#endif
