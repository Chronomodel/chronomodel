#ifndef PLUGINGAUSSSETTINGSVIEW_H
#define PLUGINGAUSSSETTINGSVIEW_H

#if USE_PLUGIN_GAUSS

#include "../PluginSettingsViewAbstract.h"
#include <QMap>

class PluginGauss;
class QLabel;
class QListWidget;
class QPushButton;


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
    
private:
    QLabel* mRefCurvesLab;
    QListWidget* mRefCurvesList;
    QPushButton* mAddRefCurveBut;
    QPushButton* mDeleteRefCurveBut;
    
    QMap<QString, QString> mFilesOrg;
    QMap<QString, QString> mFilesNew;
};

#endif
#endif

