#ifndef PluginRefCurveSettingsView_H
#define PluginRefCurveSettingsView_H

#include <QWidget>
#include <QMap>

class QLabel;
class QListWidget;
class QPushButton;
class PluginAbstract;

class PluginRefCurveSettingsView: public QWidget
{
    Q_OBJECT
public:
    PluginRefCurveSettingsView(PluginAbstract* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginRefCurveSettingsView();
    
    void updateFilesInFolder();
    void updateRefsList();
    
protected slots:
    void addRefCurve();
    void deleteRefCurve();
   // void openSelectedFile();
    void updateSelection();
    
private:
    PluginAbstract* mPlugin;
    
    QLabel* mRefCurvesLab;
    QListWidget* mRefCurvesList;
    QPushButton* mAddRefCurveBut;
    QPushButton* mDeleteRefCurveBut;
    QPushButton* mOpenBut;
    
    QMap<QString, QString> mFilesOrg;
    QMap<QString, QString> mFilesNew;
};

#endif

