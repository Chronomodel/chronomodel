/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "PluginMagForm.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include <QJsonObject>
#include <QtWidgets>


PluginMagForm::PluginMagForm(PluginMag* plugin, QWidget* parent, Qt::WindowFlags flags):PluginFormAbstract(plugin, tr("AM Measurements"), parent, flags)
{

    mIncRadio = new QRadioButton(tr("Inclination (I)"));
    mDecRadio = new QRadioButton(tr("Declination (D)"));
    mFieldRadio = new QRadioButton(tr("Field (F)"));
    mIDRadio = new QRadioButton(tr("Directional (I, D)"));
    mIFRadio = new QRadioButton(tr("Vector (I, F)"));
    mIDFRadio = new QRadioButton(tr("Vector (I, D, F)"));

    connect(mIncRadio, static_cast<void (QRadioButton::*)(bool)> (&QRadioButton::clicked), this, &PluginMagForm:: updateOptions);
    connect(mDecRadio, static_cast<void (QRadioButton::*)(bool)> (&QRadioButton::clicked), this, &PluginMagForm:: updateOptions);
    connect(mFieldRadio, static_cast<void (QRadioButton::*)(bool)> (&QRadioButton::clicked), this, &PluginMagForm:: updateOptions);
    connect(mIDRadio, static_cast<void (QRadioButton::*)(bool)> (&QRadioButton::clicked), this, &PluginMagForm:: updateOptions);
    connect(mIFRadio, static_cast<void (QRadioButton::*)(bool)> (&QRadioButton::clicked), this, &PluginMagForm:: updateOptions);
    connect(mIDFRadio, static_cast<void (QRadioButton::*)(bool)> (&QRadioButton::clicked), this, &PluginMagForm:: updateOptions);

    mIncLab = new QLabel(tr("Inclination"), this);
    mDecLab = new QLabel(tr("Declination"), this);
    mAlpha95Lab = new QLabel(tr("Alpha 95"), this);
    mFieldLab =  new QLabel(tr("Field"), this);
    mFieldErrorLab=  new QLabel(tr("Error F"), this);

    mMCMCIterationLab = new QLabel(tr("Iteration"), this);

    //------------------------------------------------------------------------------------//
    mIncEdit = new QLineEdit(this);
    mIncEdit->setAlignment(Qt::AlignHCenter);
    mIncEdit->setToolTip(tr("inclination is >=-90 and <=90"));
    connect(mIncEdit, &QLineEdit::textChanged, this, &PluginMagForm::incIsValid);

    mDecEdit = new QLineEdit(this);
    mDecEdit->setAlignment(Qt::AlignHCenter);
    mDecEdit->setToolTip(tr("declination is >=-90 and <=270"));
    connect(mDecEdit, &QLineEdit::textChanged, this, &PluginMagForm::decIsValid);

    mAlpha95Edit = new QLineEdit(this);
    mAlpha95Edit->setAlignment(Qt::AlignHCenter);
    mAlpha95Edit->setToolTip(tr("Alpha95 is > 0"));
    connect(mAlpha95Edit, &QLineEdit::textChanged, this, &PluginMagForm::errorIsValid);

    // --------------------------------------------------------------------------------------- //
    mFieldEdit = new QLineEdit(this);
    mFieldEdit ->setAlignment(Qt::AlignHCenter);
    mFieldEdit ->setToolTip(tr("declination is >=-90 and <=270"));
    connect(mFieldEdit, &QLineEdit::textChanged, this, &PluginMagForm::errorIsValid);

    mFieldErrorEdit = new QLineEdit(this);
    mFieldErrorEdit ->setAlignment(Qt::AlignHCenter);
    mFieldErrorEdit ->setToolTip(tr("error >0"));
    connect(mFieldErrorEdit, &QLineEdit::textChanged, this, &PluginMagForm::errorIsValid);

    mMCMCIterationEdit = new QLineEdit(this);
    mMCMCIterationEdit->setAlignment(Qt::AlignHCenter);
    mMCMCIterationEdit->setToolTip(tr("iteration is > 0"));
    connect(mMCMCIterationEdit, &QLineEdit::textChanged, this, &PluginMagForm::errorIsValid);

    mRefILab = new QLabel(tr("Curve I"), this);

    mRefICombo = new QComboBox(this);
    QStringList refICurves = plugin->getRefsNames();
    for (int i = 0; i<refICurves.size(); ++i) {
        if(refICurves[i].contains("_i.ref"))
            mRefICombo->addItem(refICurves[i]);
    }

    mRefDLab = new QLabel(tr("Curve D"), this);
    mRefDCombo = new QComboBox(this);
    QStringList refDCurves = plugin->getRefsNames();
    for (int i = 0; i<refDCurves.size(); ++i) {
        if(refICurves[i].contains("_d.ref"))
            mRefDCombo->addItem(refDCurves[i]);
    }
    mRefFLab = new QLabel(tr("Curve F"), this);
    mRefFCombo = new QComboBox(this);
    QStringList refFCurves = plugin->getRefsNames();
    for (int i = 0; i<refFCurves.size(); ++i) {
        if(refICurves[i].contains("_f.ref"))
            mRefFCombo->addItem(refFCurves[i]);
    }

    mIncEdit->setText("65"); // Inclinaison pour la direction
    mDecEdit->setText("0");    // Déclinaison pour la direction
    mAlpha95Edit->setText("1");

    mFieldEdit ->setText("65"); // Inclinaison pour l'analyse vectorielle
    mFieldErrorEdit ->setText("2");

    mMCMCIterationEdit->setText("500");

    mIDRadio->setChecked(true);

   /* mRefICombo->setCurrentIndex(mRefICombo->findText("_i.ref",Qt::MatchEndsWith));
    mRefDCombo->setCurrentIndex(mRefDCombo->findText("_d.ref",Qt::MatchEndsWith));
    mRefFCombo->setCurrentIndex(mRefFCombo->findText("_f.ref",Qt::MatchEndsWith));
*/
 /*   mIncEdit->setVisible(true);
    mDecEdit->setVisible(true);
    mAlpha95Edit->setVisible(true);

    mFieldEdit ->setVisible(true);
    mFieldErrorEdit->setVisible(true);

    mMCMCIterationEdit->setVisible(true);
    mRefICombo->setVisible(true);
    mRefDCombo->setVisible(true);
    mRefFCombo->setVisible(true);
*/

   QGridLayout* grid = new QGridLayout();
   grid->setContentsMargins(0, 5, 0, 0);

   grid->addWidget(mIncRadio, 0, 1);
   grid->addWidget(mDecRadio, 1, 1);
   grid->addWidget(mFieldRadio, 2, 1);

   grid->addWidget(mIDRadio, 0, 5);
   grid->addWidget(mIFRadio, 1, 5);
   grid->addWidget(mIDFRadio, 2, 5);

   grid->addWidget(mIncLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mIncEdit, 6, 1);
   grid->addWidget(mRefILab, 6, 4, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mRefICombo, 6, 5);


   grid->addWidget(mDecLab, 7, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mDecEdit, 7, 1);
   grid->addWidget(mRefDLab, 7, 4, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mRefDCombo, 7, 5);

   grid->addWidget(mFieldLab, 8, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mFieldEdit, 8, 1);
   grid->addWidget(mRefFLab, 8, 4, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mRefFCombo, 8, 5);

   grid->addWidget(mAlpha95Lab, 9, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mAlpha95Edit, 9, 1);

   grid->addWidget(mFieldErrorLab, 10, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mFieldErrorEdit, 10, 1);


   grid->addWidget(mMCMCIterationLab, 12, 0, Qt::AlignRight | Qt::AlignVCenter);
   grid->addWidget(mMCMCIterationEdit, 12, 1);
   setLayout(grid);

   updateOptions();
}

PluginMagForm::~PluginMagForm()
{

}

void PluginMagForm::setData(const QJsonObject& data, bool isCombined)
{
    if (isCombined)
        return;

    ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    switch (pta) {
    case eInc:
        mIncRadio->setChecked(true);
        break;
    case eDec:
        mDecRadio->setChecked(true);
        break;
    case eField:
        mFieldRadio->setChecked(true);
        break;
    case eID:
        mIDRadio->setChecked(true);
        break;
    case eIF:
        mIFRadio->setChecked(true);
        break;
    case eIDF:
        mIDFRadio->setChecked(true);
        break;
    default:
        break;
    }

    const double incl = data.value(DATE_AM_INC_STR).toDouble();
    const double decl = data.value(DATE_AM_DEC_STR).toDouble();
    const double alpha95 = data.value(DATE_AM_ALPHA95_STR).toDouble();
    const double field = data.value(DATE_AM_FIELD_STR).toDouble();
    const double error_f = data.value(DATE_AM_ERROR_F_STR).toDouble();
    const int iter = data.value(DATE_AM_ITERATION_STR).toInt();

    QLocale locale  =QLocale();
    mIncEdit->setText(locale.toString(incl)); // Inclinaison pour la direction
    mDecEdit->setText(locale.toString(decl));    // Déclinaison pour la direction
    mAlpha95Edit->setText(locale.toString(alpha95));

    mFieldEdit ->setText(locale.toString(field));
    mFieldErrorEdit ->setText(locale.toString(error_f));

    mMCMCIterationEdit->setText(locale.toString(iter));

    mRefICombo->setCurrentText(data.value(DATE_AM_REF_CURVEI_STR).toString().toLower());
    mRefDCombo->setCurrentText(data.value(DATE_AM_REF_CURVED_STR).toString().toLower());
    mRefFCombo->setCurrentText(data.value(DATE_AM_REF_CURVEF_STR).toString().toLower());

    updateOptions();
    emit PluginFormAbstract::OkEnabled(true );
}

QJsonObject PluginMagForm::getData()
{
    QJsonObject data;
    const QLocale locale = QLocale();

    ProcessTypeAM pta = ProcessTypeAM::eNone;
    if (mIncRadio->isChecked())
        pta = eInc;
    else if (mDecRadio->isChecked())
        pta = eDec;
    else if (mFieldRadio->isChecked())
        pta = eField;
    else if (mIDRadio->isChecked())
        pta = eID;
    else if (mIFRadio->isChecked())
        pta = eIF;
    else if (mIDFRadio->isChecked())
        pta = eIDF;

    const double inc = locale.toDouble(mIncEdit->text());
    const double dec = locale.toDouble(mDecEdit->text());
    const double alpha95 = locale.toDouble(mAlpha95Edit->text());
    const double field = locale.toDouble(mFieldEdit->text());
    const double error_f = locale.toDouble(mFieldErrorEdit->text());

    const double iteration_mcmc = locale.toDouble(mMCMCIterationEdit->text());
    QString refI_curve = mRefICombo->currentText();
    QString refD_curve = mRefDCombo->currentText();
    QString refF_curve = mRefFCombo->currentText();

    data.insert(DATE_AM_PROCESS_TYPE_STR, pta);

    data.insert(DATE_AM_INC_STR, inc);
    data.insert(DATE_AM_DEC_STR, dec);
    data.insert(DATE_AM_ALPHA95_STR, alpha95);

    data.insert(DATE_AM_FIELD_STR, field);
    data.insert(DATE_AM_ERROR_F_STR, error_f);

    data.insert(DATE_AM_ITERATION_STR, iteration_mcmc);
    data.insert(DATE_AM_REF_CURVEI_STR, refI_curve);
    data.insert(DATE_AM_REF_CURVED_STR, refD_curve);
    data.insert(DATE_AM_REF_CURVEF_STR, refF_curve);

    return data;
}

void PluginMagForm::errorIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

/*
void PluginMagForm::errorIsValididf(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

void PluginMagForm::errorIsValidif(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

void PluginMagForm::errorIsValidii(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

void PluginMagForm::errorIsValidff(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}

void PluginMagForm::iterationValid(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>0) );
}
*/
void PluginMagForm::incIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>=-90) && (value<=90) );
}

void PluginMagForm::decIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double value = locale.toDouble(str, &ok);

    emit PluginFormAbstract::OkEnabled(ok && (value>=-90) && (value<=270) );
}


bool PluginMagForm::isValid()
{
 /*   const ProcessTypeAM pta = static_cast<ProcessTypeAM> (date.mData.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    QString ref_curve;
    switch (pta) {
    case eInc:
        ref_curve = date.mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        break;
    case eDec:
        ref_curve = date.mData.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        break;
    case eField:
        ref_curve = date.mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        break;
    case eID:
        mIDRadio->setChecked(true);
        break;
    case eIF:
        mIFRadio->setChecked(true);
        break;
    case eIDF:
        mIDFRadio->setChecked(true);
        break;

    default:
        break;
    }

    if (refCurve.isEmpty())
        mError = tr("Ref. curve is empty!");
    return !refCurve.isEmpty();
*/
return true;
}

/*
bool PluginMagForm::isValidD()
{
    QString refCurve = mRefDCombo->currentText();
    if (refCurve.isEmpty())
        mError = tr("Ref. curve is empty!");
    return !refCurve.isEmpty();
}

bool PluginMagForm::isValidF()
{
    QString refCurve = mRefFCombo->currentText();
    if (refCurve.isEmpty())
        mError = tr("Ref. curve is empty!");
    return !refCurve.isEmpty();
}
*/

void PluginMagForm::updateOptions()
{
   /* mIncEdit->setVisible(true);
    mDecEdit->setVisible(true);
    mAlpha95Edit->setVisible(true);

    mFieldEdit ->setVisible(true);
    mFieldErrorEdit ->setVisible(true);*/


   /* mRefICombo->setVisible(true);
    mRefDCombo->setVisible(true);
    mRefFCombo->setVisible(true);*/

    const bool showInc = mIncRadio->isChecked() || mIDRadio->isChecked() ||
            mIFRadio->isChecked() || mIDFRadio->isChecked();
    mIncLab->setVisible(showInc);
    mIncEdit->setVisible(showInc);

    mRefILab->setVisible(showInc);
    mRefICombo->setVisible(showInc);

    const bool showDec = mDecRadio->isChecked() || mIDRadio->isChecked() || mIDFRadio->isChecked();
    mDecLab->setVisible(showDec);
    mDecEdit->setVisible(showDec);

    mRefDLab->setVisible(showDec);
    mRefDCombo->setVisible(showDec);

    const bool showAlpha95 = !mFieldRadio->isChecked();
    mAlpha95Lab->setVisible(showAlpha95);
    mAlpha95Edit->setVisible(showAlpha95);

    const bool showField = mFieldRadio->isChecked() || mIFRadio->isChecked() || mIDFRadio->isChecked();
    mFieldLab->setVisible(showField);
    mFieldEdit->setVisible(showField);

    mRefFLab->setVisible(showField);
    mRefFCombo->setVisible(showField);

    mFieldErrorLab->setVisible(showField);
    mFieldErrorEdit->setVisible(showField);

    const bool showIter = mIDRadio->isChecked() || mIFRadio->isChecked() || mIDFRadio->isChecked();
    mMCMCIterationLab->setVisible(showIter);
    mMCMCIterationEdit->setVisible(showIter);

}

#endif