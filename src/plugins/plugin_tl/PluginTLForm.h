#ifndef PluginTLForm_H
#define PluginTLForm_H

#if USE_PLUGIN_TL

#include "../PluginFormAbstract.h"

class PluginTL;
class Label;
class LineEdit;


class PluginTLForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginTLForm(PluginTL* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginTLForm();
    
    virtual void setData(const QJsonObject& data, bool isCombined);
    virtual QJsonObject getData();
    
    bool isValid();
    
protected:
    void resizeEvent(QResizeEvent* e);

private:
    Label* mAverageLab;
    Label* mErrorLab;
    Label* mYearLab;
    
    LineEdit* mAverageEdit;
    LineEdit* mErrorEdit;
    LineEdit* mYearEdit;
};

#endif
#endif
