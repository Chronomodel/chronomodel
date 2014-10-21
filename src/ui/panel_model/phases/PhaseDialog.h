#ifndef PhaseDialog_H
#define PhaseDialog_H

#include <QDialog>

class Phase;
class QLabel;
class LineEdit;
class QComboBox;
class ColorPicker;


class PhaseDialog: public QDialog
{
    Q_OBJECT
public:
    PhaseDialog(Phase* phase, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~PhaseDialog();
    
protected slots:
    void acceptPhase();
    void rejectPhase();
    void showAppropriateTauOptions(int typeIndex);
    
public:
    Phase* mPhase;
    bool mModifying;
    
    LineEdit* mNameEdit;
    ColorPicker* mColorPicker;
    QComboBox* mTauTypeCombo;
    LineEdit* mTauFixedEdit;
    LineEdit* mTauMinEdit;
    LineEdit* mTauMaxEdit;
    
    QLabel* mTauFixedLab;
    QLabel* mTauMinLab;
    QLabel* mTauMaxLab;
};

#endif
