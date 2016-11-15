#ifndef PluginOptionsDialog_H
#define PluginOptionsDialog_H

#include <QDialog>

class QComboBox;
class Label;
class LineEdit;
class Button;


class PluginOptionsDialog: public QDialog
{
    Q_OBJECT
public:
    PluginOptionsDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginOptionsDialog();

    QString getC14Ref() const;
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
private:
    Label* mTitleLab;

    Label* mC14RefLab;
    QComboBox* mC14RefCombo;
    
    Button* mOkBut;
    Button* mCancelBut;
    
    int mComboH;
};

#endif
