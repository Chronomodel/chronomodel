#ifndef EventConstraintDialog_H
#define EventConstraintDialog_H

#include <QDialog>
#include "EventConstraint.h"

class QLabel;
class LineEdit;
class QComboBox;
class ColorPicker;


class EventConstraintDialog: public QDialog
{
    Q_OBJECT
public:
    EventConstraintDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~EventConstraintDialog();
    
    void setConstraint(const EventConstraint& constraint);
    const EventConstraint& getConstraint();
    
protected slots:
    void deleteConstraint();
    void showAppropriatePhiOptions();
    
public:
    EventConstraint mEventConstraint;
    bool mDeleteRequested;
    
    QComboBox* mPhiTypeCombo;
    LineEdit* mPhiFixedEdit;
    LineEdit* mPhiMinEdit;
    LineEdit* mPhiMaxEdit;
    
    QLabel* mPhiFixedLab;
    QLabel* mPhiMinLab;
    QLabel* mPhiMaxLab;
};

#endif
