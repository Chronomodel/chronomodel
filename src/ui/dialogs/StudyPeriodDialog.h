#ifndef StudyPeriodDialog_H
#define StudyPeriodDialog_H

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
class QDoubleSpinBox;
class QLabel;
class HelpWidget;


class StudyPeriodDialog: public QDialog
{
    Q_OBJECT
public:
    StudyPeriodDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Window);
    ~StudyPeriodDialog();
    
    void setStep(double step, bool forced, double suggested);
    void setSettings(const ProjectSettings& s);
    ProjectSettings getSettings() const;
    double step() const;
    bool forced() const;
    
protected slots:
    void setAdvancedVisible(bool visible);
    void updateVisibleControls();
    void setOkEnabled(const QString& text);

signals:
    void wiggleChange();
    
public:
    QLabel* mMinLab;
    QLabel* mMaxLab;

    QLineEdit* mMinEdit;
    QLineEdit* mMaxEdit;

    Collapsible* mAdvanced;
    
    QCheckBox* mAdvancedCheck;
    QGroupBox* mAdvancedWidget;

    QLabel* mForcedLab;
    QCheckBox* mForcedCheck;
    QLabel* mStepLab;

    QDoubleSpinBox* mStepSpin;

    int mMargin;
    int mLineH;
    int mButW;
    int mButH;
    int mComboH;
    
    QVBoxLayout* mLayout;
    QDialogButtonBox* mButtonBox;
};

#endif
