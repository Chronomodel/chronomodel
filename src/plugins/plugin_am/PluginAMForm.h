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
    QRadioButton* mIncRadio;
    QRadioButton* mDecRadio;
    QRadioButton* mIntensityRadio;
    
    QLabel* mIncLab;
    QLabel* mDecLab;
    QLabel* mDecIncLab;
    QLabel* mIntensityLab;
    QLabel* mAlpha95Lab;
    QLabel* mRefLab;
    
    QLineEdit* mIncEdit;
    QLineEdit* mDecEdit;
    QLineEdit* mDecIncEdit;
    QLineEdit* mIntensityEdit;
    QLineEdit* mAlpha95Edit;
    
    QComboBox* mRefCombo;
};

#endif
#endif
