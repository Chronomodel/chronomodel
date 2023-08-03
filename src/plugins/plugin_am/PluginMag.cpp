/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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

#include "PluginMag.h"
#if USE_PLUGIN_AM

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "PluginMagForm.h"
#include "PluginMagRefView.h"
#include "PluginMagSettingsView.h"
#include "Generator.h"

#include <QJsonObject>
#include <QtWidgets>
#include <QMessageBox>
#include <QVector>

#include <cstdlib>
#include <iostream>

PluginMag::PluginMag()
{
    mColor = QColor(198, 79, 32);
    loadRefDatas();
}

PluginMag::~PluginMag()
{
    if (mRefGraph)
        delete mRefGraph;
}


// Likelihood
long double PluginMag::getLikelihood(const double& t, const QJsonObject& data)
{
    return Likelihood(t, data);
}

QPair<long double, long double> PluginMag::getLikelihoodArg(const double& t, const QJsonObject& data)
{
    (void) data;
    return qMakePair<long double, long double> (0, 0);
}

long double PluginMag::Likelihood(const double& t, const QJsonObject& data)
{
    // Lecture des données

    const long double alpha95 = static_cast<long double> (data.value(DATE_AM_ALPHA95_STR).toDouble());
    const long double incl = static_cast<long double> (data.value(DATE_AM_INC_STR).toDouble());
    const long double decl = static_cast<long double> (data.value(DATE_AM_DEC_STR).toDouble());
    const long double field = static_cast<long double> (data.value(DATE_AM_FIELD_STR).toDouble());
    const long double error_f = static_cast<long double> (data.value(DATE_AM_ERROR_F_STR).toDouble());

    const double iteration_mcmc = data.value(DATE_AM_ITERATION_STR).toDouble();

    //QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();

    // La partie des calculs de l'étalonnage
    long double mesureIncl (0.l);
    long double mesureDecl (0.l);
    long double mesureField (0.l);
    long double errorD (0.l);
    long double errorI (0.l);
    long double errorF (0.l);
    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    switch (pta) {
    case eInc:
        mesureIncl = incl;
        errorI = alpha95 / 2.448l;
        break;
    case eDec:
        mesureIncl = incl;
        mesureDecl = decl;
        errorD = alpha95 / (2.448l * cosl(incl * M_PIl / 180.l));
        break;
    case eField:
        mesureField = field;
        errorF = error_f;
        break;
    case eID:
        mesureIncl = incl;
        errorI = alpha95 / 2.448l;
        mesureDecl = decl;
        errorD = alpha95 / (2.448l * cosl(incl * M_PIl / 180.l));

        break;
    case eIF:
        mesureIncl = incl;
        errorI = alpha95 / 2.448l;
        mesureField = field;
        errorF = error_f;
        break;
    case eIDF:
        mesureIncl = incl;
        errorI = alpha95 / 2.448l;
        mesureDecl = decl;
        errorD = alpha95 / (2.448l * cosl(incl * M_PIl / 180.l));
        mesureField = field;
        errorF = error_f;
        break;
    default:
        break;
    }



    // Lecture des diférentes courbes d'étalonnage
    const QString refI_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
    const QString refD_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
    const QString refF_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();

    // Déclaration des variables
    long double vI (.0l);
    long double I (.0l);
    long double vD (.0l);
    long double D (.0l);
    long double vF (.0l);
    long double F (.0l);
    long double exponent  (0.l);

    long double refValueI (0.l);
    long double refErrorI (0.l);
    long double refValueD (0.l);
    long double refErrorD (0.l);
    long double refValueF (0.l);
    long double refErrorF (0.l);

    if (pta == eInc){
        // Étalonnage  I
        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI;
        I = -0.5l*powl((mesureIncl - refValueI), 2.l);

        exponent += expl(I/vI )/sqrtl(vI);

    }
    else if (pta == eDec){
        // Étalonnage D

        refValueD = getRefCurveValueAt(refD_curve, t);
        refErrorD = getRefCurveErrorAt(refD_curve, t);

        vD = errorD*errorD + refErrorD*refErrorD ;
        D = -0.5l*powl((mesureDecl - refValueD), 2.l);


        exponent += expl(D/vD )/sqrtl(vD);

    }
    else if (pta == eField){
        // Étalonnage directionnel : I et D
        refValueF = getRefCurveValueAt(refF_curve, t);
        refErrorF = getRefCurveErrorAt(refF_curve, t);

        vF = errorF*errorF + refErrorF*refErrorF ;
        F = -0.5l*powl((mesureField - refValueF), 2.l);

        exponent += expl(F/vF )/sqrtl(vF);
    }
    else if(pta == eID){
        // Étalonnage directionnel : I et D
        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);
        refValueD = getRefCurveValueAt(refD_curve, t);
        refErrorD = getRefCurveErrorAt(refD_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI;
        I = -0.5l*powl((mesureIncl - refValueI), 2.l);
        vD = errorD*errorD + refErrorD*refErrorD ;
        D = -0.5l*powl((mesureDecl - refValueD), 2.l);

        // Simulation des valeurs de sigmaID
        QVector <double> sigmaID(iteration_mcmc);
        double w = 0;
       for(int i=0; i < sigmaID.size(); ++i) {
               w += 1/iteration_mcmc;
             sigmaID[i] = errorI*errorI*((1. - w)/w);
         }

        long double rr = powl((cosl(mesureIncl*M_PIl/180.l)),2.l);
        for(int j = 0; j < sigmaID.size(); ++j){
          exponent += expl(I/(vI + sigmaID[j]) + D/(vD + sigmaID[j]/rr))/sqrtl((vI + sigmaID[j])*(vD + sigmaID[j]/rr));
        }
    }
    else if(pta == eIF){
        // Étalonnage vectoriel : I et F
        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);
        refValueF = getRefCurveValueAt(refF_curve, t);
        refErrorF = getRefCurveErrorAt(refF_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI;
        I = -0.5l*powl((mesureIncl - refValueI), 2.l);
        vF = errorF*errorF + refErrorF*refErrorF ;
        F = -0.5l*powl((mesureField - refValueF), 2.l);
        // Calcul de s0IF
        long double s0IF = 0.5*(errorF*errorF + powl(errorI*mesureField*M_PIl/180.l,2.l));
        // Simulation des valeurs de sigmaIF
        QVector <double> sigmaIF(iteration_mcmc);
        double w =0;
       for(int i=0; i < sigmaIF.size(); ++i) {
              w += 1/iteration_mcmc;
             sigmaIF[i] = s0IF*((1. - w)/w);
         }
       for(int j = 0; j < sigmaIF.size(); ++j){
           exponent += expl(I/(vI + sigmaIF[j]/(mesureField*mesureField)) + F/(vF + sigmaIF[j]))/sqrtl((vI + sigmaIF[j]/(mesureField*mesureField))*(vF + sigmaIF[j]));
       }
    }
    else if(pta == eIDF) {
        // Étalonnage vectoriel : I, D et F
        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);
        refValueD = getRefCurveValueAt(refD_curve, t);
        refErrorD = getRefCurveErrorAt(refD_curve, t);
        refValueF = getRefCurveValueAt(refF_curve, t);
        refErrorF = getRefCurveErrorAt(refF_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI ;
        I = -0.5l*powl((mesureIncl - refValueI), 2.l);
        vD = errorD*errorD + refErrorD*refErrorD ;
        D = -0.5l*powl((mesureDecl - refValueD), 2.l);
        vF = errorF*errorF + refErrorF*refErrorF ;
        F = -0.5l*powl((mesureField - refValueF), 2.l);

        long double rr = powl((mesureField * cosl(mesureIncl * M_PIl / 180.l)), 2);
         // calcul de s0IDF
        long double s0IDF = (errorF*errorF + 2.l*powl((errorI*mesureField*M_PIl/180.l), 2))/3;
        // Simulation des valeurs de sigmaIDF
        QVector <long double> sigmaIDF(iteration_mcmc);
        double w = 0;
       for (int i=0; i < sigmaIDF.size(); ++i) {
             w += 1/iteration_mcmc;
             sigmaIDF[i] = s0IDF*((1. - w)/w);
         }
       for (int j = 0; j < sigmaIDF.size(); ++j){
           exponent += expl(I/(vI + sigmaIDF[j]/(mesureField*mesureField)) + D/(vD + sigmaIDF[j]/rr)  + F/(vF + sigmaIDF[j]))/sqrtl((vI + sigmaIDF[j]/(mesureField*mesureField))*(vF + sigmaIDF[j])*(vD + sigmaIDF[j]/rr));
       }
    }

    return exponent;
}

// Properties
QString PluginMag::getName() const
{
   return QString("AM");
}

QIcon PluginMag::getIcon() const
{
    return QIcon(":/AM_w.png");
}

bool PluginMag::doesCalibration() const
{
    return true;
}

bool PluginMag::wiggleAllowed() const
{
    return false;
}

MHVariable::SamplerProposal PluginMag::getDataMethod() const
{
    return MHVariable::eInversion;
}

QList<MHVariable::SamplerProposal> PluginMag::allowedDataMethods() const
{
    QList<MHVariable::SamplerProposal> methods;
    methods.append(MHVariable::eMHSymetric);
    methods.append(MHVariable::eInversion);
    methods.append(MHVariable::eMHSymGaussAdapt);
    return methods;
}

QString PluginMag::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QLocale locale = QLocale();
    QString result;
    if (date->mOrigin == Date::eSingleDate) {

        const QJsonObject data = date->mData;

        const double alpha95 = data.value(DATE_AM_ALPHA95_STR).toDouble();
        const double incl = data.value(DATE_AM_INC_STR).toDouble();
        const double decl = data.value(DATE_AM_DEC_STR).toDouble();
        const double field = data.value(DATE_AM_FIELD_STR).toDouble();
        const double error_f = data.value(DATE_AM_ERROR_F_STR).toDouble();
        const double iteration_mcmc = data.value(DATE_AM_ITERATION_STR).toDouble();

        const ProcessTypeAM pta = static_cast<ProcessTypeAM> (date->mData.value(DATE_AM_PROCESS_TYPE_STR).toInt());

        QString ref_curve;
        switch (pta) {
        case eInc:
            ref_curve = date->mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
            break;
        case eDec:
            ref_curve = date->mData.value(DATE_AM_REF_CURVED_STR).toString().toLower();
            break;
        case eField:
            ref_curve = date->mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
            break;
      /*  case eID:
            mIDRadio->setChecked(true);
            break;
        case eIF:
            mIFRadio->setChecked(true);
            break;
        case eIDF:
            mIDFRadio->setChecked(true);
            break;
            */
        default:
            break;
        }

        switch (pta) {
        case eInc:
            result += QObject::tr("Inclination : %1").arg(locale.toString(incl));
                        // this is the html form, but not reconized in the DatesListItemDelegate
                       // result += "; " + QString("α<SUB>95</SUB>") + " : " + locale.toString(alpha);
            result += " : " + QObject::tr("α 95 = %1").arg(locale.toString(alpha95));
            if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
                result += " : " + tr("Ref. curve = %1").arg(ref_curve);
            else
                result += " = " + tr("ERROR -> Ref. curve : %1").arg(ref_curve);
            break;

        case eDec:
            result += QObject::tr("Declination : %1").arg(locale.toString(decl));
            result += " : " + QObject::tr("Inclination = %1").arg(locale.toString(incl));
            result += " : " + QObject::tr("α 95 = %1").arg(locale.toString(alpha95));
            if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
                result += " : " + tr("Ref. curve = %1").arg(ref_curve);
            else
                result += " = " + tr("ERROR -> Ref. curve : %1").arg(ref_curve);
            break;

        case eField:
            result += QObject::tr("Field = %1").arg(locale.toString(field));
            result += " : " + QObject::tr("Error = %1").arg(locale.toString(error_f));
            if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
                result += " : " + tr("Ref. curve = %1").arg(ref_curve);
            else
                result += " = " + tr("ERROR -> Ref. curve : %1").arg(ref_curve);
            break;

        case eID:
            result += "MCMC ID = (" + QObject::tr(" %1").arg(locale.toString(incl)) ;
             result += " ;" + QObject::tr(" %1").arg(locale.toString(decl)) + ")";
             result += " ; " + QObject::tr("Alpha95 = %1").arg(locale.toString(alpha95));

             if (tr("%1").arg(ref_curve)==tr("bulgaria_i.ref")) {
                 result += " ; Curve =" + tr("Bulgaria");
             } else {
                 result += " ; Curve = " + tr("GAL2002sph2014");
             }
             result += " ; " + QObject::tr("Iteration = %1").arg(locale.toString(iteration_mcmc));
            break;

        case eIF:
            result += "MCMC IF = (" + QObject::tr(" %1").arg(locale.toString(incl));
            result += ";" + QObject::tr(" %1").arg(locale.toString(field)) + ")";
            result += " ; (ErrorF, Alpha95) =  (" + QObject::tr(" %1").arg(locale.toString(error_f));
            result += " ; " + QObject::tr(" %1").arg(locale.toString(alpha95)) + ")";
            if (tr("%1").arg(ref_curve)==tr("bulgaria_i.ref")){
                result += " ; Curve =" + tr("Bulgaria");

            } else {
                result += " ; Curve = " + tr("GAL2002sph2014");
            }
           result += " ; " + QObject::tr("Iteration = %1").arg(locale.toString(iteration_mcmc));
            break;

        case eIDF:
            result += "MCMC IDF = (" + QObject::tr(" %1").arg(locale.toString(incl));
            result += " ;" + QObject::tr(" %1").arg(locale.toString(decl));
            result += " ;" + QObject::tr(" %1").arg(locale.toString(field)) + ")";
            result += " ; (ErrorF; alpha95) = (" + QObject::tr(" %1").arg(locale.toString(error_f));
            result += " ;" + QObject::tr(" %1").arg(locale.toString(alpha95)) + ")";
            if (tr("%1").arg(ref_curve)==tr("bulgaria_i.ref")) {
                result += " ; Curve =" + tr("Bulgaria");

            } else {
                result += " ; Curve = " + tr("GAL2002sph2014");
            }
            result += " ; " + QObject::tr("Iteration = %1").arg(locale.toString(iteration_mcmc));
            break;

        default:
            break;
        }



    } else {
            result = "Combine (";
            QStringList datesDesc;

            for (auto&& d: date->mSubDates) {
                Date subDate (d.toObject() );
                datesDesc.append(getDateDesc(&subDate));
            }
            result += datesDesc.join(" | ") + " )";
    }

    return result;
}

QString PluginMag::getDateRefCurveName(const Date* date)
{
    Q_ASSERT(date);
    const QJsonObject data = date->mData;
    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (date->mData.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    QString ref_curve;
    switch (pta) {
    case eInc:
        ref_curve = date->mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        break;
    case eDec:
        ref_curve = date->mData.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        break;
    case eField:
        ref_curve = date->mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        break;
  /*  case eID:
        mIDRadio->setChecked(true);
        break;
    case eIF:
        mIFRadio->setChecked(true);
        break;
    case eIDF:
        mIDFRadio->setChecked(true);
        break;
        */
    default:
        ref_curve = "";
        break;
    }
   return  ref_curve;
}




// CSV
QStringList PluginMag::csvColumns() const
{
    QStringList cols;
    cols << "Data Name"
        << "type"
        << "Inclination value"
        << "Declination value"
        << "Intensity value"
        << "Error (sd) or α 95"
        << "Ref. curve";
    return cols;
}


PluginFormAbstract* PluginMag::getForm()
{
    PluginMagForm* form = new PluginMagForm(this);
    return form;
}
//Convert old project versions
QJsonObject PluginMag::checkValuesCompatibility(const QJsonObject& values)
{
    QJsonObject result = values;

    //force type double
    result[DATE_AM_INC_STR] = result.value(DATE_AM_INC_STR).toDouble();
    result[DATE_AM_DEC_STR] = result.value(DATE_AM_DEC_STR).toDouble();
    result[DATE_AM_ALPHA95_STR] = result.value(DATE_AM_ALPHA95_STR).toDouble();
    result[DATE_AM_FIELD_STR] = result.value(DATE_AM_FIELD_STR).toDouble();
    result[DATE_AM_ERROR_F_STR] = result.value(DATE_AM_ERROR_F_STR).toDouble();

    result[DATE_AM_REF_CURVEI_STR] = result.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
    result[DATE_AM_REF_CURVED_STR] = result.value(DATE_AM_REF_CURVED_STR).toString().toLower();
    result[DATE_AM_REF_CURVED_STR] = result.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
    return result;
}

QJsonObject PluginMag::fromCSV(const QStringList& list,const QLocale &csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        
// checks the validity interval
        const QString name = list.at(0);
        const double valInc (csvLocale.toDouble(list.at(2)));
        const double valDec (csvLocale.toDouble(list.at(3)));
        const double valAlpha95 (csvLocale.toDouble(list.at(4)));

        const double valIntensity (csvLocale.toDouble(list.at(5)));
        const double valError_f = csvLocale.toDouble(list.at(6));
         
        if ((list.at(1) == "inclination" || list.at(1) == "declination") && (valInc>90 || valInc < -90)) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for Field"), tr(" %1 : Inclination must be >=-90 and <=90").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;

        } else  if (list.at(1) == "declination" && (valDec>270 || valDec < -90) ) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for declination"), tr(" %1 : Declination must be >=-90 and <=270").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;

        } else if (list.at(1) == "intensity" && valIntensity <=0 ) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for Intensity"), tr(" %1 : Intensity must be >0").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;

        } else if ( valError_f <=0 ) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for error"), tr(" %1 : Alpha95 must be >0").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;
        }

        ProcessTypeAM pta = eNone;

        const QString typeStr = list.at(1);

        if (typeStr == "inclination") {
            pta = eInc;

        } else        if (typeStr == "declination") {
            pta = eDec;

        } else         if (typeStr == "field" || typeStr=="intensity") {
            pta = eField;

        } else         if (typeStr == "incl-decl") {
            pta = eID;

        } else         if (typeStr == "incl-field") {
            pta = eIF;

        } else         if (typeStr == "incl-decl-field") {
            pta = eIDF;
        }

        json.insert(DATE_AM_PROCESS_TYPE_STR, pta);
        json.insert(DATE_AM_INC_STR, valInc);
        json.insert(DATE_AM_DEC_STR, valDec);
        json.insert(DATE_AM_ALPHA95_STR, valAlpha95);

        json.insert(DATE_AM_FIELD_STR, valIntensity);
        json.insert(DATE_AM_ERROR_F_STR, valError_f);

        json.insert(DATE_AM_REF_CURVEI_STR, list.at(7).toLower());
        json.insert(DATE_AM_REF_CURVEI_STR, list.at(8).toLower());
        json.insert(DATE_AM_REF_CURVEI_STR, list.at(9).toLower());


    }
    return json;
}

QStringList PluginMag::toCSV(const QJsonObject& data, const QLocale& csvLocale) const
{
    QStringList list;

    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    switch (pta) {
    case eInc:
        list << "inclination";
        break;
    case eDec:
        list << "declination";
        break;
    case eField:
        list << "field";
        break;
    case eID:
        list << "incl-decl";
        break;
    case eIF:
        list << "incl-field";
        break;
    case eIDF:
        list << "incl-decl-field";
        break;

    default:
        break;
    }

    list << csvLocale.toString(data.value(DATE_AM_INC_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_DEC_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_ALPHA95_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_FIELD_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_ERROR_F_STR).toDouble());

    list << data.value(DATE_AM_REF_CURVEI_STR).toString();
    list << data.value(DATE_AM_REF_CURVED_STR).toString();
    list << data.value(DATE_AM_REF_CURVEF_STR).toString();
    return list;
}

//Reference curves (files)
QString PluginMag::getRefExt() const
{
    return "ref";
}

QString PluginMag::getRefsPath() const
{
    return AppPluginLibrary() + "/Calib/AM";
}

RefCurve PluginMag::loadRefFile(QFileInfo refFile)
{
    RefCurve curve;
    curve.mName = refFile.fileName().toLower();

    QFile file(refFile.absoluteFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {

        QLocale locale = QLocale(QLocale::English);
        QTextStream stream(&file);
        bool firstLine = true;

        while (!stream.atEnd()) {
            QString line = stream.readLine();

            if (line.contains("reference", Qt::CaseInsensitive) && line.contains("point", Qt::CaseInsensitive)) {
                // TODO : start loading points
                break;
            }
            bool ok;
            if (!isComment(line)) {
                QStringList values = line.split(",");
                if (values.size() >= 3) {
                    const int t = locale.toInt(values.at(0),&ok);

                    const double g = locale.toDouble(values.at(1), &ok);
                    if (!ok)
                        continue;

                    const double e = locale.toDouble(values.at(2), &ok);
                    if(!ok)
                        continue;

                    const double gSup = g + 1.96 * e;

                    const double gInf = g - 1.96 * e;


                    curve.mDataMean[t] = g;
                    curve.mDataError[t] = e;
                    curve.mDataSup[t] = gSup;
                    curve.mDataInf[t] = gInf;

                    if (firstLine) {
                        curve.mDataMeanMin = g;
                        curve.mDataMeanMax = g;

                        curve.mDataErrorMin = e;
                        curve.mDataErrorMax = e;

                        curve.mDataSupMin = gSup;
                        curve.mDataSupMax = gSup;

                        curve.mDataInfMin = gInf;
                        curve.mDataInfMax = gInf;

                    } else {
                        curve.mDataMeanMin = qMin(curve.mDataMeanMin, g);
                        curve.mDataMeanMax = qMax(curve.mDataMeanMax, g);

                        curve.mDataErrorMin = qMin(curve.mDataErrorMin, e);
                        curve.mDataErrorMax = qMax(curve.mDataErrorMax, e);

                        curve.mDataSupMin = qMin(curve.mDataSupMin, gSup);
                        curve.mDataSupMax = qMax(curve.mDataSupMax, gSup);

                        curve.mDataInfMin = qMin(curve.mDataInfMin, gInf);
                        curve.mDataInfMax = qMax(curve.mDataInfMax, gInf);
                    }
                    firstLine = false;
                }
            }
        }
        file.close();

        // invalid file ?
        if (!curve.mDataMean.isEmpty()) {
            curve.mTmin = curve.mDataMean.firstKey();
            curve.mTmax = curve.mDataMean.lastKey();
        }
    }
    return curve;
}

//Reference Values & Errors
double PluginMag::getRefValueAt(const QJsonObject& data, const double& t)
{
    QString ref_curve;
    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    switch (pta) {
    case eInc:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        break;
    case eDec:
        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        break;
    case eField:
        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        break;
  /*  case eID:
        mIDRadio->setChecked(true);
        break;
    case eIF:
        mIFRadio->setChecked(true);
        break;
    case eIDF:
        mIDFRadio->setChecked(true);
        break;
        */
    default:
        break;
    }
    return getRefCurveValueAt(ref_curve, t);
}

double PluginMag::getRefErrorAt(const QJsonObject& data, const double& t)
{
    QString ref_curve;
    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    switch (pta) {
    case eInc:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        break;
    case eDec:
        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        break;
    case eField:
        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        break;
  /*  case eID:
        mIDRadio->setChecked(true);
        break;
    case eIF:
        mIFRadio->setChecked(true);
        break;
    case eIDF:
        mIDFRadio->setChecked(true);
        break;
        */
    default:
        ref_curve = "";
        break;
    }
    return getRefCurveErrorAt(ref_curve, t);
}

QPair<double,double> PluginMag::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    double tmin = 0.;
    double tmax = 0.;
    QString ref_curve;
    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    switch (pta) {
    case eInc:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = mRefCurves.value(ref_curve).mTmin;
            tmax = mRefCurves.value(ref_curve).mTmax;
        }
        break;
    case eDec:
        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = mRefCurves.value(ref_curve).mTmin;
            tmax = mRefCurves.value(ref_curve).mTmax;
        }
        break;
    case eField:
        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = mRefCurves.value(ref_curve).mTmin;
            tmax = mRefCurves.value(ref_curve).mTmax;
        }
        break;
    case eID:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = mRefCurves.value(ref_curve).mTmin;
            tmax = mRefCurves.value(ref_curve).mTmax;
        }
        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = std::max(tmin, mRefCurves.value(ref_curve).mTmin);
            tmax = std::min(tmax, mRefCurves.value(ref_curve).mTmax);
        }
        break;
    case eIF:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = mRefCurves.value(ref_curve).mTmin;
            tmax = mRefCurves.value(ref_curve).mTmax;
        }
        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = std::max(tmin, mRefCurves.value(ref_curve).mTmin);
            tmax = std::min(tmax, mRefCurves.value(ref_curve).mTmax);
        }
        break;
    case eIDF:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = mRefCurves.value(ref_curve).mTmin;
            tmax = mRefCurves.value(ref_curve).mTmax;
        }
        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = std::max(tmin, mRefCurves.value(ref_curve).mTmin);
            tmax = std::min(tmax, mRefCurves.value(ref_curve).mTmax);
        }
        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
            tmin = std::max(tmin, mRefCurves.value(ref_curve).mTmin);
            tmax = std::min(tmax, mRefCurves.value(ref_curve).mTmax);
        }
        break;

    default:
        tmin = 0;
        tmax = 0;
        break;
    }
    //QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();


    return QPair<double,double>(tmin, tmax);
}

//Settings / Input Form / RefView
GraphViewRefAbstract* PluginMag::getGraphViewRef()
{
   mRefGraph = new PluginMagRefView();
   return mRefGraph;
}

void PluginMag::deleteGraphViewRef(GraphViewRefAbstract* graph)
{
    if (graph)
        delete static_cast<PluginMagRefView*>(graph);

    graph = nullptr;
    mRefGraph = nullptr;
}

PluginSettingsViewAbstract* PluginMag::getSettingsView()
{
    return new PluginMagSettingsView(this);
}

//Date validity
bool PluginMag::isDateValid(const QJsonObject& data, const StudyPeriodSettings& settings)
{
    qDebug() <<"PluginMag::isDateValid for="<< data.value(STATE_NAME).toString();
    // check valid curve
    /*QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    const bool is_vec = data.value(DATE_AM_IS_VEC_STR).toBool();
    const bool is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
    const bool is_int = data.value(DATE_AM_IS_INT_STR).toBool(); */

    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());
    double incl = data.value(DATE_AM_INC_STR).toDouble();
    double decl = data.value(DATE_AM_DEC_STR).toDouble();
    double field = data.value(DATE_AM_FIELD_STR).toDouble();
    bool valid = false;

    QString ref_curve;
    switch (pta) {
    case eInc:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        valid = measureIsValidForCurve(incl, ref_curve, data, settings);
        break;
    case eDec:
        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        valid = measureIsValidForCurve(decl, ref_curve, data, settings);
        break;

    case eField:
        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        valid = measureIsValidForCurve(field, ref_curve, data, settings);
        break;

    case eID:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        valid = measureIsValidForCurve(incl, ref_curve, data, settings);

        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        valid = measureIsValidForCurve(decl, ref_curve, data, settings) && valid;

        break;
    case eIF:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        valid = measureIsValidForCurve(incl, ref_curve, data, settings);

        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        valid = measureIsValidForCurve(field, ref_curve, data, settings) && valid;

        return valid;
        break;
    case eIDF:
        ref_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        valid = measureIsValidForCurve(incl, ref_curve, data, settings);

        ref_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        valid = measureIsValidForCurve(decl, ref_curve, data, settings) && valid;

        ref_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        valid = measureIsValidForCurve(field, ref_curve, data, settings) && valid;

        break;

    default:
        break;
    }


return valid;

}

bool PluginMag::measureIsValidForCurve(const double m, const QString& ref, const QJsonObject& data, const StudyPeriodSettings& settings)
{
    const RefCurve& curve = mRefCurves.value(ref);
    bool valid = false;
   // qDebug()<<"in plugmag refcurve"<<ref_curve<< curve.mDataInf<<curve.mTmin;

    if (m > curve.mDataInfMin && m < curve.mDataSupMax)
        valid = true;

    else {
        double t = curve.mTmin;
        long double repartition (0.l);
        long double v (0.l);
        long double lastV (0.l);
        while (valid == false && t <= curve.mTmax) {
            v = static_cast<long double> (getLikelihood(t, data));
            // we have to check this calculs
            //because the repartition can be smaller than the calibration
            if (lastV>0.l && v>0.l)
                repartition += static_cast<long double> (settings.mStep) * (lastV + v) / 2.l;

            lastV = v;

            valid = ( repartition > 0.l);
            t += settings.mStep;
        }
    }

    return valid;
}



// Combine / Split
bool PluginMag::areDatesMergeable(const QJsonArray& )
{
   return true;
}
/**
 * @brief Combine several Mag datation
 **/
QJsonObject PluginMag::mergeDates(const QJsonArray& dates)
{
    QJsonObject result;
    if (dates.size() > 1) {
       
        QJsonObject mergedData;
        
        mergedData[DATE_AM_REF_CURVEF_STR] = "|mag|.ref";
        mergedData[DATE_AM_PROCESS_TYPE_STR] = eCombine;
        /*
         * mergedData[DATE_AM_IS_VEC_STR] = false;
        mergedData[DATE_AM_IS_DEC_STR] = false;
        mergedData[DATE_AM_IS_INT_STR] = true; */
        
        QStringList names;

        bool subDatIsValid (true);
        for (auto&& d : dates) {
            names.append(d.toObject().value(STATE_NAME).toString());
            // Validate the date before merge
            subDatIsValid = d.toObject().value(STATE_DATE_VALID).toBool() & subDatIsValid;
        }

        if (subDatIsValid) {
            // inherits the first data propeties as plug-in and method...
            result = dates.at(0).toObject();
            result[STATE_NAME] =  names.join(" | ") ;
            result[STATE_DATE_UUID] = QString::fromStdString( Generator::UUID());
            result[STATE_DATE_DATA] = mergedData;
            result[STATE_DATE_ORIGIN] = Date::eCombination;
            result[STATE_DATE_SUB_DATES] = dates;
            result[STATE_DATE_VALID] = true;
            
        } else {
            result["error"] = tr("Combine needs valid dates !");
        }

        
    } else {
               result["error"] = tr("Combine needs at least 2 dates !");
           }
        return result;

}

#endif
