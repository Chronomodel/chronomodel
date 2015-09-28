#ifndef DateDialog_H
#define DateDialog_H

#include <QDialog>
#include <QJsonObject>
#include "Date.h"

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class QGroupBox;
class QCheckBox;
class QDialogButtonBox;

class PluginFormAbstract;
class Collapsible;
class QRadioButton;
class Button;
class QLabel;
class HelpWidget;


class DateDialog: public QDialog
{
    Q_OBJECT
public:
    DateDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~DateDialog();
    
    void setDate(const QJsonObject& date);
    void setForm(PluginFormAbstract* form);
    void setDataMethod(Date::DataMethod method);
    
    QString getName() const;
    Date::DataMethod getMethod() const;
    Date::DeltaType getDeltaType() const;
    double getDeltaFixed() const;
    double getDeltaMin() const;
    double getDeltaMax() const;
    double getDeltaAverage() const;
    double getDeltaError() const;
    
    void setWiggleEnabled(bool enabled);
    
protected:
    //void paintEvent(QPaintEvent* e);
    //void resizeEvent(QResizeEvent* e);
    
protected slots:
    void setAdvancedVisible(bool visible);
    void updateVisibleControls();
    //void updateLayout();
    //void adaptSize();
    
public:
    PluginFormAbstract* mForm;
    
    QLabel* mNameLab;
    QLineEdit* mNameEdit;
    
    Collapsible* mAdvanced;
    
    QCheckBox* mAdvancedCheck;
    QGroupBox* mAdvancedWidget;
    
    QLabel* mMethodLab;
    QLabel* mWiggleLab;
    QComboBox* mMethodCombo;
    
    QRadioButton* mDeltaFixedRadio;
    QRadioButton* mDeltaRangeRadio;
    QRadioButton* mDeltaGaussRadio;
    
    HelpWidget* mDeltaHelp;
    QLabel* mDeltaFixedLab;
    QLabel* mDeltaMinLab;
    QLabel* mDeltaMaxLab;
    QLabel* mDeltaAverageLab;
    QLabel* mDeltaErrorLab;
    
    QLineEdit* mDeltaFixedEdit;
    QLineEdit* mDeltaMinEdit;
    QLineEdit* mDeltaMaxEdit;
    QLineEdit* mDeltaAverageEdit;
    QLineEdit* mDeltaErrorEdit;
    
    int mWidth;
    int mMargin;
    int mLineH;
    int mButW;
    int mButH;
    int mComboH;
    
    bool mWiggleEnabled;
    
    QVBoxLayout* mLayout;
    QDialogButtonBox* mButtonBox;
};

#endif
