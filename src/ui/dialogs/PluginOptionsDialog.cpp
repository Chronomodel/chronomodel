#include "PluginOptionsDialog.h"
#include "Button.h"
#include "LineEdit.h"
#include "Label.h"
#include "CheckBox.h"
#include "Painting.h"
#include "PluginManager.h"
#include "Plugin14C.h"
#include <QtWidgets>

/**
 * @todo transform this dialog with autocharging plugin
 */
PluginOptionsDialog::PluginOptionsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("Data Options"));
    
    mTitleLab = new Label(tr("Data Options"), this);
    mTitleLab->setIsTitle(true);
    
    mC14RefLab = new Label(tr("Change all 14C references to") + " : ", this);
    mC14RefCombo = new QComboBox(this);
    mComboH = mC14RefCombo->sizeHint().height();
    Plugin14C* plugin14C = (Plugin14C*)PluginManager::getPluginFromName("14C");
    QStringList refCurves = plugin14C->getRefsNames();
    for(int i = 0; i<refCurves.size(); ++i)
    {
        mC14RefCombo->addItem(refCurves[i]);
    }
    QString defCurve = QString("intcal13.14c").toLower();
    
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    mOkBut->setAutoDefault(true);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    setFixedSize(500, 90);
}

PluginOptionsDialog::~PluginOptionsDialog()
{

}

QString PluginOptionsDialog::getC14Ref() const
{
    return mC14RefCombo->currentText();
}

void PluginOptionsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    int lineH = 20;
    int w1 = 200;
    int w2 = width() - 3*m - w1;
    int butW = 80;
    int butH = 25;
    
    int y = -lineH;
    
    mTitleLab->setGeometry(m, y += (lineH + m), width() - 2*m, lineH);
    
    mC14RefLab->setGeometry(m, y += (lineH + m), w1, mComboH);
    mC14RefCombo->setGeometry(2*m + w1, y, w2, mComboH);
    
    mOkBut->setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

void PluginOptionsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(180, 180, 180));
}

