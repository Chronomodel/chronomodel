#ifndef PLUGINGAUSSSETTINGSVIEW_H
#define PLUGINGAUSSSETTINGSVIEW_H

#if USE_PLUGIN_GAUSS

#include "PluginSettingsViewAbstract.h"
#include <QMap>

class PluginGauss;
class Label;
class QListWidget;
class Button;


class PluginGaussSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginGaussSettingsView(PluginGauss* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginGaussSettingsView();
    
    void updateRefsList();
    
    protected slots:
    void addRefCurve();
    void deleteRefCurve();
    void onAccepted();
    
protected:
    void resizeEvent(QResizeEvent*);
    
private:
    Label* mRefCurvesLab;
    QListWidget* mRefCurvesList;
    Button* mAddRefCurveBut;
    Button* mDeleteRefCurveBut;
    
    QMap<QString, QString> mFilesOrg;
    QMap<QString, QString> mFilesNew;
};

#endif
#endif

