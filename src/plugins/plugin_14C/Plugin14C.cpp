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

QPair<double, double > Plugin14C::getLikelyhoodArg(const double& t, const QJsonObject& data)
{
    double age = data[DATE_14C_AGE_STR].toDouble();
    double error = data[DATE_14C_ERROR_STR].toDouble();
    double delta_r = data[DATE_14C_DELTA_R_STR].toDouble();
    double delta_r_error = data[DATE_14C_DELTA_R_ERROR_STR].toDouble();
    QString ref_curve = data[DATE_14C_REF_CURVE_STR].toString().toLower();
    //qDebug()<<"Plugin14C::getLikelyhoodArg"<<ref_curve;
    // Apply reservoir effect
    age = (age - delta_r);
    error = sqrt(error * error + delta_r_error * delta_r_error);
    
    
    
    
    // Check if calib curve exists !
    if(mRefDatas.find(ref_curve) != mRefDatas.end())
    {
        double variance;
        double exponent;
        
        const QMap<double, double>& curveG = mRefDatas[ref_curve]["G"];
        const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        double tMinDef=curveG.firstKey();
        double tMaxDef=curveG.lastKey();
        double g;
        double g_sup;
        double e;
        
        if(t>tMaxDef){
            g=interpolate(t, tMinDef, tMaxDef, curveG[tMinDef], curveG[tMaxDef]);
            e = (curveG95Sup[tMaxDef] - curveG[tMaxDef]) / 1.96f;
            variance = e * e + error * error;
        }
        else if (t<tMinDef){
            g=interpolate(t, tMinDef, tMaxDef, curveG[tMinDef], curveG[tMaxDef]);
            e = (curveG95Sup[tMinDef] - curveG[tMinDef]) / 1.96f;
            variance = e * e + error * error;
        }
        else {
            
            double t_under = floor(t);
            double t_upper = t_under + 1;
            if(curveG.find(t_under) != curveG.end() &&
               curveG.find(t_upper) != curveG.end())
            {
                double g_under = curveG[t_under];
                double g_upper = curveG[t_upper];
                g = interpolate(t, t_under, t_upper, g_under, g_upper);
            
                double g_sup_under = curveG95Sup[t_under];
                double g_sup_upper = curveG95Sup[t_upper];
                 g_sup = interpolate(t, t_under, t_upper, g_sup_under, g_sup_upper);
            
                e = (g_sup - g) / 1.96f;
                variance = e * e + error * error;
            }
        }
        exponent = -0.5f * pow(g - age, 2.f) / variance;
        return qMakePair(variance,exponent);
        
    }
    else {
        return QPair<double,double>();
    }
}

double Plugin14C::getLikelyhood(const double& t, const QJsonObject& data)
{
    QPair<double, double > result = getLikelyhoodArg(t, data);
    double back = exp(result.second) / sqrt(result.first) ;
    return back;

}

/*
double Plugin14C::getLikelyhood(const double& t, const QJsonObject& data)
{
    double age = data[DATE_14C_AGE_STR].toDouble();
    double error = data[DATE_14C_ERROR_STR].toDouble();
    double delta_r = data[DATE_14C_DELTA_R_STR].toDouble();
    double delta_r_error = data[DATE_14C_DELTA_R_ERROR_STR].toDouble();
    QString ref_curve = data[DATE_14C_REF_CURVE_STR].toString().toLower();
    
    // Apply reservoir effect
    age = (age - delta_r);
    error = sqrt(error * error + delta_r_error * delta_r_error);
    
    double result = 0;
    
    // Check if calib curve exists !
    
    
    if(mRefDatas.find(ref_curve) != mRefDatas.end())
    {
        const QMap<double, double>& curveG = mRefDatas[ref_curve]["G"];
        const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        
        double tMinDef=curveG.firstKey();
        double tMaxDef=curveG.lastKey();
        double g;
        double variance;
        double exponent;
        
        if(t>tMaxDef){
            g= curveG[tMaxDef];
            variance=10E6;
            exponent = -0.5f * pow(g - age, 2.f) / variance;
            result = exp(exponent) / sqrt(variance);
        }
        else if (t<tMinDef){
            g= curveG[tMinDef];
            variance=10E6;
            exponent = -0.5f * pow(g - age, 2.f) / variance;
            result = exp(exponent) / sqrt(variance);
        }
        else {
        
            double t_under = floor(t);
            double t_upper = t_under + 1;
        
            if(curveG.find(t_under) != curveG.end() &&
                curveG.find(t_upper) != curveG.end())
            {
                double g_under = curveG[t_under];
                double g_upper = curveG[t_upper];
                g = interpolate(t, t_under, t_upper, g_under, g_upper);
            
                double g_sup_under = curveG95Sup[t_under];
                double g_sup_upper = curveG95Sup[t_upper];
                double g_sup = interpolate(t, t_under, t_upper, g_sup_under, g_sup_upper);
            
                double e = (g_sup - g) / 1.96f;
                variance = e * e + error * error;
            
                result = exp(-0.5f * pow(g - age, 2.f) / variance) / sqrt(variance);
            }
            else
            {
                //qDebug() << "failed";
            }
        }
    }
    return result;
}
*/


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

QStringList Plugin14C::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Age" << "Error (sd)" << "Reference curve" << "ΔR" << "ΔR Error";
    return cols;
}

int Plugin14C::csvMinColumns() const{
    return csvColumns().count() - 2;
}

PluginFormAbstract* Plugin14C::getForm()
{
    Plugin14CForm* form = new Plugin14CForm(this);
    return form;
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

QStringList Plugin14C::toCSV(const QJsonObject& data)
{
    QStringList list;
    list << QString::number(data[DATE_14C_AGE_STR].toDouble());
    list << QString::number(data[DATE_14C_ERROR_STR].toDouble());
    list << data[DATE_14C_REF_CURVE_STR].toString();
    list << QString::number(data[DATE_14C_DELTA_R_STR].toDouble());
    list << QString::number(data[DATE_14C_DELTA_R_ERROR_STR].toDouble());
    return list;
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
        result += ", " + QObject::tr("Ref. curve") + " : " + ref_curve;
    }
    return result;
}

// ------------------------------------------------------------------

QStringList Plugin14C::getRefsNames() const
{
    QStringList refNames;
    typename QMap< QString, QMap<QString, QMap<double, double> > >::const_iterator it = mRefDatas.begin();
    while(it != mRefDatas.end())
    {
        refNames.push_back(it.key());
        ++it;
    }
    return refNames;
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

void Plugin14C::loadRefDatas()//const ProjectSettings& settings)
{
    mRefDatas.clear();
    
    QString calibPath = getRefsPath();
    QDir calibDir(calibPath);
    
    QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
    for(int i=0; i<files.size(); ++i)
    {
        if(files[i].suffix().toLower() == "14c")
        {
            QFile file(files[i].absoluteFilePath());
            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QMap<QString, QMap<double, double> > curves;
                
                QMap<double, double> curveG;
                QMap<double, double> curveG95Sup;
                QMap<double, double> curveG95Inf;
                
                QTextStream stream(&file);
                while(!stream.atEnd())
                {
                    QString line = stream.readLine();
                    if(!isComment(line))
                    {
                        QStringList values = line.split(",");
                        if(values.size() >= 3)
                        {
                            int t = 1950 - values[0].toInt();
                            
                            double g = values[1].toDouble();
                            double gSup = g + 1.96f * values[2].toDouble();
                            double gInf = g - 1.96f * values[2].toDouble();
                            
                            curveG[t] = g;
                            
                            curveG95Sup[t] = gSup;
                            curveG95Inf[t] = gInf;
                        }
                    }
                }
                file.close();
                
                // The curves do not have 1-year precision!
                // We have to interpolate in the blanks
                
                double tmin = curveG.firstKey();
                double tmax = curveG.lastKey();
                
                for(double t=tmin; t<tmax; ++t)//t+=settings.mStep)//++t)
                {
                    if(curveG.find(t) == curveG.end())
                    {
                        // This actually return the iterator with the nearest greater key !!!
                        QMap<double, double>::const_iterator iter = curveG.lowerBound(t);
                        if(iter != curveG.end())
                        {
                            double t_upper = iter.key();
                            --iter;
                            //if(iter != curveG.begin())
                            {
                                double t_under = iter.key();
                                
                                //qDebug() << t_under << " < " << t << " < " << t_upper;
                                
                                double g_under = curveG[t_under];
                                double g_upper = curveG[t_upper];
                                
                                double gsup_under = curveG95Sup[t_under];
                                double gsup_upper = curveG95Sup[t_upper];
                                
                                double ginf_under = curveG95Inf[t_under];
                                double ginf_upper = curveG95Inf[t_upper];
                                
                                curveG[t] = interpolate(t, t_under, t_upper, g_under, g_upper);
                                curveG95Sup[t] = interpolate(t, t_under, t_upper, gsup_under, gsup_upper);
                                curveG95Inf[t] = interpolate(t, t_under, t_upper, ginf_under, ginf_upper);
                            }
                            /*else
                            {
                                curveG[t] = 0;
                                curveG95Sup[t] = 0;
                                curveG95Inf[t] = 0;
                            }*/
                        }
                        else
                        {
                            /*curveG[t] = 0;
                            curveG95Sup[t] = 0;
                            curveG95Inf[t] = 0;*/
                        }
                    }
                }
                
                // Store the resulting curves :
                
                curves["G"] = curveG;
                curves["G95Sup"] = curveG95Sup;
                curves["G95Inf"] = curveG95Inf;
                
                mRefDatas[files[i].fileName().toLower()] = curves;
            }
        }
    }
}

const QMap<QString, QMap<double, double> >& Plugin14C::getRefData(const QString& name)
{
    return mRefDatas[name.toLower()];
}

// ------------------------------------------------------------------
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

QList<QHash<QString, QVariant>> Plugin14C::getGroupedActions()
{
    QList<QHash<QString, QVariant>> result;
    
    QHash<QString, QVariant> groupedAction;
    groupedAction.insert("pluginId", getId());
    groupedAction.insert("title", tr("Change selected events 14C Ref. Curves"));
    groupedAction.insert("label", tr("Change 14C Ref. Curves for all 14C data in selected events :"));
    groupedAction.insert("inputType", "combo");
    groupedAction.insert("items", getRefsNames());
    groupedAction.insert("valueKey", DATE_14C_REF_CURVE_STR);
    
    result.append(groupedAction);
    return result;
}

QJsonObject Plugin14C::checkValuesCompatibility(const QJsonObject& values){
    QJsonObject result = values;

    if(result.find(DATE_14C_DELTA_R_STR) == result.end())
        result[DATE_14C_DELTA_R_STR] = 0;
    
    if(result.find(DATE_14C_DELTA_R_ERROR_STR) == result.end())
        result[DATE_14C_DELTA_R_ERROR_STR] = 0;
    
    // Force curve name to lower case :
    result[DATE_14C_REF_CURVE_STR] = result[DATE_14C_REF_CURVE_STR].toString().toLower();
    
    return result;
}

bool Plugin14C::isDateValid(const QJsonObject& data, const ProjectSettings& settings){
    
    double age = data[DATE_14C_AGE_STR].toDouble();
    double error = data[DATE_14C_ERROR_STR].toDouble();
    double delta_r = data[DATE_14C_DELTA_R_STR].toDouble();
    double delta_r_error = data[DATE_14C_DELTA_R_ERROR_STR].toDouble();
    QString ref_curve = data[DATE_14C_REF_CURVE_STR].toString();
    
    // Apply reservoir effect
    age = (age - delta_r);
    error = sqrt(error * error + delta_r_error * delta_r_error);
    
    double min = 0;
    double max = 0;
    
    if(mLastRefsMinMax.find(ref_curve) != mLastRefsMinMax.end() &&
       mLastRefsMinMax[ref_curve].first.first == settings.mTmin &&
       mLastRefsMinMax[ref_curve].first.second == settings.mTmax)
    {
        min = mLastRefsMinMax[ref_curve].second.first;
        max = mLastRefsMinMax[ref_curve].second.second;
    }
    else
    {
        const QMap<double, double>& curveG95Inf = mRefDatas[ref_curve]["G95Inf"];
        const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        // Find max
        QMap<double, double>::const_iterator iter = curveG95Sup.constFind(settings.mTmin);
        if(iter == curveG95Sup.constEnd()){
            double t1 = curveG95Sup.firstKey();
            if(t1 < settings.mTmax){
                iter = curveG95Sup.constBegin();
            }
        }
        if(iter != curveG95Sup.constEnd()){
            max = iter.value();
            while(iter != curveG95Sup.constEnd() && iter.key() <= settings.mTmax){
                max = qMax(max, iter.value());
                ++iter;
            }
        }
        
        // Find min
        iter = curveG95Inf.constFind(settings.mTmin);
        if(iter == curveG95Inf.constEnd()){
            double t1 = curveG95Inf.firstKey();
            if(t1 < settings.mTmax){
                iter = curveG95Inf.constBegin();
            }
        }
        if(iter != curveG95Inf.constEnd()){
            min = iter.value();
            while(iter != curveG95Inf.constEnd() && iter.key() <= settings.mTmax){
                min = qMin(min, iter.value());
                ++iter;
            }
        }
        
        // Store min & max
        mLastRefsMinMax[ref_curve].first.first = settings.mTmin;
        mLastRefsMinMax[ref_curve].first.second = settings.mTmax;
        mLastRefsMinMax[ref_curve].second.first = min;
        mLastRefsMinMax[ref_curve].second.second = max;
    }
    return !(min == 0 && max == 0) && ((age - error < max) && (age + error > min));
}

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
