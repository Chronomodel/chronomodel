#ifndef PLUGINSSETTINGSDIALOG_H
#define PLUGINSSETTINGSDIALOG_H

#include <QDialog>

class QLabel;
class QComboBox;
class QDialogButtonBox;
class ColorPicker;
class PluginAbstract;
class PluginSettingsViewAbstract;


class PluginsSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    PluginsSettingsDialog(PluginAbstract* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginsSettingsDialog();
    
protected slots:
    void updateColor(QColor c);
    
private:
    PluginAbstract* mPlugin;
    
    QLabel* mColorLab;
    ColorPicker* mColorPicker;
    
    QLabel* mMethodLab;
    QComboBox* mMethodCombo;
    
    PluginSettingsViewAbstract* mView;
    
    QDialogButtonBox* mButtonBox;
};

#endif
