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
    mAverageLab = new QLabel(tr("Measure") + " :", this);
    mErrorLab = new QLabel(tr("Error (sd)") + " :", this);
    mCalibLab = new QLabel(tr("Calibration") + " :", this);
    
    mAverageEdit = new QLineEdit(this);
    mErrorEdit = new QLineEdit(this);
    
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
    
    connect(mEquationRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleElements()));
    connect(mNoneRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleElements()));
    connect(mEquationRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleElements()));
    
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
    double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
    double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
    double a = data.value(DATE_GAUSS_A_STR).toDouble();
    double b = data.value(DATE_GAUSS_B_STR).toDouble();
    double c = data.value(DATE_GAUSS_C_STR).toDouble();
    QString mode = data.value(DATE_GAUSS_MODE_STR).toString();
    QString curve = data.value(DATE_GAUSS_CURVE_STR).toString();
    
    mAverageEdit->setText(QString::number(age));
    mErrorEdit->setText(QString::number(error));
    mAEdit->setText(QString::number(a));
    mBEdit->setText(QString::number(b));
    mCEdit->setText(QString::number(c));
    
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
    
    double age = mAverageEdit->text().toDouble();
    double error = mErrorEdit->text().toDouble();
    double a = mAEdit->text().toDouble();
    double b = mBEdit->text().toDouble();
    double c = mCEdit->text().toDouble();
    
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

bool PluginGaussForm::isValid()
{
    return true;
}


#endif
