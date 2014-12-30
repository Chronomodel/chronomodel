#include "Plugin14CForm.h"
#if USE_PLUGIN_14C

#include "Plugin14C.h"
#include "Label.h"
#include "LineEdit.h"
#include <QJsonObject>
#include <QtWidgets>

QString Plugin14CForm::mSelectedRefCurve = QString();

Plugin14CForm::Plugin14CForm(Plugin14C* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("14C Measurements"), parent, flags)
{
    Plugin14C* plugin14C = (Plugin14C*)mPlugin;
    
    mAverageLab = new Label(tr("Age") + " :", this);
    mErrorLab = new Label(tr("Error") + " :", this);
    mRefLab = new Label(tr("Reference curve") + " :", this);
    mRefPathLab = new Label(tr("Folder") + " : " + plugin14C->getRefsPath(), this);
    mRefPathLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    mAverageEdit = new LineEdit(this);
    mAverageEdit->setText("0");
    
    mErrorEdit = new LineEdit(this);
    mErrorEdit->setText("50");
    
    mRefCombo = new QComboBox(this);
    QStringList refCurves = plugin14C->getRefsNames();
    mRefCombo->addItem("");
    for(int i = 0; i<refCurves.size(); ++i)
    {
        mRefCombo->addItem(refCurves[i]);
    }
    QString defCurve = QString("intcal13.14c").toLower();
    if(mSelectedRefCurve.isEmpty() && refCurves.contains(defCurve, Qt::CaseInsensitive))
       mSelectedRefCurve = defCurve;
        
    mRefCombo->setCurrentText(mSelectedRefCurve);
    
    mComboH = mRefCombo->sizeHint().height();
    
    setFixedHeight(sTitleHeight + 3*mLineH + mComboH + 5*mMargin);
}

Plugin14CForm::~Plugin14CForm()
{

}

void Plugin14CForm::setData(const QJsonObject& data)
{
    double a = data.value(DATE_14C_AGE_STR).toDouble();
    double e = data.value(DATE_14C_ERROR_STR).toDouble();
    QString r = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();
    
    mAverageEdit->setText(QString::number(a));
    mErrorEdit->setText(QString::number(e));
    mRefCombo->setCurrentText(r);
}

QJsonObject Plugin14CForm::getData()
{
    QJsonObject data;
    
    double a = mAverageEdit->text().toDouble();
    double e = mErrorEdit->text().toDouble();
    QString r = mRefCombo->currentText();
    
    data.insert(DATE_14C_AGE_STR, a);
    data.insert(DATE_14C_ERROR_STR, e);
    data.insert(DATE_14C_REF_CURVE_STR, r);
    
    mSelectedRefCurve = r;
    
    return data;
}

void Plugin14CForm::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = mMargin;
    int w = width();
    int w1 = 100;
    int w2 = w - 3*m - w1;
    
    mAverageLab->setGeometry(m, sTitleHeight + m, w1, mLineH);
    mErrorLab->setGeometry(m, sTitleHeight + 2*m + mLineH, w1, mLineH);
    mRefLab->setGeometry(m, sTitleHeight + 3*m + 2*mLineH, w1, mComboH);
    
    mAverageEdit->setGeometry(2*m + w1, sTitleHeight + m, w2, mLineH);
    mErrorEdit->setGeometry(2*m + w1, sTitleHeight + 2*m + mLineH, w2, mLineH);
    mRefCombo->setGeometry(2*m + w1, sTitleHeight + 3*m + 2*mLineH, w2, mComboH);
    mRefPathLab->setGeometry(2*m + w1, sTitleHeight + 4*m + 2*mLineH + mComboH, w2, mLineH);
}

#endif
