#ifndef STUDYPERIODDIALOG_H
#define STUDYPERIODDIALOG_H

#include <QDialog>

class CheckBox;
class Label;
class LineEdit;
class QDoubleSpinBox;
class Button;
class ProjectSettings;


class StudyPeriodDialog: public QDialog
{
    Q_OBJECT
public:
    StudyPeriodDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    virtual ~StudyPeriodDialog();

    void setStep(double step, bool forced, double suggested);
    void setSettings(const ProjectSettings& s);
    ProjectSettings getSettings() const;
    double step() const;
    bool forced() const;
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
private:

    Label* mStudyLab;
    Label* mMinLab;
    Label* mMaxLab;

    LineEdit* mMinEdit;
    LineEdit* mMaxEdit;

    Label* mTitleLab;
    Label* mForcedLab;
    CheckBox* mForcedCheck;
    Label* mStepLab;
    //QSpinBox* mStepSpin;
    QDoubleSpinBox* mStepSpin;
    Button* mOkBut;
    Button* mCancelBut;
};

#endif
