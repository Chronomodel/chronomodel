#include "PluginAMForm.h"
#if USE_PLUGIN_AM

#include "PluginAM.h"
#include <QJsonObject>
#include <QtWidgets>


PluginAMForm::PluginAMForm(PluginAM* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("AM Measurements"), parent, flags)
{
    mIRadio = new QRadioButton(tr("I"));
    mDRadio = new QRadioButton(tr("D"));
    mFRadio = new QRadioButton(tr("F"));
    mIDRadio = new QRadioButton(tr("I/D"));
    mIFRadio = new QRadioButton(tr("I/F"));
    mIDFRadio = new QRadioButton(tr("I/D/F"));
    
    mIRadio->setChecked(true);
    
    connect(mIRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mDRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mFRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mIDRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mIFRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    connect(mIDFRadio, SIGNAL(clicked()), this, SLOT(updateOptions()));
    
    mILab = new QLabel(tr("Inclination") + " :", this);
    mDLab = new QLabel(tr("Declination") + " :", this);
    mFLab = new QLabel(tr("Intensity") + " :", this);
    mAlpha95Lab = new QLabel(tr("Alpha 95") + " :", this);
    mSigmaFLab = new QLabel(tr("Sigma F") + " :", this);
    mCurveILab = new QLabel(tr("Reference curve I") + " :", this);
    mCurveDLab = new QLabel(tr("Reference curve D") + " :", this);
    mCurveFLab = new QLabel(tr("Reference curve F") + " :", this);
    
    mIEdit = new QLineEdit(this);
    mDEdit = new QLineEdit(this);
    mFEdit = new QLineEdit(this);
    mAlpha95Edit = new QLineEdit(this);
    mSigmaFEdit = new QLineEdit(this);
    
    mIEdit->setText("60");
    mDEdit->setText("0");
    mFEdit->setText("0");
    mAlpha95Edit->setText("1");
    mSigmaFEdit->setText("1");
    
    connect(mAlpha95Edit, &QLineEdit::textChanged, this, &PluginAMForm::errorIsValid);
    connect(mSigmaFEdit, &QLineEdit::textChanged, this, &PluginAMForm::errorIsValid);

    mCurveICombo = new QComboBox(this);
    mCurveDCombo = new QComboBox(this);
    mCurveFCombo = new QComboBox(this);
    
    QStringList refCurves = plugin->getRefsNames();
    for(int i = 0; i<refCurves.size(); ++i){
        if(refCurves[i].contains("_I.ref", Qt::CaseInsensitive))
            mCurveICombo->addItem(refCurves[i]);
        else if(refCurves[i].contains("_D.ref", Qt::CaseInsensitive))
            mCurveDCombo->addItem(refCurves[i]);
        else if(refCurves[i].contains("_F.ref", Qt::CaseInsensitive))
            mCurveFCombo->addItem(refCurves[i]);
    }
    
    mCurveICombo->setCurrentIndex(mCurveICombo->findText("i.ref", Qt::MatchEndsWith));
    mCurveDCombo->setCurrentIndex(mCurveICombo->findText("d.ref", Qt::MatchEndsWith));
    mCurveFCombo->setCurrentIndex(mCurveICombo->findText("f.ref", Qt::MatchEndsWith));
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    
    int row = 0;
    
    grid->addWidget(mIRadio, row, 1);
    grid->addWidget(mDRadio, ++row, 1);
    grid->addWidget(mFRadio, ++row, 1);
    grid->addWidget(mIDRadio, ++row, 1);
    grid->addWidget(mIFRadio, ++row, 1);
    grid->addWidget(mIDFRadio, ++row, 1);
    
    grid->addWidget(mILab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mIEdit, row, 1);
    
    grid->addWidget(mDLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mDEdit, row, 1);
    
    grid->addWidget(mFLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mFEdit, row, 1);
    
    grid->addWidget(mAlpha95Lab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAlpha95Edit, row, 1);
    
    grid->addWidget(mSigmaFLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mSigmaFEdit, row, 1);
    
    grid->addWidget(mCurveILab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCurveICombo, row, 1);
    
    grid->addWidget(mCurveDLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCurveDCombo, row, 1);
    
    grid->addWidget(mCurveFLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCurveFCombo, row, 1);
    
    setLayout(grid);

    updateOptions();
}

PluginAMForm::~PluginAMForm()
{

}

void PluginAMForm::setData(const QJsonObject& data, bool isCombined)
{
    Q_UNUSED(isCombined);
    
    QLocale locale = QLocale();
    
    // ----------------------------------
    //  Read values
    // ----------------------------------
    QString mode = data.value(DATE_AM_MODE).toString();
    
    double i = data.value(DATE_AM_I).toDouble();
    double d = data.value(DATE_AM_D).toDouble();
    double f = data.value(DATE_AM_F).toDouble();
    double alpha95 = data.value(DATE_AM_ALPHA_95).toDouble();
    double sigmaF = data.value(DATE_AM_SIGMA_F).toDouble();
    
    QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
    QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
    QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
    
    mIRadio->setChecked(mode == DATE_AM_MODE_I);
    mDRadio->setChecked(mode == DATE_AM_MODE_D);
    mFRadio->setChecked(mode == DATE_AM_MODE_F);
    mIDRadio->setChecked(mode == DATE_AM_MODE_ID);
    mIFRadio->setChecked(mode == DATE_AM_MODE_IF);
    mIDFRadio->setChecked(mode == DATE_AM_MODE_IDF);
    
    mIEdit->setText(locale.toString(i));
    mDEdit->setText(locale.toString(d));
    mFEdit->setText(locale.toString(f));
    mAlpha95Edit->setText(locale.toString(alpha95));
    mSigmaFEdit->setText(locale.toString(sigmaF));
    
    mCurveICombo->setCurrentText(curveI);
    mCurveDCombo->setCurrentText(curveD);
    mCurveFCombo->setCurrentText(curveF);
    
    updateOptions();
}

QJsonObject PluginAMForm::getData()
{
    QJsonObject data;
    const QLocale locale = QLocale();
    
    QString mode;
    if(mIRadio->isChecked()) mode = DATE_AM_MODE_I;
    if(mDRadio->isChecked()) mode = DATE_AM_MODE_D;
    if(mFRadio->isChecked()) mode = DATE_AM_MODE_F;
    if(mIDRadio->isChecked()) mode = DATE_AM_MODE_ID;
    if(mIFRadio->isChecked()) mode = DATE_AM_MODE_IF;
    if(mIDFRadio->isChecked()) mode = DATE_AM_MODE_IDF;
    
    const double i = locale.toDouble(mIEdit->text());
    const double d = locale.toDouble(mDEdit->text());
    const double f = locale.toDouble(mFEdit->text());
    
    const double alpha95 = locale.toDouble(mAlpha95Edit->text());
    const double sigmaF = locale.toDouble(mSigmaFEdit->text());
    
    const QString curveI = mCurveICombo->currentText();
    const QString curveD = mCurveDCombo->currentText();
    const QString curveF = mCurveFCombo->currentText();
    
    data.insert(DATE_AM_MODE, mode);
    data.insert(DATE_AM_I, i);
    data.insert(DATE_AM_D, d);
    data.insert(DATE_AM_F, f);
    data.insert(DATE_AM_ALPHA_95, alpha95);
    data.insert(DATE_AM_SIGMA_F, sigmaF);
    data.insert(DATE_AM_CURVE_I, curveI);
    data.insert(DATE_AM_CURVE_D, curveD);
    data.insert(DATE_AM_CURVE_F, curveF);
    
    return data;
}

void PluginAMForm::errorIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value > 0));
}

bool PluginAMForm::isValid()
{
    const bool isI = mIRadio->isChecked();
    const bool isD = mDRadio->isChecked();
    const bool isF = mFRadio->isChecked();
    
    const bool isID = mIDRadio->isChecked();
    const bool isIF = mIFRadio->isChecked();
    const bool isIDF = mIDFRadio->isChecked();
    
    const QString refCurveI = mCurveICombo->currentText();
    const QString refCurveD = mCurveDCombo->currentText();
    const QString refCurveF = mCurveFCombo->currentText();
    
    const bool isIValid = !refCurveI.isEmpty();
    const bool isDValid = !refCurveD.isEmpty();
    const bool isFValid = !refCurveF.isEmpty();
    
    bool valid = true;
    
    if(isI || isID || isIF || isIDF){
        valid &= isIValid;
    }
    if(isD || isID || isIDF){
        valid &= isDValid;
    }
    if(isF || isIF || isIDF){
        valid &= isFValid;
    }
    
    if(!valid)
        mError = tr("Ref. curve is empty!");
    
    return valid;
}

void PluginAMForm::updateOptions()
{
    const bool isI = mIRadio->isChecked();
    const bool isD = mDRadio->isChecked();
    const bool isF = mFRadio->isChecked();
    
    const bool isID = mIDRadio->isChecked();
    const bool isIF = mIFRadio->isChecked();
    const bool isIDF = mIDFRadio->isChecked();
    
    mIEdit->setEnabled(isI || isD || isID || isIF || isIDF);
    mDEdit->setEnabled(isD || isID || isIDF);
    mFEdit->setEnabled(isF || isIF || isIDF);
    
    mAlpha95Edit->setEnabled(isI || isD || isID || isIF || isIDF);
    mSigmaFEdit->setEnabled(isF || isIF || isIDF);
    
    mCurveICombo->setEnabled(isI || isID || isIF || isIDF);
    mCurveDCombo->setEnabled(isD || isID || isIDF);
    mCurveFCombo->setEnabled(isF || isIF || isIDF);
}

#endif
