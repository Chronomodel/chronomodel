#ifndef Plugin14CForm_H
#define Plugin14CForm_H

#if USE_PLUGIN_14C

#include "../PluginFormAbstract.h"

class Plugin14C;
class QLineEdit;
class QComboBox;
class QLabel;


class Plugin14CForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    Plugin14CForm(Plugin14C* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~Plugin14CForm();
    
    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();
    
    bool isValid();
signals:
    void OkEnabled(bool enabled) ;

protected slots:
    void errorIsValid(QString str);

private:
    QLabel* mAverageLab;
    QLabel* mErrorLab;
    QLabel* mRLab;
    QLabel* mRErrorLab;
    QLabel* mRefLab;
    
    QLineEdit* mAverageEdit;
    QLineEdit* mErrorEdit;
    QLineEdit* mREdit;
    QLineEdit* mRErrorEdit;
    QComboBox* mRefCombo;
    
    static QString mSelectedRefCurve;
};

#endif

#endif
