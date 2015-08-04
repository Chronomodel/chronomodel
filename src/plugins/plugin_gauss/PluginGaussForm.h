#ifndef PluginGaussForm_H
#define PluginGaussForm_H

#if USE_PLUGIN_GAUSS

#include "../PluginFormAbstract.h"

class PluginGauss;
class LineEdit;
class Label;
class RadioButton;
class QComboBox;


class PluginGaussForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginGaussForm(PluginGauss* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginGaussForm();
    
    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();
    
    bool isValid();
    
protected:
    void resizeEvent(QResizeEvent* e);
    
protected slots:
    void updateVisibleElements();

private:
    Label* mAverageLab;
    Label* mErrorLab;
    
    LineEdit* mAverageEdit;
    LineEdit* mErrorEdit;
    
    Label* mEq1Lab;
    Label* mEq2Lab;
    Label* mEq3Lab;
    
    LineEdit* mAEdit;
    LineEdit* mBEdit;
    LineEdit* mCEdit;
    
    RadioButton* mCurveRadio;
    RadioButton* mEquationRadio;
    
    QComboBox* mCurveCombo;
    int mComboH;
};

#endif
#endif
