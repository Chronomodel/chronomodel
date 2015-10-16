#ifndef PLUGINMAGSETTINGSVIEW_H
#define PLUGINMAGSETTINGSVIEW_H

#if USE_PLUGIN_AM

#include "../PluginSettingsViewAbstract.h"
#include <QMap>

class PluginMag;
class QLabel;
class QListWidget;
class QPushButton;

class PluginMagSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginMagSettingsView(PluginMag* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginMagSettingsView();
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

