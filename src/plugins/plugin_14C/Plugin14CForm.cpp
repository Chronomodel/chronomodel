#include "Plugin14CForm.h"
#if USE_PLUGIN_14C

#include "Plugin14C.h"
#include <QJsonObject>
#include <QtWidgets>

QString Plugin14CForm::mSelectedRefCurve = QString();

Plugin14CForm::Plugin14CForm(Plugin14C* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("14C Measurements"), parent, flags)
{
    Plugin14C* plugin14C = (Plugin14C*)mPlugin;
    
    mAverageLab = new QLabel(tr("Age") + " :", this);
    mErrorLab = new QLabel(tr("Error (sd)") + " :", this);
    mRLab = new QLabel(tr("Reservoir Effect (ΔR)") + " :", this);
    mRErrorLab = new QLabel(tr("ΔR Error") + " :", this);
    mRefLab = new QLabel(tr("Reference curve") + " :", this);
    
    mAverageEdit = new QLineEdit(this);
    mAverageEdit->setText("0");
    
    mErrorEdit = new QLineEdit(this);
    mErrorEdit->setText("50");
    
    mREdit = new QLineEdit(this);
    mREdit->setText("0");
    
    mRErrorEdit = new QLineEdit(this);
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
    
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    
    grid->addWidget(mAverageLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAverageEdit, 0, 1);
    
    grid->addWidget(mErrorLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mErrorEdit, 1, 1);
    
    grid->addWidget(mRLab, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mREdit, 2, 1);
    
    grid->addWidget(mRErrorLab, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mRErrorEdit, 3, 1);
    
    grid->addWidget(mRefLab, 4, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mRefCombo, 4, 1);
    
    setLayout(grid);
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

#endif
