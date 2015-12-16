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
    mColor = QColor(198,79,32);
    loadRefDatas();
}

#pragma mark Likelihood
double PluginMag::getLikelyhood(const double& t, const QJsonObject& data)
{
    QPair<double, double > result = getLikelyhoodArg(t, data);
    return exp(result.second) / sqrt(result.first);
}

QPair<double, double> PluginMag::getLikelyhoodArg(const double& t, const QJsonObject& data)
{
    bool is_inc = data[DATE_AM_IS_INC_STR].toBool();
    bool is_dec = data[DATE_AM_IS_DEC_STR].toBool();
    bool is_int = data[DATE_AM_IS_INT_STR].toBool();
    double alpha = data[DATE_AM_ERROR_STR].toDouble();
    double inc = data[DATE_AM_INC_STR].toDouble();
    double dec = data[DATE_AM_DEC_STR].toDouble();
    double intensity = data[DATE_AM_INTENSITY_STR].toDouble();
    QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    
    double mesure = 0;
    double error = 0;
    
    if(is_inc)
    {
        error = alpha / 2.448f;
        mesure = inc;
    }
    else if(is_dec)
    {
        error = alpha / (2.448f * cos(inc * M_PI / 180.f));
        mesure = dec;
    }
    else if(is_int)
    {
        error = alpha;
        mesure = intensity;
    }
    
    double refValue = getRefValueAt(data, t);
    double refError = getRefErrorAt(data, t);
    
    double variance = refError * refError + error * error;
    double exponent = -0.5f * pow((mesure - refValue), 2.f) / variance;
    
    return qMakePair(variance, exponent);
}

#pragma mark Properties
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
    if(date)
    {
        QJsonObject data = date->mData;
        
        double is_inc = data[DATE_AM_IS_INC_STR].toBool();
        double is_dec = data[DATE_AM_IS_DEC_STR].toBool();
        double is_int = data[DATE_AM_IS_INT_STR].toBool();
        double alpha = data[DATE_AM_ERROR_STR].toDouble();
        double inc = data[DATE_AM_INC_STR].toDouble();
        double dec = data[DATE_AM_DEC_STR].toDouble();
        double intensity = data[DATE_AM_INTENSITY_STR].toDouble();
        QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString().toLower();
        
        if(is_inc)
        {
            result += QObject::tr("Inclination") + " : " + locale.toString(inc);
            result += ", " + QObject::tr("Alpha95") + " : " + locale.toString(alpha);
        }
        else if(is_dec)
        {
            result += QObject::tr("Declination") + " : " + locale.toString(dec);
            result += ", " + QObject::tr("Inclination") + " : " + locale.toString(inc);
            result += ", " + QObject::tr("Alpha95") + " : " + locale.toString(alpha);
        }
        else if(is_int)
        {
            result += QObject::tr("Intensity") + " : " + locale.toString(intensity);
            result += ", " + QObject::tr("Error") + " : " + locale.toString(alpha);
        }
        if(mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty()) {
            result += ", " + tr("Ref. curve") + " : " + ref_curve;
        }
        else {
            result += ", " + tr("ERROR") +"-> "+ tr("Ref. curve") + " : " + ref_curve;
        }
    }
    return result;
}

#pragma mark CSV
QStringList PluginMag::csvColumns() const
{
    QStringList cols;
    cols << "Name"
        << "type (inclination | declination | intensity)"
        << "Inclination value"
        << "Declination value"
        << "Intensity value"
        << "Error (sd) or alpha 95"
        << "Reference curve (file name)";
    return cols;
}


PluginFormAbstract* PluginMag::getForm()
{
    PluginMagForm* form = new PluginMagForm(this);
    return form;
}

QJsonObject PluginMag::fromCSV(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_AM_IS_INC_STR, list[1] == "inclination");
        json.insert(DATE_AM_IS_DEC_STR, list[1] == "declination");
        json.insert(DATE_AM_IS_INT_STR, list[1] == "intensity");
        json.insert(DATE_AM_INC_STR, list[2].toDouble());
        json.insert(DATE_AM_DEC_STR, list[3].toDouble());
        json.insert(DATE_AM_INTENSITY_STR, list[4].toDouble());
        json.insert(DATE_AM_ERROR_STR, list[5].toDouble());
        json.insert(DATE_AM_REF_CURVE_STR, list[6].toLower());
    }
    return json;
}

QStringList PluginMag::toCSV(const QJsonObject& data, const QLocale& csvLocale)
{
    QStringList list;
    list << (data[DATE_AM_IS_INC_STR].toBool() ? "inclination" : (data[DATE_AM_IS_DEC_STR].toBool() ? "declination" : "intensity"));
    list << csvLocale.toString(data[DATE_AM_INC_STR].toDouble());
    list << csvLocale.toString(data[DATE_AM_DEC_STR].toDouble());
    list << csvLocale.toString(data[DATE_AM_INTENSITY_STR].toDouble());
    list << csvLocale.toString(data[DATE_AM_ERROR_STR].toDouble());
    list << data[DATE_AM_REF_CURVE_STR].toString();
    return list;
}

#pragma mark Reference curves (files)
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
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QLocale locale = QLocale(QLocale::English);
        QTextStream stream(&file);
        bool firstLine = true;
        
        while(!stream.atEnd())
        {
            QString line = stream.readLine();

            if(line.contains("reference", Qt::CaseInsensitive) && line.contains("point", Qt::CaseInsensitive))
            {
                // TODO : start loading points
                break;
            }
            bool ok;
            if(!isComment(line))
            {
                QStringList values = line.split(",");
                if(values.size() >= 3)
                {
                    int t = locale.toInt(values[0],&ok);

                    double g = locale.toDouble(values[1],&ok);
                    if(!ok) continue;
                    double e = locale.toDouble(values[2],&ok);
                    if(!ok) continue;
                    
                    double gSup = g + 1.96f * e;
                    if(!ok) continue;
                    double gInf = g - 1.96f * e;
                    if(!ok) continue;
                    
                    curve.mDataMean[t] = g;
                    curve.mDataError[t] = e;
                    curve.mDataSup[t] = gSup;
                    curve.mDataInf[t] = gInf;
                    
                    if(firstLine)
                    {
                        curve.mDataMeanMin = g;
                        curve.mDataMeanMax = g;
                        
                        curve.mDataErrorMin = e;
                        curve.mDataErrorMax = e;
                        
                        curve.mDataSupMin = gSup;
                        curve.mDataSupMax = gSup;
                        
                        curve.mDataInfMin = gInf;
                        curve.mDataInfMax = gInf;
                    }
                    else
                    {
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
        if(!curve.mDataMean.isEmpty())
        {
            curve.mTmin = curve.mDataMean.firstKey();
            curve.mTmax = curve.mDataMean.lastKey();
        }
    }
    return curve;
}

#pragma mark Reference Values & Errors
double PluginMag::getRefValueAt(const QJsonObject& data, const double& t)
{
    QString curveName = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    return getRefCurveValueAt(curveName, t);
}

double PluginMag::getRefErrorAt(const QJsonObject& data, const double& t)
{
    QString curveName = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    return getRefCurveErrorAt(curveName, t);
}

QPair<double,double> PluginMag::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    double tmin = 0;
    double tmax = 0;
    QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    
    if(mRefCurves.contains(ref_curve)  && !mRefCurves[ref_curve].mDataMean.isEmpty())
    {
        tmin = mRefCurves[ref_curve].mTmin;
        tmax = mRefCurves[ref_curve].mTmax;
    }
    return qMakePair<double,double>(tmin,tmax);
}

#pragma mark Settings / Input Form / RefView
GraphViewRefAbstract* PluginMag::getGraphViewRef()
{
    if(mRefGraph) delete mRefGraph;
    mRefGraph = new PluginMagRefView();
    return mRefGraph;
}

PluginSettingsViewAbstract* PluginMag::getSettingsView()
{
    return new PluginMagSettingsView(this);
}

#pragma mark Date validity
bool PluginMag::isDateValid(const QJsonObject& data, const ProjectSettings& settings)
{
    // check valid curve
    QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    double is_inc = data[DATE_AM_IS_INC_STR].toBool();
    double is_dec = data[DATE_AM_IS_DEC_STR].toBool();
    double is_int = data[DATE_AM_IS_INT_STR].toBool();
    double alpha = data[DATE_AM_ERROR_STR].toDouble();
    double inc = data[DATE_AM_INC_STR].toDouble();
    double dec = data[DATE_AM_DEC_STR].toDouble();
    double intensity = data[DATE_AM_INTENSITY_STR].toDouble();
    
    double mesure;
    double error;
    
    if(is_inc)
    {
        error = alpha / 2.448f;
        mesure = inc;
    }
    else if(is_dec)
    {
        error = alpha / (2.448f * cos(inc * M_PI / 180.f));
        mesure = dec;
    }
    else if(is_int)
    {
        error = alpha;
        mesure = intensity;
    }
    
    const RefCurve& curve = mRefCurves[ref_curve];
    return ((mesure - 1.96f * error < curve.mDataSupMax) && (mesure + 1.96f * error > curve.mDataInfMin));
}


#endif
