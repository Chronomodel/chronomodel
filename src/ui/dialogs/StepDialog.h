#ifndef STEPDIALOG_H
#define STEPDIALOG_H

#include <QDialog>

class CheckBox;
class Label;
//class QSpinBox;
class QDoubleSpinBox;
class Button;


class StepDialog: public QDialog
{
    Q_OBJECT
public:
    StepDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~StepDialog();

    void setStep(double step, bool forced, double suggested);
    double step() const;
    bool forced() const;
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
private:
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
