/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


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
    QPair<long double, long double > result = getLikelihoodArg(t, data);
    qDebug()<<"in plugmag = t"<< t << double(expl(result.second) / sqrtl(result.first));
    return expl(result.second) / sqrtl(result.first);
}

QPair<long double, long double> PluginMag::getLikelihoodArg(const double& t, const QJsonObject& data)
{
    const bool is_inc = data.value(DATE_AM_IS_INC_STR).toBool();
    const bool is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
    const bool is_int = data.value(DATE_AM_IS_INT_STR).toBool();
    const long double alpha = static_cast<long double> (data.value(DATE_AM_ERROR_STR).toDouble());
    const long double inc = static_cast<long double> (data.value(DATE_AM_INC_STR).toDouble());
    const long double dec = static_cast<long double> (data.value(DATE_AM_DEC_STR).toDouble());
    const long double intensity = static_cast<long double> (data.value(DATE_AM_INTENSITY_STR).toDouble());
    //QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();

    long double mesure (0.l);
    long double error (0.l);

    if (is_inc) {
        error = alpha / 2.448l;
        mesure = inc;
    }
    else if (is_dec) {
        error = alpha / (2.448l * cosl(inc * M_PIl / 180.l));
        mesure = dec;
    }
    else if (is_int) {
        error = alpha;
        mesure = intensity;
    }

    long double refValue = static_cast<long double> (getRefValueAt(data, t));
    long double refError = static_cast<long double> (getRefErrorAt(data, t));

    long double variance = refError * refError + error * error;
    long double exponent = -0.5l * powl((mesure - refValue), 2.l) / variance;

    return qMakePair(variance, exponent);
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

Date::DataMethod PluginMag::getDataMethod() const
{
    return Date::eInversion;
}

QList<Date::DataMethod> PluginMag::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHSymetric);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}

QString PluginMag::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QLocale locale = QLocale();
    QString result;
    
    if (date->mOrigin == Date::eSingleDate) {

        const QJsonObject data = date->mData;

        const bool is_inc = data.value(DATE_AM_IS_INC_STR).toBool();
        const bool is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
        const bool is_int = data.value(DATE_AM_IS_INT_STR).toBool();
        const double alpha = data.value(DATE_AM_ERROR_STR).toDouble();
        const double inc = data.value(DATE_AM_INC_STR).toDouble();
        const double dec = data.value(DATE_AM_DEC_STR).toDouble();
        const double intensity = data.value(DATE_AM_INTENSITY_STR).toDouble();
        const QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();

        if (is_inc) {
            result += QObject::tr("Inclination : %1").arg(locale.toString(inc));
            // this is the html form, but not reconized in the DatesListItemDelegate
           // result += "; " + QString("α<SUB>95</SUB>") + " : " + locale.toString(alpha);
             result += "; " + QObject::tr("α 95 : %1").arg(locale.toString(alpha));
        } else if (is_dec) {
            result += QObject::tr("Declination : %1").arg(locale.toString(dec));
            result += "; " + QObject::tr("Inclination : %1").arg(locale.toString(inc));
            result += "; " + QObject::tr("α 95 : %1").arg(locale.toString(alpha));
        } else if (is_int)  {
            result += QObject::tr("Intensity : %1").arg(locale.toString(intensity));
            result += "; " + QObject::tr("Error : %1").arg(locale.toString(alpha));
        }

        if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
            result += "; " + tr("Ref. curve : %1").arg(ref_curve);
        else
            result += "; " + tr("ERROR -> Ref. curve : %1").arg(ref_curve);
        
    } else {
            result = "Combine (";
            QStringList datesDesc;
            for (int i (0); i< date->mSubDates.size(); i++) {
                const QJsonObject d = date->mSubDates.at(i).toObject();
                Date subDate;
                subDate.fromJson(d);
                datesDesc.append(getDateDesc(&subDate));

            }
            result += "Combined ( " + datesDesc.join(" | ") + " )";
    }

    return result;
}

QString PluginMag::getDateRefCurveName(const Date* date)
{
    Q_ASSERT(date);
    const QJsonObject data = date->mData;

    return  data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
}

// CSV
QStringList PluginMag::csvColumns() const
{
    QStringList cols;
    cols << "Data Name"
        //<< "type (inclination | declination | intensity)"
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
    result[DATE_AM_INTENSITY_STR] = result.value(DATE_AM_INTENSITY_STR).toDouble();
    result[DATE_AM_ERROR_STR] = result.value(DATE_AM_ERROR_STR).toDouble();

    result[DATE_AM_REF_CURVE_STR] = result.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    return result;
}

QJsonObject PluginMag::fromCSV(const QStringList& list,const QLocale &csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        double error = csvLocale.toDouble(list.at(5));
        if (error == 0.)
            return json;

        json.insert(DATE_AM_IS_INC_STR, list.at(1) == "inclination");
        json.insert(DATE_AM_IS_DEC_STR, list.at(1) == "declination");
        json.insert(DATE_AM_IS_INT_STR, list.at(1) == "intensity");
        json.insert(DATE_AM_INC_STR, csvLocale.toDouble(list.at(2)));
        json.insert(DATE_AM_DEC_STR, csvLocale.toDouble(list.at(3)));
        json.insert(DATE_AM_INTENSITY_STR, csvLocale.toDouble(list.at(4)));

        json.insert(DATE_AM_ERROR_STR, error);
        json.insert(DATE_AM_REF_CURVE_STR, list.at(6).toLower());
    }
    return json;
}

QStringList PluginMag::toCSV(const QJsonObject& data, const QLocale& csvLocale) const
{
    QStringList list;
    list << (data.value(DATE_AM_IS_INC_STR).toBool() ? "inclination" : (data.value(DATE_AM_IS_DEC_STR).toBool() ? "declination" : "intensity"));
    list << csvLocale.toString(data.value(DATE_AM_INC_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_DEC_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_INTENSITY_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_ERROR_STR).toDouble());
    list << data.value(DATE_AM_REF_CURVE_STR).toString();
    return list;
}

//Reference curves (files)
QString PluginMag::getRefExt() const
{
    return "ref";
}

QString PluginMag::getRefsPath() const
{
    #ifdef Q_OS_MAC
        QString path  =  qApp->applicationDirPath();
        QDir dir(path);
        dir.cdUp();
        path = dir.absolutePath() + "/Resources";
    #else
        //http://doc.qt.io/qt-5/qstandardpaths.html#details
        QStringList dataPath = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
        QString path  =  dataPath[0];
    #endif

    QString calibPath = path + "/Calib/AM";
    return calibPath;
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
    QString curveName = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    return getRefCurveValueAt(curveName, t);
}

double PluginMag::getRefErrorAt(const QJsonObject& data, const double& t)
{
    const QString curveName = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    return getRefCurveErrorAt(curveName, t);
}

QPair<double,double> PluginMag::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    double tmin (0.);
    double tmax (0.);
    QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();

    if (mRefCurves.contains(ref_curve)  && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
        tmin = mRefCurves.value(ref_curve).mTmin;
        tmax = mRefCurves.value(ref_curve).mTmax;
    }
    return qMakePair<double,double>(tmin,tmax);
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
bool PluginMag::isDateValid(const QJsonObject& data, const ProjectSettings& settings)
{
    qDebug() <<"PluginMag::isDateValid for="<< data.value(STATE_NAME).toString();
    // check valid curve
    QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    const bool is_inc = data.value(DATE_AM_IS_INC_STR).toBool();
    const bool is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
    const bool is_int = data.value(DATE_AM_IS_INT_STR).toBool();
    //double alpha = data.value(DATE_AM_ERROR_STR).toDouble();
    double inc = data.value(DATE_AM_INC_STR).toDouble();
    double dec = data.value(DATE_AM_DEC_STR).toDouble();
    double intensity = data.value(DATE_AM_INTENSITY_STR).toDouble();

    double mesure (0.);
    //double error = 0;

    if (is_inc) {
        //error = alpha / 2.448f;
        mesure = inc;
    }
    else if (is_dec) {
        //error = alpha / (2.448f * cos(inc * M_PI / 180.f));
        mesure = dec;
    }
    else if (is_int) {
        //error = alpha;
        mesure = intensity;
    }
    // controle valid solution (double)likelihood>0
    // remember likelihood type is long double
    const RefCurve& curve = mRefCurves.value(ref_curve);
    bool valid = false;
   // qDebug()<<"in plugmag refcurve"<<ref_curve<< curve.mDataInf<<curve.mTmin;

    if (mesure > curve.mDataInfMin && mesure < curve.mDataSupMax)
        valid = true;

    else {
        double t = curve.mTmin;
        long double repartition (0.l);
        long double v (0.l);
        long double lastV (0.l);
        while (valid == false && t <= curve.mTmax) {
            v = static_cast<long double> (getLikelihood(t,data));
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
        
        mergedData[DATE_AM_REF_CURVE_STR] = "|mag|.ref";
        mergedData[DATE_AM_IS_INC_STR] = false;
        mergedData[DATE_AM_IS_DEC_STR] = false;
        mergedData[DATE_AM_IS_INT_STR] = true;
        
        QStringList names;

        bool subDatIsValid (true);
        for (int i=0; i<dates.size(); ++i) {
            const QJsonObject date = dates.at(i).toObject();
          
            names.append(date.value(STATE_NAME).toString());
            // Validate the date before merge
            Date d;
            d.fromJson(date);
            subDatIsValid = d.mIsValid & subDatIsValid;
        
        }
        if (subDatIsValid) {
            // inherits the first data propeties as plug-in and method...
            result = dates.at(0).toObject();
            result[STATE_NAME] = "Combined ( " + names.join(" | ") + " )";
            result[STATE_DATE_DATA] = mergedData;
            result[STATE_DATE_ORIGIN] = Date::eCombination;
            result[STATE_DATE_SUB_DATES] = dates;
            result[STATE_DATE_VALID] = true;
            
        } else {
            result["error"] = tr("Combine needs valid dates !");
        }

        
    } else  {
               result["error"] = tr("Combine needs at least 2 dates !");
           }
        return result;

}
//QPair<double,double> PluginMag::getTminTmaxRefsCurveCombine(const QJsonArray& subData)
//{
//    double tmin (INFINITY);
//    double tmax (-INFINITY);
//
//    for (int i(0); i<subData.size(); ++i) {
//
//        const QPair<double, double> tminTmax = getTminTmaxRefsCurve( subData.at(i).toObject().value(STATE_DATE_DATA).toObject() );
//        tmin = std::min(tmin, tminTmax.first);
//        tmax = std::max(tmax, tminTmax.second);
//
//
//    }
//    return qMakePair(tmin, tmax);
//}
#endif
