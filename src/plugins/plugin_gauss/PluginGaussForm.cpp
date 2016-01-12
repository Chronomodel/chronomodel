#include "PluginGaussForm.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "Label.h"
#include "LineEdit.h"
#include "RadioButton.h"
#include <QJsonObject>
#include <QtWidgets>


PluginGaussForm::PluginGaussForm(PluginGauss* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("Gaussian measurement"), parent, flags)
{
    //PluginGauss* pluginGauss = (PluginGauss*)mPlugin;
    
    mAverageLab = new QLabel(tr("Measure") + " :", this);
    mErrorLab = new QLabel(tr("Error (sd)") + " :", this);
    mCalibLab = new QLabel(tr("Calibration") + " :", this);
    
    mAverageEdit = new QLineEdit(this);
    mErrorEdit = new QLineEdit(this);
    connect(mErrorEdit, &QLineEdit::textChanged, this, &PluginGaussForm::errorIsValid);
    
    mAverageEdit->setText("0");
    mErrorEdit->setText("50");
    
    mEqWidget = new QWidget();
    
    mEq1Lab = new QLabel("g(t) = ", this);
    mEq2Lab = new QLabel(" t^2 + ", this);
    mEq3Lab = new QLabel(" t + ", this);
    mEq1Lab->setAlignment(Qt::AlignCenter);
    mEq2Lab->setAlignment(Qt::AlignCenter);
    mEq3Lab->setAlignment(Qt::AlignCenter);
    
    mAEdit = new QLineEdit(this);
    mBEdit = new QLineEdit(this);
    mCEdit = new QLineEdit(this);
    mAEdit->setText("0");
    mBEdit->setText("1");
    mCEdit->setText("0");
    connect(mAEdit, &QLineEdit::textChanged, this, &PluginGaussForm::equationIsValid);
    connect(mBEdit, &QLineEdit::textChanged, this, &PluginGaussForm::equationIsValid);
    
    QHBoxLayout* eqLayout = new QHBoxLayout();
    eqLayout->setContentsMargins(0, 0, 0, 0);
    eqLayout->QLayout::addWidget(mEq1Lab);
    eqLayout->QLayout::addWidget(mAEdit);
    eqLayout->QLayout::addWidget(mEq2Lab);
    eqLayout->QLayout::addWidget(mBEdit);
    eqLayout->QLayout::addWidget(mEq3Lab);
    eqLayout->QLayout::addWidget(mCEdit);
    mEqWidget->setLayout(eqLayout);
    
    mNoneRadio = new QRadioButton(tr("None"), this);
    mEquationRadio = new QRadioButton(tr("Build your equation"), this);
    mCurveRadio = new QRadioButton(tr("Use custom curve file"), this);
    
    mNoneRadio->setChecked(true);
    
    mCurveCombo = new QComboBox(this);
    QStringList refCurves = plugin->getRefsNames();
    for(int i = 0; i<refCurves.size(); ++i){
         mCurveCombo->addItem(refCurves[i]);
    }
    
    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);
    connect(mNoneRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);
    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::updateVisibleElements);
    
    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::equationIsValid);
    connect(mNoneRadio, &QRadioButton::toggled, this, &PluginGaussForm::equationIsValid);
    connect(mEquationRadio, &QRadioButton::toggled, this, &PluginGaussForm::equationIsValid);

    updateVisibleElements();
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(mAverageLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAverageEdit, 0, 1);
    
    grid->addWidget(mErrorLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mErrorEdit, 1, 1);
    
    grid->addWidget(mCalibLab, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mNoneRadio, 2, 1);
    grid->addWidget(mEquationRadio, 3, 1);
    grid->addWidget(mCurveRadio, 4, 1);
    
    grid->addWidget(mEqWidget, 5, 1);
    grid->addWidget(mCurveCombo, 5, 1);
    
    setLayout(grid);
}

PluginGaussForm::~PluginGaussForm()
{

}

void PluginGaussForm::setData(const QJsonObject& data, bool isCombined)
{
    QLocale locale=QLocale();
    double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
    double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
    double a = data.value(DATE_GAUSS_A_STR).toDouble();
    double b = data.value(DATE_GAUSS_B_STR).toDouble();
    double c = data.value(DATE_GAUSS_C_STR).toDouble();
    QString mode = data.value(DATE_GAUSS_MODE_STR).toString();
    QString curve = data.value(DATE_GAUSS_CURVE_STR).toString();
    
    mAverageEdit->setText(locale.toString(age));
    mErrorEdit->setText(locale.toString(error));
    mAEdit->setText(locale.toString(a));
    mBEdit->setText(locale.toString(b));
    mCEdit->setText(locale.toString(c));
    
    mCurveRadio->setChecked(mode == DATE_GAUSS_MODE_CURVE);
    mEquationRadio->setChecked(mode == DATE_GAUSS_MODE_EQ);
    mNoneRadio->setChecked(mode == DATE_GAUSS_MODE_NONE);
    
    mCurveCombo->setCurrentText(curve);
    updateVisibleElements();
}

void PluginGaussForm::updateVisibleElements()
{
    mCurveCombo->setVisible(mCurveRadio->isChecked());
    mEqWidget->setVisible(mEquationRadio->isChecked());
    adjustSize();
}

QJsonObject PluginGaussForm::getData()
{
    QJsonObject data;
    QLocale locale=QLocale();
    
    double age = locale.toDouble(mAverageEdit->text());
    double error = locale.toDouble(mErrorEdit->text());
    double a = locale.toDouble(mAEdit->text());
    double b = locale.toDouble(mBEdit->text());
    double c = locale.toDouble(mCEdit->text());
    
    QString mode = "";
    if(mCurveRadio->isChecked()) mode = DATE_GAUSS_MODE_CURVE;
    else if(mEquationRadio->isChecked()) mode = DATE_GAUSS_MODE_EQ;
    else if(mNoneRadio->isChecked()) mode = DATE_GAUSS_MODE_NONE;
    
    QString curve = mCurveCombo->currentText();
    
    data.insert(DATE_GAUSS_AGE_STR, age);
    data.insert(DATE_GAUSS_ERROR_STR, error);
    data.insert(DATE_GAUSS_A_STR, a);
    data.insert(DATE_GAUSS_B_STR, b);
    data.insert(DATE_GAUSS_C_STR, c);
    data.insert(DATE_GAUSS_MODE_STR, mode);
    data.insert(DATE_GAUSS_CURVE_STR, curve);
    
    return data;
}

void PluginGaussForm::errorIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str,&ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

void PluginGaussForm::equationIsValid()
{
    if(mEquationRadio->isChecked()) {
        bool oka,okb;
        QLocale locale;
        double a = locale.toDouble(mAEdit->text(),&oka);
        if(a == 0) oka = false;

        double b = locale.toDouble(mBEdit->text(),&okb);
        if(b == 0) okb = false;

        emit PluginFormAbstract::OkEnabled(oka || okb);
    }
    else emit PluginFormAbstract::OkEnabled(true);
}

bool PluginGaussForm::isValid()
{
    return true;
}


#endif
