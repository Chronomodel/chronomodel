#ifndef PhaseConstraintDialog_H
#define PhaseConstraintDialog_H

#include <QDialog>
#include "PhaseConstraint.h"

class QLabel;
class LineEdit;
class QComboBox;
class ColorPicker;


class PhaseConstraintDialog: public QDialog
{
    Q_OBJECT
public:
    PhaseConstraintDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~PhaseConstraintDialog();
    
    void setConstraint(const PhaseConstraint& constraint);
    const PhaseConstraint& getConstraint();
    
protected slots:
    void deleteConstraint();
    void showAppropriateGammaOptions();
    
public:
    PhaseConstraint mPhaseConstraint;
    bool mDeleteRequested;
    
    QComboBox* mGammaTypeCombo;
    LineEdit* mGammaFixedEdit;
    LineEdit* mGammaMinEdit;
    LineEdit* mGammaMaxEdit;
    
    QLabel* mGammaFixedLab;
    QLabel* mGammaMinLab;
    QLabel* mGammaMaxLab;
};

#endif
