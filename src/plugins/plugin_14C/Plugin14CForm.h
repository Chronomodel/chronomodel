#ifndef Plugin14CForm_H
#define Plugin14CForm_H

#if USE_PLUGIN_14C

#include "../PluginFormAbstract.h"

class Plugin14C;
class LineEdit;
class QComboBox;
class Label;


class Plugin14CForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    Plugin14CForm(Plugin14C* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~Plugin14CForm();
    
    void setData(const QJsonObject& data);
    QJsonObject getData();

protected:
    void resizeEvent(QResizeEvent* e);
    
private:
    Label* mAverageLab;
    Label* mErrorLab;
    Label* mRefLab;
    Label* mRefPathLab;
    
    LineEdit* mAverageEdit;
    LineEdit* mErrorEdit;
    QComboBox* mRefCombo;
    
    static QString mSelectedRefCurve;
    
    int mComboH;
};

#endif

#endif
