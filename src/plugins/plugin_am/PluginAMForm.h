#ifndef PluginAMForm_H
#define PluginAMForm_H

#if USE_PLUGIN_AM

#include "../PluginFormAbstract.h"

class PluginAM;
class QLineEdit;
class QComboBox;
class QLabel;
class QRadioButton;


class PluginAMForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginAMForm(PluginAM* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginAMForm();
    
    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();
    
    bool isValid();

signals:
    void OkEnabled(bool enabled = true);
     
protected slots:
    void updateOptions();
    void errorIsValid(QString str);

private:
    QRadioButton* mIRadio;
    QRadioButton* mDRadio;
    QRadioButton* mFRadio;
    QRadioButton* mIDRadio;
    QRadioButton* mIFRadio;
    QRadioButton* mIDFRadio;
    
    QLabel* mILab;
    QLabel* mDLab;
    QLabel* mFLab;
    QLabel* mAlpha95Lab;
    QLabel* mSigmaFLab;
    QLabel* mCurveILab;
    QLabel* mCurveDLab;
    QLabel* mCurveFLab;
    
    QLineEdit* mIEdit;
    QLineEdit* mDEdit;
    QLineEdit* mFEdit;
    QLineEdit* mAlpha95Edit;
    QLineEdit* mSigmaFEdit;
    QComboBox* mCurveICombo;
    QComboBox* mCurveDCombo;
    QComboBox* mCurveFCombo;
};

#endif
#endif
