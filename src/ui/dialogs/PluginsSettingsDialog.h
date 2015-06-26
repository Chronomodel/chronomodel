#ifndef PLUGINSSETTINGSDIALOG_H
#define PLUGINSSETTINGSDIALOG_H

#include <QDialog>

class Button;
class Label;
class ColorPicker;
class PluginAbstract;
class PluginSettingsViewAbstract;


class PluginsSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    PluginsSettingsDialog(PluginAbstract* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginsSettingsDialog();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
protected slots:
    void updateColor(QColor c);
    
private:
    PluginAbstract* mPlugin;
    
    Label* mTitleLab;
    
    Label* mColorLab;
    ColorPicker* mColorPicker;
    
    PluginSettingsViewAbstract* mView;
    
    Button* mOkBut;
    Button* mCancelBut;
};

#endif
