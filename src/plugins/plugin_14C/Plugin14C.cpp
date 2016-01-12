#include "Plugin14C.h"
#if USE_PLUGIN_14C

#include "QtUtilities.h"
#include "StdUtilities.h"
#include "Plugin14CForm.h"
#include "Plugin14CRefView.h"
#include "Plugin14CSettingsView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


Plugin14C::Plugin14C()
{
    mColor = QColor(47,46,68);
    loadRefDatas();
}

Plugin14C::~Plugin14C()
{

}

#pragma mark Likelihood
QPair<long double, long double> Plugin14C::getLikelihoodArg(const double& t, const QJsonObject& data)
{
    double age = data[DATE_14C_AGE_STR].toDouble();
    double error = data[DATE_14C_ERROR_STR].toDouble();
    double delta_r = data[DATE_14C_DELTA_R_STR].toDouble();
    double delta_r_error = data[DATE_14C_DELTA_R_ERROR_STR].toDouble();
    
    // Apply reservoir effect
    age = (age - delta_r);
    error = sqrt(error * error + delta_r_error * delta_r_error);
    
    double refValue = getRefValueAt(data, t);
    double refError = getRefErrorAt(data, t);
    
    long double variance = refError * refError + error * error;
    long double exponent = -0.5f * powl((long double)(age - refValue), 2.l) / variance;
    return qMakePair(variance, exponent);
}

long double Plugin14C::getLikelihood(const double& t, const QJsonObject& data)
{
    QPair<long double, long double > result = getLikelihoodArg(t, data);
    long double back = expl(result.second) / sqrt(result.first) ;
    return back;
}

#pragma mark Properties
QString Plugin14C::getName() const
{
    return QString("14C");
}

QIcon Plugin14C::getIcon() const
{
    return QIcon(":/14C_w.png");
}

bool Plugin14C::doesCalibration() const
{
    return true;
}

bool Plugin14C::wiggleAllowed() const
{
    return true;
}
Date::DataMethod Plugin14C::getDataMethod() const
{
    return Date::eInversion;
}
QList<Date::DataMethod> Plugin14C::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHSymetric);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}

QString Plugin14C::getDateDesc(const Date* date) const
{
    QLocale locale=QLocale();
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        
        double age = data[DATE_14C_AGE_STR].toDouble();
        double error = data[DATE_14C_ERROR_STR].toDouble();
        double delta_r = data[DATE_14C_DELTA_R_STR].toDouble();
        double delta_r_error = data[DATE_14C_DELTA_R_ERROR_STR].toDouble();
        QString ref_curve = data[DATE_14C_REF_CURVE_STR].toString().toLower();
        
        result += QObject::tr("Age") + " : " + locale.toString(age);
        result += " ± " + locale.toString(error);
        if(delta_r != 0 || delta_r_error != 0){
            result += ", " + QObject::tr("ΔR") + " : " + locale.toString(delta_r);
            result += " ± " +locale.toString(delta_r_error);
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
QStringList Plugin14C::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Age" << "Error (sd)" << "Reference curve" << "ΔR" << "ΔR Error";
    return cols;
}

int Plugin14C::csvMinColumns() const{
    return csvColumns().count() - 2;
}

QJsonObject Plugin14C::fromCSV(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_14C_AGE_STR, list[1].toDouble());
        json.insert(DATE_14C_ERROR_STR, list[2].toDouble());
        json.insert(DATE_14C_REF_CURVE_STR, list[3].toLower());
        
        // These columns are nor mandatory in the CSV file so check if they exist :
        json.insert(DATE_14C_DELTA_R_STR, (list.size() > 4) ? list[4].toDouble() : 0);
        json.insert(DATE_14C_DELTA_R_ERROR_STR, (list.size() > 5) ? list[5].toDouble() : 0);
        
        //qDebug() << list;
        //qDebug() << json;
    }
    return json;
}

QStringList Plugin14C::toCSV(const QJsonObject& data, const QLocale& csvLocale)
{
    QStringList list;
    list << csvLocale.toString(data[DATE_14C_AGE_STR].toDouble());
    list << csvLocale.toString(data[DATE_14C_ERROR_STR].toDouble());
    list << data[DATE_14C_REF_CURVE_STR].toString();
    list << csvLocale.toString(data[DATE_14C_DELTA_R_STR].toDouble());
    list << csvLocale.toString(data[DATE_14C_DELTA_R_ERROR_STR].toDouble());
    return list;
}

// ------------------------------------------------------------------

#pragma mark Ref curves (files)
QString Plugin14C::getRefExt() const
{
    return "14c";
}

QString Plugin14C::getRefsPath() const
{
    QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
#endif
    QString calibPath = path + "/Calib/14C";
    return calibPath;
}

RefCurve Plugin14C::loadRefFile(QFileInfo refFile)
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
            if(!isComment(line))
            {
                QStringList values = line.split(",");
                if(values.size() >= 3)
                {
                    bool ok = true;
                    
                    int t = 1950 - locale.toInt(values[0],&ok);
                    if(!ok) continue;
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

#pragma mark References Values & Errors
double Plugin14C::getRefValueAt(const QJsonObject& data, const double& t)
{
    QString curveName = data[DATE_14C_REF_CURVE_STR].toString().toLower();
    return getRefCurveValueAt(curveName, t);
}

double Plugin14C::getRefErrorAt(const QJsonObject& data, const double& t)
{
    QString curveName = data[DATE_14C_REF_CURVE_STR].toString().toLower();
    return getRefCurveErrorAt(curveName, t);
}

QPair<double,double> Plugin14C::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    double tmin = 0;
    double tmax = 0;
    QString ref_curve = data[DATE_14C_REF_CURVE_STR].toString().toLower();

    if(mRefCurves.contains(ref_curve)  && !mRefCurves[ref_curve].mDataMean.isEmpty())
    {
       tmin = mRefCurves[ref_curve].mTmin;
       tmax = mRefCurves[ref_curve].mTmax;
    }
    return qMakePair<double,double>(tmin,tmax);
}

#pragma mark Settings / Input Form / RefView
GraphViewRefAbstract* Plugin14C::getGraphViewRef()
{
    //if(!mRefGraph) mRefGraph = new Plugin14CRefView();
    
    if(mRefGraph) delete mRefGraph;
    mRefGraph = new Plugin14CRefView();
    
    return mRefGraph;
}

PluginSettingsViewAbstract* Plugin14C::getSettingsView()
{
    return new Plugin14CSettingsView(this);
}

PluginFormAbstract* Plugin14C::getForm()
{
    Plugin14CForm* form = new Plugin14CForm(this);
    return form;
}

#pragma mark Convert old project versions
QJsonObject Plugin14C::checkValuesCompatibility(const QJsonObject& values)
{
    QJsonObject result = values;

    if(result.find(DATE_14C_DELTA_R_STR) == result.end())
        result[DATE_14C_DELTA_R_STR] = 0;
    
    if(result.find(DATE_14C_DELTA_R_ERROR_STR) == result.end())
        result[DATE_14C_DELTA_R_ERROR_STR] = 0;
    
    // Force curve name to lower case :
    result[DATE_14C_REF_CURVE_STR] = result[DATE_14C_REF_CURVE_STR].toString().toLower();
    
    return result;
}

#pragma mark Date Validity
bool Plugin14C::isDateValid(const QJsonObject& data, const ProjectSettings& settings)
{
    QString ref_curve = data[DATE_14C_REF_CURVE_STR].toString().toLower();
    bool valid = false;
    if(!mRefCurves.contains(ref_curve)) {
        qDebug()<<"in Plugin14C::isDateValid() unkowned curve"<<ref_curve;
        valid = false;
    }
    else {
        // controle valid solution (double)likelihood>0
        // remember likelihood type is long double
        const RefCurve& curve = mRefCurves[ref_curve];
        valid = false;
        double age = data[DATE_14C_AGE_STR].toDouble();
        double error = data[DATE_14C_ERROR_STR].toDouble();
        double delta_r = data[DATE_14C_DELTA_R_STR].toDouble();
        double delta_r_error = data[DATE_14C_DELTA_R_ERROR_STR].toDouble();

        // Apply reservoir effect
        age = (age - delta_r);
        error = sqrt(error * error + delta_r_error * delta_r_error);

        if(age>curve.mDataInfMin && age < curve.mDataSupMax){
            valid = true;
        }
        else {
            double t = curve.mTmin;
            long double repartition = 0;
            long double v = 0;
            long double lastV = 0;
            while(valid==false && t<=curve.mTmax) {
                v = (double)getLikelihood(t,data);
                // we have to check this calculs
                //because the repartition can be smaller than the calibration
                if (lastV>0 && v>0) {
                    repartition += (long double) settings.mStep * (lastV + v) / 2.;
                }
                lastV = v;

                valid = ( (double)repartition > 0);
                t +=settings.mStep;
            }
        }
    }
    return valid;
}

#pragma mark Grouped Actions
QList<QHash<QString, QVariant>> Plugin14C::getGroupedActions()
{
    QList<QHash<QString, QVariant>> result;
    
    QHash<QString, QVariant> groupedAction;
    groupedAction.insert("pluginId", getId());
    groupedAction.insert("title", tr("Selected events with 14C: change Ref. Curves"));
    groupedAction.insert("label", tr("Change 14C Ref. Curves for all 14C data in selected events :"));
    groupedAction.insert("inputType", "combo");
    groupedAction.insert("items", getRefsNames());
    groupedAction.insert("valueKey", DATE_14C_REF_CURVE_STR);
    
    result.append(groupedAction);
    return result;
}

#pragma mark Combine / Split
bool Plugin14C::areDatesMergeable(const QJsonArray& dates)
{
    QString refCurve;
    for(int i=0; i<dates.size(); ++i)
    {
        QJsonObject date = dates[i].toObject();
        QJsonObject data = date[STATE_DATE_DATA].toObject();
        QString curve = data[DATE_14C_REF_CURVE_STR].toString();
        
        if(refCurve.isEmpty())
            refCurve = curve;
        else if(refCurve != curve)
            return false;
    }
    return true;
}

QJsonObject Plugin14C::mergeDates(const QJsonArray& dates)
{
    QJsonObject result;
    if(dates.size() > 1){
        // Verify all dates have the same ref curve :
        QJsonObject firstDate = dates[0].toObject();
        QJsonObject firstDateData = firstDate[STATE_DATE_DATA].toObject();
        QString firstCurve = firstDateData[DATE_14C_REF_CURVE_STR].toString();
        
        for(int i=1; i<dates.size(); ++i){
            QJsonObject date = dates[i].toObject();
            QJsonObject dateData = date[STATE_DATE_DATA].toObject();
            QString curve = dateData[DATE_14C_REF_CURVE_STR].toString();
            if(firstCurve != curve){
                result["error"] = tr("All combined data must use the same reference curve !");
                return result;
            }
        }
        
        double sum_vi = 0;
        double sum_mi_vi = 0;
        double sum_1_vi = 0;
      //  double sum_mi_2 = 0;
        QStringList names;
        
        for(int i=0; i<dates.size(); ++i){
            QJsonObject date = dates[i].toObject();
            QJsonObject data = date[STATE_DATE_DATA].toObject();
            
            names.append(date[STATE_NAME].toString());
            double a = data[DATE_14C_AGE_STR].toDouble();
            double e = data[DATE_14C_ERROR_STR].toDouble();
            double r = data[DATE_14C_DELTA_R_STR].toDouble();
            double re = data[DATE_14C_DELTA_R_ERROR_STR].toDouble();
            
            // Reservoir effet
            double m = a - r;
            double v = e * e + re * re;
            
            sum_vi += v;
            sum_mi_vi += m/v;
            sum_1_vi += 1/v;
         //   sum_mi_2 += m*m;
        }
        
        result = dates[0].toObject();
        result[STATE_NAME] = "Combined (" + names.join(" | ") + ")";
        
        QJsonObject mergedData = result[STATE_DATE_DATA].toObject();
        mergedData[DATE_14C_AGE_STR] = sum_mi_vi / sum_1_vi;
        mergedData[DATE_14C_ERROR_STR] = sqrt(1 / sum_1_vi);
        mergedData[DATE_14C_DELTA_R_STR] = 0;
        mergedData[DATE_14C_DELTA_R_ERROR_STR] = 0;
        
        qDebug() << mergedData;
        
        result[STATE_DATE_DATA] = mergedData;
        result[STATE_DATE_SUB_DATES] = dates;
    }else{
        result["error"] = tr("Combine needs at least 2 data !");
    }
    return result;
    
}

#endif
