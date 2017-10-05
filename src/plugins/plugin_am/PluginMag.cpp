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

//#pragma mark Likelihood
long double PluginMag::getLikelihood(const double& t, const QJsonObject& data)
{
    QPair<long double, long double > result = getLikelihoodArg(t, data);
    return expl(result.second) / sqrtl(result.first);
}

QPair<long double, long double> PluginMag::getLikelihoodArg(const double& t, const QJsonObject& data)
{
    const bool is_inc = data.value(DATE_AM_IS_INC_STR).toBool();
    const bool is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
    const bool is_int = data.value(DATE_AM_IS_INT_STR).toBool();
    const long double alpha =(long double) data.value(DATE_AM_ERROR_STR).toDouble();
    const long double inc = (long double) data.value(DATE_AM_INC_STR).toDouble();
    const long double dec = (long double) data.value(DATE_AM_DEC_STR).toDouble();
    const long double intensity = (long double) data.value(DATE_AM_INTENSITY_STR).toDouble();
    //QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    
    long double mesure (0.l);
    long double error (0.l);
    
    if (is_inc) {
        error = alpha / 2.448l;
        mesure = inc;
    }
    else if (is_dec) {
        error = alpha / (2.448l * cosl(inc * M_PI / 180.l));
        mesure = dec;
    }
    else if (is_int) {
        error = alpha;
        mesure = intensity;
    }

    long double refValue = (long double)getRefValueAt(data, t);
    long double refError = (long double)getRefErrorAt(data, t);
    
    long double variance = refError * refError + error * error;
    long double exponent = -0.5l * powl((mesure - refValue), 2.l) / variance;
    
    return qMakePair(variance, exponent);
}

//#pragma mark Properties
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
    QLocale locale=QLocale();
    QString result;
    if (date) {
        QJsonObject data = date->mData;
        
        double is_inc = data.value(DATE_AM_IS_INC_STR).toBool();
        double is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
        double is_int = data.value(DATE_AM_IS_INT_STR).toBool();
        double alpha = data.value(DATE_AM_ERROR_STR).toDouble();
        double inc = data.value(DATE_AM_INC_STR).toDouble();
        double dec = data.value(DATE_AM_DEC_STR).toDouble();
        double intensity = data.value(DATE_AM_INTENSITY_STR).toDouble();
        QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
        
        if (is_inc) {
            result += QObject::tr("Inclination") + " : " + locale.toString(inc);
            // this is the html form, but not reconized in the DatesListItemDelegate
           // result += "; " + QString("α<SUB>95</SUB>") + " : " + locale.toString(alpha);
             result += "; " + QObject::tr("α_95") + " : " + locale.toString(alpha);
        } else if (is_dec) {
            result += QObject::tr("Declination") + " : " + locale.toString(dec);
            result += "; " + QObject::tr("Inclination") + " : " + locale.toString(inc);
            result += "; " + QObject::tr("α_95") + " : " + locale.toString(alpha);
        } else if (is_int)  {
            result += QObject::tr("Intensity") + " : " + locale.toString(intensity);
            result += "; " + QObject::tr("Error") + " : " + locale.toString(alpha);
        }

        if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
            result += "; " + tr("Ref. curve") + " : " + ref_curve;
        else
            result += "; " + tr("ERROR") +"-> "+ tr("Ref. curve") + " : " + ref_curve;

    }
    return result;
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
        << "Error (sd) or α_95"
        << "Ref. curve";
    return cols;
}


PluginFormAbstract* PluginMag::getForm()
{
    PluginMagForm* form = new PluginMagForm(this);
    return form;
}
//#pragma mark Convert old project versions
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

//#pragma mark Reference curves (files)
QString PluginMag::getRefExt() const
{
    return "ref";
}

QString PluginMag::getRefsPath() const
{
    QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
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
                    int t = locale.toInt(values.at(0),&ok);

                    double g = locale.toDouble(values.at(1), &ok);
                    if (!ok)
                        continue;
                    double e = locale.toDouble(values.at(2), &ok);
                    if(!ok)
                        continue;
                    
                    double gSup = g + 1.96 * e;

                    double gInf = g - 1.96 * e;

                    
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

//#pragma mark Reference Values & Errors
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
    // check valid curve
    QString ref_curve = data.value(DATE_AM_REF_CURVE_STR).toString().toLower();
    double is_inc = data.value(DATE_AM_IS_INC_STR).toBool();
    double is_dec = data.value(DATE_AM_IS_DEC_STR).toBool();
    double is_int = data.value(DATE_AM_IS_INT_STR).toBool();
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

    if (mesure>curve.mDataInfMin && mesure < curve.mDataSupMax)
        valid = true;

    else {
        double t = curve.mTmin;
        long double repartition (0.);
        long double v (0.);
        long double lastV (0.);
        while (valid==false && t<=curve.mTmax) {
            v = (double)getLikelihood(t,data);
            // we have to check this calculs
            //because the repartition can be smaller than the calibration
            if (lastV>0. && v>0.)
                repartition += (long double) settings.mStep * (lastV + v) / 2.;

            lastV = v;

            valid = ( (double)repartition > 0.);
            t += settings.mStep;
        }
    }

return valid;

}


#endif
