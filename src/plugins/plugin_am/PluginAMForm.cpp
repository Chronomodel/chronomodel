#include "PluginAMForm.h"
#if USE_PLUGIN_AM

#include "PluginAM.h"
#include <QJsonObject>
#include <QtWidgets>


PluginAMForm::PluginAMForm(PluginAM* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("AM Measurements"), parent, flags)
{
    
    mIDRadio = new QRadioButton(tr("I/D"));
    mIFRadio = new QRadioButton(tr("I/F"));
    mIDFRadio = new QRadioButton(tr("I/D/F"));
    
    mIDRadio->setChecked(true);
    
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
        mCurveICombo->addItem(refCurves[i]);
        mCurveDCombo->addItem(refCurves[i]);
        mCurveFCombo->addItem(refCurves[i]);
    }
    
    mCurveICombo->setCurrentIndex(mCurveICombo->findText("i.ref", Qt::MatchEndsWith));
    mCurveDCombo->setCurrentIndex(mCurveICombo->findText("d.ref", Qt::MatchEndsWith));
    mCurveFCombo->setCurrentIndex(mCurveICombo->findText("f.ref", Qt::MatchEndsWith));
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    
    grid->addWidget(mIDRadio, 0, 1);
    grid->addWidget(mIFRadio, 1, 1);
    grid->addWidget(mIDFRadio, 2, 1);
    
    grid->addWidget(mILab, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mIEdit, 3, 1);
    
    grid->addWidget(mDLab, 4, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mDEdit, 4, 1);
    
    grid->addWidget(mFLab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mFEdit, 5, 1);
    
    grid->addWidget(mAlpha95Lab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAlpha95Edit, 6, 1);
    
    grid->addWidget(mSigmaFLab, 7, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mSigmaFEdit, 7, 1);
    
    grid->addWidget(mCurveILab, 8, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCurveICombo, 8, 1);
    
    grid->addWidget(mCurveDLab, 9, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCurveDCombo, 9, 1);
    
    grid->addWidget(mCurveFLab, 10, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCurveFCombo, 10, 1);
    
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
    if(mIDRadio->isChecked()) mode = DATE_AM_MODE_ID;
    if(mIFRadio->isChecked()) mode = DATE_AM_MODE_IF;
    if(mIDFRadio->isChecked()) mode = DATE_AM_MODE_IDF;
    
    const double i = locale.toDouble(mIEdit->text());
    const double d = locale.toDouble(mDEdit->text());
    const double f = locale.toDouble(mFEdit->text());
    
    const double alpha95 = locale.toDouble(mAlpha95Edit->text());
    const double sigmaF = locale.toDouble(mSigmaFEdit->text());
    
    const QString curveI = mCurveICombo->currentText();
    const QString curveD = mCurveICombo->currentText();
    const QString curveF = mCurveICombo->currentText();
    
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
    double value = locale.toDouble(str,&ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

bool PluginAMForm::isValid()
{
    const bool isID = mIDRadio->isChecked();
    const bool isIF = mIFRadio->isChecked();
    const bool isIDF = mIDFRadio->isChecked();
    
    const QString refCurveI = mCurveICombo->currentText();
    const QString refCurveD = mCurveDCombo->currentText();
    const QString refCurveF = mCurveFCombo->currentText();
    
    const bool isIValid = !refCurveI.isEmpty();
    const bool isDValid = !refCurveD.isEmpty();
    const bool isFValid = !refCurveF.isEmpty();
    
    bool valid = false;
    
    if(isID) valid = isIValid && isDValid;
    else if(isIF) valid = isIValid && isFValid;
    else if(isIDF) valid = isIValid && isDValid && isFValid;
    
    if(!valid)
        mError = tr("Ref. curve is empty!");
    
    return valid;
}

void PluginAMForm::updateOptions()
{
    bool isID = mIDRadio->isChecked();
    bool isIF = mIFRadio->isChecked();
    bool isIDF = mIDFRadio->isChecked();
    
    mDEdit->setEnabled(isID || isIDF);
    mFEdit->setEnabled(isIF || isIDF);
    mSigmaFEdit->setEnabled(isIF || isIDF);
    mCurveDCombo->setEnabled(isID || isIDF);
    mCurveFCombo->setEnabled(isIF || isIDF);
}

#endif
