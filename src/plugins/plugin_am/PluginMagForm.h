#ifndef PluginMagForm_H
#define PluginMagForm_H

#if USE_PLUGIN_AM

#include "../PluginFormAbstract.h"

class PluginMag;
class LineEdit;
class QComboBox;
class Label;
class RadioButton;


class PluginMagForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginMagForm(PluginMag* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginMagForm();
    
    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();
    
    bool isValid();
    
protected:
    void resizeEvent(QResizeEvent* e);
    
private slots:
    void updateOptions();

private:
    RadioButton* mIncRadio;
    RadioButton* mDecRadio;
    RadioButton* mIntensityRadio;
    
    Label* mIncLab;
    Label* mDecLab;
    Label* mDecIncLab;
    Label* mIntensityLab;
    Label* mAlpha95Lab;
    Label* mRefLab;
    Label* mRefPathLab;
    
    LineEdit* mIncEdit;
    LineEdit* mDecEdit;
    LineEdit* mDecIncEdit;
    LineEdit* mIntensityEdit;
    LineEdit* mAlpha95Edit;
    
    QComboBox* mRefCombo;
    
    int mComboH;
};

#endif
#endif
