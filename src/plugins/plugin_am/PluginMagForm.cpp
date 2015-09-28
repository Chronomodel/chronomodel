#include "PluginMagForm.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include <QJsonObject>
#include <QtWidgets>


PluginMagForm::PluginMagForm(PluginMag* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("AM Measurements"), parent, flags)
{
    PluginMag* pluginMag = (PluginMag*)mPlugin;
    
    mIncRadio = new QRadioButton(tr("Inclination"));
    mDecRadio = new QRadioButton(tr("Declination"));
    mIntensityRadio = new QRadioButton(tr("Intensity"));
    
    connect(mIncRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mDecRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mIntensityRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    
    mIncLab = new QLabel(tr("Inclination") + " :", this);
    mDecLab = new QLabel(tr("Declination") + " :", this);
    mDecIncLab = new QLabel(tr("Inclination") + " :", this);
    mIntensityLab = new QLabel(tr("Intensity") + " :", this);
    mAlpha95Lab = new QLabel(tr("Alpha 95") + " :", this);
    mRefLab = new QLabel(tr("Reference") + " :", this);
    
    mIncEdit = new QLineEdit(this);
    mDecEdit = new QLineEdit(this);
    mDecIncEdit = new QLineEdit(this);
    mIntensityEdit = new QLineEdit(this);
    mAlpha95Edit = new QLineEdit(this);
    
    mRefCombo = new QComboBox(this);
    QStringList refCurves = pluginMag->getRefsNames();
    for(int i = 0; i<refCurves.size(); ++i)
        mRefCombo->addItem(refCurves[i]);
    
    mIncEdit->setText("60");
    mDecEdit->setText("0");
    mDecIncEdit->setText("60");
    mIntensityEdit->setText("0");
    mAlpha95Edit->setText("1");
    
    mIncRadio->setChecked(true);
    
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    
    grid->addWidget(mIncRadio, 0, 1);
    grid->addWidget(mDecRadio, 1, 1);
    grid->addWidget(mIntensityRadio, 2, 1);
    
    grid->addWidget(mIncLab, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mIncEdit, 3, 1);
    
    grid->addWidget(mDecLab, 4, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mDecEdit, 4, 1);
    
    grid->addWidget(mDecIncLab, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mDecIncEdit, 3, 1);
    
    grid->addWidget(mIntensityLab, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mIntensityEdit, 3, 1);
    
    grid->addWidget(mAlpha95Lab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAlpha95Edit, 5, 1);
    
    grid->addWidget(mRefLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mRefCombo, 6, 1);
    
    setLayout(grid);
    
    
    
    updateOptions();
}

PluginMagForm::~PluginMagForm()
{

}

void PluginMagForm::setData(const QJsonObject& data, bool isCombined)
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

void PluginMagForm::updateOptions()
{
    mIncEdit->setVisible(mIncRadio->isChecked());
    mIncLab->setVisible(mIncRadio->isChecked());
    
    mDecEdit->setVisible(mDecRadio->isChecked());
    mDecLab->setVisible(mDecRadio->isChecked());
    
    mDecIncEdit->setVisible(mDecRadio->isChecked());
    mDecIncLab->setVisible(mDecRadio->isChecked());
    
    mIntensityEdit->setVisible(mIntensityRadio->isChecked());
    mIntensityLab->setVisible(mIntensityRadio->isChecked());
    
    if(mIntensityRadio->isChecked())
        mAlpha95Lab->setText(tr("Error (sd)") + " :");
    else
        mAlpha95Lab->setText(tr("Alpha 95") + " :");
}

#endif
