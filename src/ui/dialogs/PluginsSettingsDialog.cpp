#include "PluginsSettingsDialog.h"
#include "Label.h"
#include "Button.h"
#include "ColorPicker.h"
#include "PluginManager.h"
#include "PluginAbstract.h"
#include "PluginSettingsViewAbstract.h"
#include <QtWidgets>


PluginsSettingsDialog::PluginsSettingsDialog(PluginAbstract* plugin, QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags),
mPlugin(plugin)
{
    QString title = plugin->getName() + " " + tr("Settings");
    setWindowTitle(title);
    
    mTitleLab = new Label(title, this);
    mTitleLab->setIsTitle(true);
    
    mColorLab = new Label(tr("Color") + " :", this);
    mColorPicker = new ColorPicker(mPlugin->getColor(), this);
    
    mView = plugin->getSettingsView();
    mView->setParent(this);
    mView->setVisible(true);
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    mOkBut->setAutoDefault(true);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(this, SIGNAL(accepted()), mView, SLOT(onAccepted()));
    connect(mColorPicker, SIGNAL(colorChanged(QColor)), this, SLOT(updateColor(QColor)));
    
    resize(400, 400);
}

PluginsSettingsDialog::~PluginsSettingsDialog()
{

}

void PluginsSettingsDialog::updateColor(QColor c)
{
    mPlugin->mColor = c;
}

void PluginsSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    int lineH = 20;
    int w = width() - 2*m;
    int w1 = 100;
    int w2 = width() - 3*m - w1;
    int butW = 80;
    int butH = 25;
    int viewH = height() - 2*lineH - butH - 5*m;
    
    int y = -lineH;
    
    mTitleLab->setGeometry(m, y += (lineH + m), width() - 2*m, lineH);
    
    mColorLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mColorPicker->setGeometry(2*m + w1, y, w2, lineH);
    
    mView->setGeometry(m, y += (lineH + m), w, viewH);
    
    mOkBut->setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

void PluginsSettingsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(180, 180, 180));
}

