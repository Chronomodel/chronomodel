#ifndef REBUIDCURVEDIALOG_H
#define REBUIDCURVEDIALOG_H

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>

class RebuidCurveDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RebuidCurveDialog(QWidget *parent = nullptr);

    int getResult() const;
    int getXSpinResult() const;
    int getYSpinResult() const;
    double getYMin() const;
    double getYMax() const;
    bool doCurve() {return curveCB->isChecked();}
    bool doMap() {return mapCB->isChecked();}
    QString compo();
    void setCompo(QStringList &list);

protected slots:
    void updateOptions();
    void YMinIsValid(QString str);
    void YMaxIsValid(QString str);
    void setOkEnabled();

private:
    QLabel *label;
    QLineEdit *lineEdit;
    QCheckBox *curveCB;
    QCheckBox *mapCB;
    QDialogButtonBox *buttonBox;

    QSpinBox *XspinBox;
    QSpinBox *YspinBox;

    QLineEdit *YminEdit;
    bool YMinOK;
    bool YMaxOK;
    QLineEdit *YmaxEdit;
    QComboBox *mCompo;

signals:
    void OkEnabled(bool enabled = true) ;
};

#endif // REBUIDCURVEDIALOG_H
