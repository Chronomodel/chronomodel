#ifndef DateDialog_H
#define DateDialog_H

#include <QDialog>
#include <QJsonObject>
#include "Date.h"

class LineEdit;
class QComboBox;
class QVBoxLayout;

class PluginFormAbstract;
class Collapsible;
class RadioButton;
class Button;
class Label;
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
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
protected slots:
    void updateLayout();
    void adaptSize();
    
public:
    PluginFormAbstract* mForm;
    
    Label* mNameLab;
    LineEdit* mNameEdit;
    
    Collapsible* mAdvanced;
    QWidget* mAdvancedWidget;
    
    Label* mMethodLab;
    QComboBox* mMethodCombo;
    
    RadioButton* mDeltaFixedRadio;
    RadioButton* mDeltaRangeRadio;
    RadioButton* mDeltaGaussRadio;
    
    HelpWidget* mDeltaHelp;
    Label* mDeltaFixedLab;
    Label* mDeltaMinLab;
    Label* mDeltaMaxLab;
    Label* mDeltaAverageLab;
    Label* mDeltaErrorLab;
    
    LineEdit* mDeltaFixedEdit;
    LineEdit* mDeltaMinEdit;
    LineEdit* mDeltaMaxEdit;
    LineEdit* mDeltaAverageEdit;
    LineEdit* mDeltaErrorEdit;
    
    Button* mOkBut;
    Button* mCancelBut;
    
    int mWidth;
    int mMargin;
    int mLineH;
    int mButW;
    int mButH;
    int mComboH;
};

#endif
