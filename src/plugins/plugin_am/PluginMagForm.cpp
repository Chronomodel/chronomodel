#include "PluginMagForm.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include "Label.h"
#include "LineEdit.h"
#include "RadioButton.h"
#include <QJsonObject>
#include <QtWidgets>


PluginMagForm::PluginMagForm(PluginMag* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("AM Measurements"), parent, flags)
{
    PluginMag* pluginMag = (PluginMag*)mPlugin;
    
    mIncRadio = new RadioButton(this);
    mDecRadio = new RadioButton(this);
    mIntensityRadio = new RadioButton(this);
    
    connect(mIncRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mDecRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mIntensityRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    
    mIncLab = new Label(tr("Inclination") + " :", this);
    mDecLab = new Label(tr("Declination") + " :", this);
    mDecIncLab = new Label(tr("Inclination") + " :", this);
    mIntensityLab = new Label(tr("Intensity") + " :", this);
    mAlpha95Lab = new Label(tr("Alpha 95") + " :", this);
    mRefLab = new Label(tr("Reference") + " :", this);
    mRefPathLab = new Label(tr("Folder") + " : " + pluginMag->getRefsPath(), this);
    mRefPathLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mRefPathLab->setWordWrap(true);
    
    mIncEdit = new LineEdit(this);
    mDecEdit = new LineEdit(this);
    mDecIncEdit = new LineEdit(this);
    mIntensityEdit = new LineEdit(this);
    mAlpha95Edit = new LineEdit(this);
    
    mRefCombo = new QComboBox(this);
    QStringList refCurves = pluginMag->getRefsNames();
    for(int i = 0; i<refCurves.size(); ++i)
        mRefCombo->addItem(refCurves[i]);
    mComboH = mRefCombo->sizeHint().height();
    
    mIncEdit->setText("60");
    mDecEdit->setText("0");
    mDecIncEdit->setText("60");
    mIntensityEdit->setText("0");
    mAlpha95Edit->setText("1");
    
    mIncRadio->setChecked(true);
    
    setFixedHeight(sTitleHeight + 6*mLineH + mComboH + 7*mMargin);
    
    updateOptions();
}

PluginMagForm::~PluginMagForm()
{

}

void PluginMagForm::setData(const QJsonObject& data)
{
    bool is_inc = data.value(DATE_AM_IS_INC_STR).toBool();
    bool is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
    bool is_int = data.value(DATE_AM_IS_INT_STR).toBool();
    
    double inc = data.value(DATE_AM_INC_STR).toDouble();
    double dec = data.value(DATE_AM_DEC_STR).toDouble();
    double intensity = data.value(DATE_AM_INTENSITY_STR).toDouble();
    double error = data.value(DATE_AM_ERROR_STR).toDouble();
    QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    
    mIncRadio->setChecked(is_inc);
    mDecRadio->setChecked(is_dec);
    mIntensityRadio->setChecked(is_int);
    
    mIncEdit->setText(QString::number(inc));
    mDecEdit->setText(QString::number(dec));
    mDecIncEdit->setText(QString::number(inc));
    mIntensityEdit->setText(QString::number(intensity));
    mAlpha95Edit->setText(QString::number(error));
    mRefCombo->setCurrentText(ref_curve);
    
    updateOptions();
}

QJsonObject PluginMagForm::getData()
{
    QJsonObject data;
    
    bool is_inc = mIncRadio->isChecked();
    bool is_dec = mDecRadio->isChecked();
    bool is_int = mIntensityRadio->isChecked();
    
    double inc = mIncEdit->text().toDouble();
    double dec = mDecEdit->text().toDouble();
    if(is_dec)
        inc = mDecIncEdit->text().toDouble();
    double intensity = mIntensityEdit->text().toDouble();
    double error = mAlpha95Edit->text().toDouble();
    QString ref_curve = mRefCombo->currentText();
    
    data.insert(DATE_AM_IS_INC_STR, is_inc);
    data.insert(DATE_AM_IS_DEC_STR, is_dec);
    data.insert(DATE_AM_IS_INT_STR, is_int);
    
    data.insert(DATE_AM_INC_STR, inc);
    data.insert(DATE_AM_DEC_STR, dec);
    data.insert(DATE_AM_INTENSITY_STR, intensity);
    data.insert(DATE_AM_ERROR_STR, error);
    data.insert(DATE_AM_REF_CURVE_STR, ref_curve);
    
    return data;
}

bool PluginMagForm::isValid()
{
    QString refCurve = mRefCombo->currentText();
    if(refCurve.isEmpty())
        mError = tr("Ref. curve is empty!");
    return !refCurve.isEmpty();
}

void PluginMagForm::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int m = mMargin;
    int w = width();
    int r = mLineH;
    int w1 = (w - 5*m) / 4;
    int w2 = w1 - m - r;
    int w3 = w - 3*m - w1;
    
    mIncRadio->setGeometry(m, sTitleHeight + m, r, mLineH);
    mDecRadio->setGeometry(m, sTitleHeight + 2*m + mLineH, r, mLineH);
    mIntensityRadio->setGeometry(m, sTitleHeight + 3*m + 2*mLineH, r, mLineH);
    
    mIncLab->setGeometry(2*m + r, sTitleHeight + m, w2, mLineH);
    mDecLab->setGeometry(2*m + r, sTitleHeight + 2*m + mLineH, w2, mLineH);
    mDecIncLab->setGeometry(3*m + 2*w1, sTitleHeight + 2*m + mLineH, w1, mLineH);
    mIntensityLab->setGeometry(2*m + r, sTitleHeight + 3*m + 2*mLineH, w2, mLineH);
    mAlpha95Lab->setGeometry(m, sTitleHeight + 4*m + 3*mLineH, w1, mLineH);
    mRefLab->setGeometry(m, sTitleHeight + 5*m + 4*mLineH, w1, mComboH);
    
    mIncEdit->setGeometry(2*m + w1, sTitleHeight + m, w3, mLineH);
    mDecEdit->setGeometry(2*m + w1, sTitleHeight + 2*m + mLineH, w1, mLineH);
    mDecIncEdit->setGeometry(4*m + 3*w1, sTitleHeight + 2*m + mLineH, w1, mLineH);
    mIntensityEdit->setGeometry(2*m + w1, sTitleHeight + 3*m + 2*mLineH, w3, mLineH);
    mAlpha95Edit->setGeometry(2*m + w1, sTitleHeight + 4*m + 3*mLineH, w3, mLineH);
    mRefCombo->setGeometry(2*m + w1, sTitleHeight + 5*m + 4*mLineH, w3, mComboH);
    mRefPathLab->setGeometry(2*m + w1, sTitleHeight + 6*m + 4*mLineH + mComboH, w3, 2*mLineH);
}

void PluginMagForm::updateOptions()
{
    mIncEdit->setEnabled(mIncRadio->isChecked());
    mDecEdit->setEnabled(mDecRadio->isChecked());
    mDecIncEdit->setEnabled(mDecRadio->isChecked());
    mIntensityEdit->setEnabled(mIntensityRadio->isChecked());
    
    if(mIntensityRadio->isChecked())
        mAlpha95Lab->setText(tr("Error (sd)") + " :");
    else
        mAlpha95Lab->setText(tr("Alpha 95") + " :");
}

#endif
