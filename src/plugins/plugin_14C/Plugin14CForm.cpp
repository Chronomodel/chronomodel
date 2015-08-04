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
    mErrorLab = new Label(tr("Error (sd)") + " :", this);
    mRLab = new Label(tr("ΔR") + " :", this);
    mRErrorLab = new Label(tr("ΔR Error") + " :", this);
    mRefLab = new Label(tr("Reference curve") + " :", this);
    
    /*mRefPathLab = new Label(tr("Folder") + " : " + plugin14C->getRefsPath(), this);
    mRefPathLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);*/
    
    mAverageEdit = new LineEdit(this);
    mAverageEdit->setText("0");
    
    mErrorEdit = new LineEdit(this);
    mErrorEdit->setText("50");
    
    mREdit = new LineEdit(this);
    mREdit->setText("0");
    
    mRErrorEdit = new LineEdit(this);
    mRErrorEdit->setText("0");
    
    mRefCombo = new QComboBox(this);
    QStringList refCurves = plugin14C->getRefsNames();
    for(int i = 0; i<refCurves.size(); ++i)
    {
        mRefCombo->addItem(refCurves[i]);
    }
    QString defCurve = QString("intcal13.14c").toLower();
    if(mSelectedRefCurve.isEmpty() && refCurves.contains(defCurve, Qt::CaseInsensitive))
       mSelectedRefCurve = defCurve;
        
    mRefCombo->setCurrentText(mSelectedRefCurve);
    
    mComboH = mRefCombo->sizeHint().height();
    
    setFixedHeight(sTitleHeight + 4*mLineH + mComboH + 6*mMargin);
}

Plugin14CForm::~Plugin14CForm()
{

}

void Plugin14CForm::setData(const QJsonObject& data, bool isCombined)
{
    double a = data.value(DATE_14C_AGE_STR).toDouble();
    double e = data.value(DATE_14C_ERROR_STR).toDouble();
    double r = data.value(DATE_14C_DELTA_R_STR).toDouble();
    double re = data.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();
    QString c = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();
    
    mAverageEdit->setText(QString::number(a));
    mErrorEdit->setText(QString::number(e));
    mREdit->setText(QString::number(r));
    mRErrorEdit->setText(QString::number(re));
    mRefCombo->setCurrentText(c);
    
    mAverageEdit->setEnabled(!isCombined);
    mErrorEdit->setEnabled(!isCombined);
    mREdit->setEnabled(!isCombined);
    mRErrorEdit->setEnabled(!isCombined);
    mRefCombo->setEnabled(!isCombined);
}

QJsonObject Plugin14CForm::getData()
{
    QJsonObject data;
    
    double a = mAverageEdit->text().toDouble();
    double e = mErrorEdit->text().toDouble();
    double r = mREdit->text().toDouble();
    double re = mRErrorEdit->text().toDouble();
    QString c = mRefCombo->currentText();
    
    data.insert(DATE_14C_AGE_STR, a);
    data.insert(DATE_14C_ERROR_STR, e);
    data.insert(DATE_14C_DELTA_R_STR, r);
    data.insert(DATE_14C_DELTA_R_ERROR_STR, re);
    data.insert(DATE_14C_REF_CURVE_STR, c);
    
    mSelectedRefCurve = c;
    
    return data;
}

bool Plugin14CForm::isValid()
{
    QString refCurve = mRefCombo->currentText();
    if(refCurve.isEmpty())
        mError = tr("Ref. curve is empty!");
    return !refCurve.isEmpty();
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
    mRLab->setGeometry(m, sTitleHeight + 3*m + 2*mLineH, w1, mComboH);
    mRErrorLab->setGeometry(m, sTitleHeight + 4*m + 3*mLineH, w1, mComboH);
    mRefLab->setGeometry(m, sTitleHeight + 5*m + 4*mLineH, w1, mComboH);
    
    mAverageEdit->setGeometry(2*m + w1, sTitleHeight + m, w2, mLineH);
    mErrorEdit->setGeometry(2*m + w1, sTitleHeight + 2*m + mLineH, w2, mLineH);
    mREdit->setGeometry(2*m + w1, sTitleHeight + 3*m + 2*mLineH, w2, mLineH);
    mRErrorEdit->setGeometry(2*m + w1, sTitleHeight + 4*m + 3*mLineH, w2, mLineH);
    mRefCombo->setGeometry(2*m + w1, sTitleHeight + 5*m + 4*mLineH, w2, mComboH);
}

#endif
