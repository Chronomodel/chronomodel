/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
long double PluginMag::getLikelihood(const double t, const QJsonObject &data)
{
    return Likelihood(t, data);
}

QPair<long double, long double> PluginMag::getLikelihoodArg(const double t, const QJsonObject &data)
{
    (void) data;
    (void) t;
    return qMakePair<long double, long double> (0, 0);
}

long double PluginMag::Likelihood(const double t, const QJsonObject &data)
{
    // Lecture des données
    const long double incl = static_cast<long double> (data.value(DATE_AM_INC_STR).toDouble());
    const long double decl = static_cast<long double> (data.value(DATE_AM_DEC_STR).toDouble());
    const long double alpha95 = static_cast<long double> (data.value(DATE_AM_ALPHA95_STR).toDouble());
    const long double field = static_cast<long double> (data.value(DATE_AM_FIELD_STR).toDouble());
    const long double error_f = static_cast<long double> (data.value(DATE_AM_ERROR_F_STR).toDouble());

    const double iteration_mcmc = data.value(DATE_AM_ITERATION_STR).toDouble();

    // La partie des calculs de l'étalonnage
    //long double mesureIncl;
    //long double mesureDecl;
    //long double mesureField;
    long double errorD;
    long double errorI;
    long double errorF;



    // Lecture des différentes courbes d'étalonnage
    const QString &refI_curve = data.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
    const QString &refD_curve = data.value(DATE_AM_REF_CURVED_STR).toString().toLower();
    const QString &refF_curve = data.value(DATE_AM_REF_CURVEF_STR).toString().toLower();

    // Déclaration des variables
    long double vI;
    long double I;
    long double vD;
    long double D;
    long double vF;
    long double F;
    long double exponent = 0.;

    long double refValueI;
    long double refErrorI;
    long double refValueD;
    long double refErrorD;
    long double refValueF;
    long double refErrorF;
    double w = 0;
    long double rr;
    long double si, si0;

    const ProcessTypeAM pta = static_cast<ProcessTypeAM> (data.value(DATE_AM_PROCESS_TYPE_STR).toInt());

    switch (pta) {
    case eInc:
        //mesureIncl = incl;
        errorI = alpha95 / 2.448l;

        // Étalonnage  I
        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI;
        I = -0.5l*powl((incl - refValueI), 2.l);

        return expl(I/vI )/sqrtl(vI);
        break;

    case eDec:
        //mesureIncl = incl;
        //mesureDecl = decl;
        errorD = alpha95 / (2.448l * cosl(incl * rad));

        // Étalonnage D
        refValueD = getRefCurveValueAt(refD_curve, t);
        refErrorD = getRefCurveErrorAt(refD_curve, t);

        vD = errorD*errorD + refErrorD*refErrorD ;
        D = -0.5l*powl((decl - refValueD), 2.l);

        return expl(D/vD )/sqrtl(vD);
        break;

    case eField:
        //mesureField = field;
        errorF = error_f;
        // Étalonnage directionnel : I et D
        refValueF = getRefCurveValueAt(refF_curve, t);
        refErrorF = getRefCurveErrorAt(refF_curve, t);

        vF = errorF*errorF + refErrorF*refErrorF ;
        F = -0.5l*powl((field - refValueF), 2.l);

        return expl(F/vF )/sqrtl(vF);
        break;

    case eID:
        //mesureIncl = incl;
        errorI = alpha95 / 2.448l;
        //mesureDecl = decl;
        errorD = alpha95 / (2.448l * cosl(incl * rad));

        // Étalonnage directionnel : I et D
        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);

        refValueD = getRefCurveValueAt(refD_curve, t);
        refErrorD = getRefCurveErrorAt(refD_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI;
        I = -0.5l*powl((incl - refValueI), 2.l);
        vD = errorD*errorD + refErrorD*refErrorD ;
        D = -0.5l*powl((decl - refValueD), 2.l);

        // Simulation des valeurs de sigmaID
        /*QVector <double> sigmaID(iteration_mcmc);
        double w = 0;
       for (int i=0; i < sigmaID.size(); ++i) {
               w += 1/iteration_mcmc;
             sigmaID[i] = errorI*errorI*((1. - w)/w);
        }

        long double rr = powl((cosl(mesureIncl * rad)), 2.l);
        for (auto& sID : sigmaID){
          exponent += expl(I/(vI + sID) + D/(vD + sID/rr))/sqrtl((vI + sID)*(vD + sID/rr));
        }
        */
        // Code Optimization
        rr = pow((cos(incl * rad)), 2.);
        for (int i=0; i < iteration_mcmc; ++i) {
            w += 1/iteration_mcmc;
            si = errorI*errorI*((1. - w)/w);
            exponent += expl(I/(vI + si) + D/(vD + si/rr))/sqrtl((vI + si)*(vD + si/rr));
        }
        return exponent;
        break;

    case eIF:
        errorI = alpha95 / 2.448l;
        errorF = error_f;

        // Vector calibration: I and F
        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);

        refValueF = getRefCurveValueAt(refF_curve, t);
        refErrorF = getRefCurveErrorAt(refF_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI;
        I = -0.5l*powl((incl - refValueI), 2.l);
        vF = errorF*errorF + refErrorF*refErrorF ;
        F = -0.5l*powl((field - refValueF), 2.l);
        // Calculation of s0IF
        si0 = 0.5*(errorF*errorF + powl(errorI*field * rad, 2.l));
        // Simulation of sigmaIF values

        for (int i=0; i < iteration_mcmc; ++i) {
            w += 1/iteration_mcmc;
            si = si0*((1. - w)/w);
            exponent += expl(I/(vI + si/(field*field)) + F/(vF + si))/sqrtl((vI + si/(field*field))*(vF + si));
        }
        return exponent;
        break;

    case eIDF:
        errorI = alpha95 / 2.448l;
        errorD = alpha95 / (2.448l * cosl(incl * rad));
        errorF = error_f;

        refValueI = getRefCurveValueAt(refI_curve, t);
        refErrorI = getRefCurveErrorAt(refI_curve, t);

        refValueD = getRefCurveValueAt(refD_curve, t);
        refErrorD = getRefCurveErrorAt(refD_curve, t);

        refValueF = getRefCurveValueAt(refF_curve, t);
        refErrorF = getRefCurveErrorAt(refF_curve, t);

        vI = errorI*errorI + refErrorI*refErrorI;
        I = -0.5l*powl((incl - refValueI), 2.l);

        vD = errorD*errorD + refErrorD*refErrorD ;
        D = -0.5l*powl((decl - refValueD), 2.l);

        vF = errorF*errorF + refErrorF*refErrorF ;
        F = -0.5l*powl((field - refValueF), 2.l);


        rr = powl((field * cosl(incl * rad)), 2);
        // calculation of s0IDF
        si0 = (errorF*errorF + 2.l*powl((errorI*field * rad), 2))/3;

        // Simulation of sigmaIDF values
        for (int i=0; i < iteration_mcmc; ++i) {
            w += 1/iteration_mcmc;
            si = si0*((1. - w)/w);
            exponent += expl(I/(vI + si/(field*field)) + D/(vD + si/rr)  + F/(vF + si))/sqrtl((vI + si/(field*field))*(vF + si)*(vD + si/rr));
        }
        return exponent;
        break;
    default:
        return 0.;
        break;
    }

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

        const double incl = data.value(DATE_AM_INC_STR).toDouble();
        const double decl = data.value(DATE_AM_DEC_STR).toDouble();
        const double alpha95 = data.value(DATE_AM_ALPHA95_STR).toDouble();

        const double field = data.value(DATE_AM_FIELD_STR).toDouble();
        const double error_f = data.value(DATE_AM_ERROR_F_STR).toDouble();

        const double iteration_mcmc = data.value(DATE_AM_ITERATION_STR).toDouble();

        const ProcessTypeAM pta = static_cast<ProcessTypeAM> (date->mData.value(DATE_AM_PROCESS_TYPE_STR).toInt());

        QString ref_curve_i = date->mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        if (!mRefCurves.contains(ref_curve_i) || mRefCurves[ref_curve_i].mDataMean.isEmpty())
           ref_curve_i = tr("ERROR -> Unknown curve : %1").arg(ref_curve_i);


        QString ref_curve_d = date->mData.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        if (!mRefCurves.contains(ref_curve_d) || mRefCurves[ref_curve_d].mDataMean.isEmpty())
           ref_curve_d = tr("ERROR -> Unknown curve : %1").arg(ref_curve_d);

        QString ref_curve_f = date->mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        if (!mRefCurves.contains(ref_curve_f) || mRefCurves[ref_curve_f].mDataMean.isEmpty())
           ref_curve_f = tr("ERROR -> Unknown curve : %1").arg(ref_curve_f);


        switch (pta) {
        case eInc:
            result += tr("Inclination : %1; α 95 = %2").arg(locale.toString(incl), locale.toString(alpha95));
            result += " : " + ref_curve_i;
            break;

        case eDec:
            result += tr("Declination : %1; Inclination : %2; α 95 : %3").arg(locale.toString(decl), locale.toString(incl)), locale.toString(alpha95);
            result += " : " + ref_curve_d;
            break;

        case eField:
            result += tr("Field : %1; F Error : %2").arg(locale.toString(field), locale.toString(error_f));
            result += " : " + ref_curve_f;
            break;

        case eID:
            result += tr("ID = (%1; %2; α 95 : %3)").arg(locale.toString(incl), locale.toString(decl), locale.toString(alpha95)) ;
            result += tr("; (%1 | %2)").arg(ref_curve_i, ref_curve_d);
            result += tr("; (Iter : %1)").arg(locale.toString(iteration_mcmc));
            break;

        case eIF:
            result += tr("IF = (%1; %2)").arg(locale.toString(incl), locale.toString(field));
            result += tr("; (α 95; F Error) =  (%1; %2)").arg(locale.toString(alpha95), locale.toString(error_f));
            result += tr("; (%1 | %2)").arg(ref_curve_i, ref_curve_f);
            result += tr("; (Iter : %1)").arg(locale.toString(iteration_mcmc));
            break;

        case eIDF:
            result += tr("IDF = (%1; %2; %3)").arg(locale.toString(incl), locale.toString(decl), locale.toString(field));
            result += tr(" ; (α 95; F Error) = (%1; %2)").arg(locale.toString(alpha95), locale.toString(error_f));
            result += tr("; (%1 | %2 | %3)").arg(ref_curve_i, ref_curve_d, ref_curve_f);
            result += tr("; (Iter : %1)").arg(locale.toString(iteration_mcmc));
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
    case eID:
        ref_curve = date->mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower()+ " | "; ref_curve = date->mData.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        break;
    case eIF:
        ref_curve = date->mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower()+ " | "; ref_curve = date->mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        break;
    case eIDF:
        ref_curve = date->mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower()+ " | "; ref_curve = date->mData.value(DATE_AM_REF_CURVED_STR).toString().toLower()
                    + " | "; ref_curve = date->mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
        break;

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
        << "α 95"
        << "Field value"
        << "Field Error (sd)"
        << "Iteration"
        << "Inclination Ref. curve"
        << "Declination Ref. curve"
        << "Field Ref. curve";
    return cols;
}

PluginFormAbstract* PluginMag::getForm()
{
    PluginMagForm* form = new PluginMagForm(this);
    return form;
}

//Convert old project versions
/**
 * @brief PluginMag::checkValuesCompatibility
 * @param values
 * @return
 */
/**  data JSON version 2
 *   "data": {
                "dec": 150,
                "error": 1,
                "inc": 60,
                "intensity": 50,
                "is_dec": true,
                "is_inc": false,
                "is_int": false,
                "ref_curve": "gal2002sph2014_d.ref"
            },
  ** data JSON version 3
  **  "data": {
                "alpha95": 1,
                "dec": 5.4,
                "error_f": 2,
                "field": 65,
                "inc": 65,
                "integration_steps": 500,
                "process_type": 4,
                "refD_curve": "gal2002sph2014_d.ref",
                "refF_curve": "gwh2013uni_f.ref",
                "refI_curve": "gal2002sph2014_i.ref"
              },
*/
QJsonObject PluginMag::checkValuesCompatibility(const QJsonObject &values)
{
    QJsonObject result = values;

    if (!result.contains(DATE_AM_PROCESS_TYPE_STR)) {


        ProcessTypeAM pta = eNone;


        if (result.value("is_inc").toBool()) {
                pta = eInc;
                result[DATE_AM_INC_STR] = result.value("inc").toDouble();
                result[DATE_AM_DEC_STR] = result.value("dec").toDouble();
                result[DATE_AM_ALPHA95_STR] = result.value("error").toDouble();
                result[DATE_AM_REF_CURVEI_STR] = result.value("ref_curve").toString().toLower();
                result[DATE_AM_REF_CURVED_STR] = "";

                result[DATE_AM_FIELD_STR] = result.value("intensity").toDouble();
                result[DATE_AM_ERROR_F_STR] = 0.;
                result[DATE_AM_REF_CURVEF_STR] = "";

        } else if (result.value("is_dec").toBool()) {
                pta = eDec;
                result[DATE_AM_INC_STR] = result.value("inc").toDouble();
                result[DATE_AM_DEC_STR] = result.value("dec").toDouble();
                result[DATE_AM_ALPHA95_STR] = result.value("error").toDouble();
                result[DATE_AM_REF_CURVEI_STR] = "";
                result[DATE_AM_REF_CURVED_STR] = result.value("ref_curve").toString().toLower();

                result[DATE_AM_FIELD_STR] = result.value("intensity").toDouble();
                result[DATE_AM_ERROR_F_STR] = 0.;
                result[DATE_AM_REF_CURVEF_STR] = "";

        } else if (result.value("is_int").toBool()) {
                pta = eField;
                result[DATE_AM_INC_STR] = result.value("inc").toDouble();
                result[DATE_AM_DEC_STR] = result.value("dec").toDouble();
                result[DATE_AM_ALPHA95_STR] = 0.;
                result[DATE_AM_REF_CURVEI_STR] = "";
                result[DATE_AM_REF_CURVED_STR] = "";

                result[DATE_AM_FIELD_STR] = result.value("intensity").toDouble();
                result[DATE_AM_ERROR_F_STR] = result.value("error").toDouble();
                result[DATE_AM_REF_CURVEF_STR] = result.value("ref_curve").toString().toLower();
        }

        result.insert(DATE_AM_PROCESS_TYPE_STR, pta);
        result.insert(DATE_AM_ITERATION_STR, 500);

        result.remove("error");
        result.remove("intensity");
        result.remove("ref_curve");

    } else {

        // Version 3 : force type double
        result[DATE_AM_INC_STR] = result.value(DATE_AM_INC_STR).toDouble();
        result[DATE_AM_DEC_STR] = result.value(DATE_AM_DEC_STR).toDouble();
        result[DATE_AM_ALPHA95_STR] = result.value(DATE_AM_ALPHA95_STR).toDouble();

        result[DATE_AM_FIELD_STR] = result.value(DATE_AM_FIELD_STR).toDouble();
        result[DATE_AM_ERROR_F_STR] = result.value(DATE_AM_ERROR_F_STR).toDouble();

        result[DATE_AM_ITERATION_STR] = result.value(DATE_AM_ITERATION_STR).toInt(500);

        result[DATE_AM_REF_CURVEI_STR] = result.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        result[DATE_AM_REF_CURVED_STR] = result.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        result[DATE_AM_REF_CURVEF_STR] = result.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
    }
    return result;
}


/**
 * @brief PluginMag::fromCSV
 * @param list
 * @param csvLocale
 * @return
 * @example:
 * #Event name; plugin=AM; dating name/code; method; Inclination; Declination; α_95;Field; Field_error; MCMC_Iter; Inclination_ref_curve; Declination_ref_curve; Field_ref_curve;wiggle type;wiggle param a;wiggle param b;;;;;;X_Inc_Depth;Err X- apha95- Err depth;Y_Declinaison;Err Y;Z_Field;Err Z_Err F
 * my Event Inc ;AM;my Inclination;inclination;60;0;3;65;2;bulgaria_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 * my Event Dec ;AM;my Declination;declination;60;5,7;2,5;0;0;gal2002sph2014_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 * my Event Field;AM;my Field;field;0;0;0;47,5;4,5;;;bulgaria_f.ref;none
 * my Event Inc2 ;AM;my Inclination;inclination;60;0;3;65;2;bulgaria_i.ref;;;none
 * my Event Dec 2;AM;my Declination;declination;60;5,7;2,5;0;0;;bulgaria_d.ref;;none
 * my Event Field2;AM;my Intensity;field;60;0;0;47,5;4,5;;;bulgaria_f.ref;none
 * my Event incl-decl;AM;my Direction;incl-decl;60;5,7;3;65;2;bulgaria_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 * my Event incl-field;AM;my Vector IF;incl-field;60;5,7;2,5;5,7;2;bulgaria_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 * my Event incl-decl-field;AM;my Vector IDF;incl-decl-field;60;5,7;2,5;47,5;2,3;bulgaria_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 * my Event 1;AM;my Direction ID;incl-decl;60;5,7;3;65;2;501;bulgaria_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 * my Event 1;AM;my Vector IF;incl-field;60;5,7;2,5;47,5;4,5;502;bulgaria_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 * my Event 1;AM;my Vector IDF;incl-decl-field;60;5,7;2,5;47,5;4,5;503;bulgaria_i.ref;bulgaria_d.ref;bulgaria_f.ref;none
 */
QJsonObject PluginMag::fromCSV(const QStringList &list,const QLocale &csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {

        // checks the validity interval
        const QString name = list.at(0);
        const double valInc = csvLocale.toDouble(list.at(2));
        const double valDec = csvLocale.toDouble(list.at(3));
        const double valAlpha95 = csvLocale.toDouble(list.at(4));

        const double valField = csvLocale.toDouble(list.at(5));
        const double valError_f = csvLocale.toDouble(list.at(6));
        const double valIter = csvLocale.toDouble(list.at(7));
         
        if ((list.at(1) == "inclination" || list.at(1) == "declination") && (valInc>90 || valInc < -90)) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for Field"), tr(" %1 : must be >=-90 and <=90").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;

        } else  if (list.at(1) == "declination" && (valDec>270 || valDec < -90) ) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for declination"), tr(" %1 : must be >=-90 and <=270").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;

        } else if (list.at(1) == "field" && valField <=0 ) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for Field"), tr(" %1 : must be >0").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;

        } else if ( valError_f <= 0 ) {
            QMessageBox message(QMessageBox::Warning, tr("Invalide value for F Error"), tr(" %1 : must be >0").arg(name) ,  QMessageBox::Ok, qApp->activeWindow());
            message.exec();
            return json;
        }

        ProcessTypeAM pta = eNone;

        const QString typeStr = list.at(1).toLower();

        if (typeStr == "inclination") {
            pta = eInc;

        } else if (typeStr == "declination") {
            pta = eDec;

        } else if (typeStr == "field" || typeStr=="intensity") {
            pta = eField;

        } else if (typeStr == "incl-decl") {
            pta = eID;

        } else if (typeStr == "incl-field") {
            pta = eIF;

        } else if (typeStr == "incl-decl-field") {
            pta = eIDF;
        }

        json.insert(DATE_AM_PROCESS_TYPE_STR, pta);
        json.insert(DATE_AM_INC_STR, valInc);
        json.insert(DATE_AM_DEC_STR, valDec);
        json.insert(DATE_AM_ALPHA95_STR, valAlpha95);

        json.insert(DATE_AM_FIELD_STR, valField);
        json.insert(DATE_AM_ERROR_F_STR, valError_f);
        json.insert(DATE_AM_ITERATION_STR, valIter);

        json.insert(DATE_AM_REF_CURVEI_STR, list.at(8).toLower());
        json.insert(DATE_AM_REF_CURVED_STR, list.at(9).toLower());
        json.insert(DATE_AM_REF_CURVEF_STR, list.at(10).toLower());


    }
    return json;
}

QStringList PluginMag::toCSV(const QJsonObject &data, const QLocale &csvLocale) const
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
    list << csvLocale.toString(data.value(DATE_AM_ITERATION_STR).toInt(500));

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
        double prev_t = -INFINITY;
        double delta_t = INFINITY;

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

                    delta_t = std::min(delta_t, abs(t-prev_t));

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
                    prev_t = t;
                }
            }
        }
        file.close();

        curve.mMinStep = delta_t;
        // invalid file ?
        if (!curve.mDataMean.isEmpty()) {
            curve.mTmin = curve.mDataMean.firstKey();
            curve.mTmax = curve.mDataMean.lastKey();
        }
    }
    return curve;
}

//Reference Values & Errors
double PluginMag::getRefValueAt(const QJsonObject &data, const double t)
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
        default:
            break;
    }
    return getRefCurveValueAt(ref_curve, t);
}

double PluginMag::getRefErrorAt(const QJsonObject &data, const double t)
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
