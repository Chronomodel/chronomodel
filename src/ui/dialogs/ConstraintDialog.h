#ifndef ConstraintDialog_H
#define ConstraintDialog_H

#include <QDialog>
#include <QJsonObject>

class Label;
class LineEdit;
class QComboBox;
class Button;


class ConstraintDialog: public QDialog
{
    Q_OBJECT
public:
    enum Type{
        eEvent = 0,
        ePhase = 1
    };
    
    ConstraintDialog(QWidget* parent = 0, Type type = eEvent, Qt::WindowFlags flags = 0);
    ~ConstraintDialog();
    
    void setConstraint(const QJsonObject& constraint);
    QJsonObject constraint() const;
    bool deleteRequested() const;
    
protected slots:
    void deleteConstraint();
    void showAppropriateOptions();
    
protected:
    void resizeEvent(QResizeEvent* e);
    
private:
    QJsonObject mConstraint;
    Type mType;
    bool mDeleteRequested;
    
    Label* mTypeLab;
    Label* mFixedLab;
    //Label* mMinLab;
    //Label* mMaxLab;
    
    QComboBox* mTypeCombo;
    LineEdit* mFixedEdit;
    //LineEdit* mMinEdit;
    //LineEdit* mMaxEdit;
    
    Button* mOkBut;
    Button* mCancelBut;
    Button* mDeleteBut;
    
    qreal mComboH;
};

#endif
