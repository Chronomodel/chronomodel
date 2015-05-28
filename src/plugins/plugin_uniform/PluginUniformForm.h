#ifndef PluginUniformForm_H
#define PluginUniformForm_H

#if USE_PLUGIN_UNIFORM

#include "../PluginFormAbstract.h"

class PluginUniform;
class LineEdit;
class Label;


class PluginUniformForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginUniformForm(PluginUniform* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginUniformForm();
    
    void setData(const QJsonObject& data);
    QJsonObject getData();
    
    bool isValid();
    
protected:
    void resizeEvent(QResizeEvent* e);

private:
    Label* mMinLab;
    Label* mMaxLab;
    
    LineEdit* mMinEdit;
    LineEdit* mMaxEdit;
};

#endif
#endif
