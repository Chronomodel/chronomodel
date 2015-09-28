#ifndef PluginTLForm_H
#define PluginTLForm_H

#if USE_PLUGIN_TL

#include "../PluginFormAbstract.h"

class PluginTL;
class QLabel;
class QLineEdit;


class PluginTLForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginTLForm(PluginTL* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginTLForm();
    
    virtual void setData(const QJsonObject& data, bool isCombined);
    virtual QJsonObject getData();
    
    bool isValid();

private:
    QLabel* mAverageLab;
    QLabel* mErrorLab;
    QLabel* mYearLab;
    
    QLineEdit* mAverageEdit;
    QLineEdit* mErrorEdit;
    QLineEdit* mYearEdit;
};

#endif
#endif
