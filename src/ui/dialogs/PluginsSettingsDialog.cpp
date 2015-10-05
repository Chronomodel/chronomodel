#include "PluginsSettingsDialog.h"
#include "Label.h"
#include "Button.h"
#include "ColorPicker.h"
#include "PluginManager.h"
#include "ModelUtilities.h"
#include "Date.h"
#include "../PluginAbstract.h"
#include "../PluginSettingsViewAbstract.h"
#include <QtWidgets>


PluginsSettingsDialog::PluginsSettingsDialog(PluginAbstract* plugin, QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags),
mPlugin(plugin)
{
    QString title = plugin->getName() + " " + tr("Settings");
    setWindowTitle(title);
    
    /*mLangHelpLab = new QLabel(tr("The following parameters can "), this);
    QFont f;
    f.setPointSize(pointSize(11));
    mLangHelpLab->setFont(f);
    mLangHelpLab->setAlignment(Qt::AlignCenter);
    mLangHelpLab->setWordWrap(true);*/
    
    // ----------------------------------------
    //  Common form for all plugins
    // ----------------------------------------
    mColorLab = new Label(tr("Color") + " :", this);
    mColorPicker = new ColorPicker(mPlugin->getColor(), this);
    
    mMethodLab = new QLabel(tr("Method") + " :", this);
    mMethodCombo = new QComboBox(this);
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eMHSymetric));
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eInversion));
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eMHSymGaussAdapt));
    
    
    // ----------------------------------------
    //  This form is plugin specific
    // ----------------------------------------
    mView = plugin->getSettingsView();
    if(mView){
        // useless using a layout below...
        mView->setParent(this);
        mView->setVisible(true);
    }
    
    // ----------------------------------------
    //  Connections
    // ----------------------------------------
    connect(this, SIGNAL(accepted()), mView, SLOT(onAccepted()));
    connect(mColorPicker, SIGNAL(colorChanged(QColor)), this, SLOT(updateColor(QColor)));
    
    // ----------------------------------------
    //  Buttons
    // ----------------------------------------
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
    // ----------------------------------------
    //  Layout
    // ----------------------------------------
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(mColorLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mColorPicker, 0, 1);
    grid->addWidget(mMethodLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mMethodCombo, 1, 1);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(grid);
    if(mView){
        QFrame* line1 = new QFrame();
        line1->setFrameShape(QFrame::HLine);
        line1->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line1);
        layout->addWidget(mView);
    }
    layout->addWidget(mButtonBox);
    
    setLayout(layout);
    
    setFixedWidth(450);
}

PluginsSettingsDialog::~PluginsSettingsDialog()
{
    
}

void PluginsSettingsDialog::updateColor(QColor c)
{
    mPlugin->mColor = c;
}
