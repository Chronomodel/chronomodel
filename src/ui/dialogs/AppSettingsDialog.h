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
    AppSettingsDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~AppSettingsDialog();

    void setSettings(const AppSettings& settings);
    AppSettings getSettings();
    
protected:
    //void paintEvent(QPaintEvent* e);
    //void resizeEvent(QResizeEvent* e);
    
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
    
    QLabel* mDpmLab;
    QComboBox* mDpm;
    
    QLabel* mImageQualityLab;
    QSpinBox* mImageQuality;
    
    QLabel* mFormatDateLab;
    QComboBox* mFormatDate;
    
    QLabel* mPrecisionLab;
    QSpinBox* mPrecision;
    
    QLabel* mSheetLab;
    QSpinBox* mNbSheet;
    
    QDialogButtonBox* mButtonBox;
};

#endif
