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
class QListWidget;
class QStackedWidget;

class CheckBox;
class Label;
class LineEdit;
class Button;


class AppSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    AppSettingsDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    virtual ~AppSettingsDialog();

    void setSettings(const AppSettings& settings);
    AppSettings getSettings();
        
private slots:
    void changeSettings();
    void buttonClicked(QAbstractButton*);
    
signals:
    void settingsChanged(const AppSettings&);
    
private:
    QListWidget* mList;
    QStackedWidget* mStack;
    
    QWidget* mGeneralView;
    
    QLabel* mLangHelpLab;
    
    QLabel* mLanguageLab;
    QComboBox* mLanguageCombo;

    //Label* mFontLab;
    //Button* mFontBut;
    QFont mFont;
   // QLabel* mCountryLab;
   // QComboBox* mCountryCombo;
    
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
    
    /* disabled since v 1.6.4_alpha
     QLabel* mDpmLab;
     QComboBox* mDpm;
    */
    
    QLabel* mImageQualityLab;
    QSpinBox* mImageQuality;
    
    QLabel* mFormatDateLab;
    QComboBox* mFormatDate;
    
    QLabel* mPrecisionLab;
    QSpinBox* mPrecision;
    
    QDialogButtonBox* mButtonBox;
};

#endif
