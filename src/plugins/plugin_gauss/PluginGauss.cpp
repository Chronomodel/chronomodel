#include "PluginGauss.h"
#if USE_PLUGIN_GAUSS

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "PluginGaussForm.h"
#include "PluginGaussRefView.h"
#include "PluginGaussSettingsView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


PluginGauss::PluginGauss()
{
    mColor = QColor(217,37,37);
    loadRefDatas();
}



double PluginGauss::getRefValueAt(const QJsonObject& data, const double& t)
{
    double a = data[DATE_GAUSS_A_STR].toDouble();
    double b = data[DATE_GAUSS_B_STR].toDouble();
    double c = data[DATE_GAUSS_C_STR].toDouble();
    QString mode = data[DATE_GAUSS_MODE_STR].toString();

    
    if(mode == DATE_GAUSS_MODE_NONE){
        return t;
    }
    else if(mode == DATE_GAUSS_MODE_EQ)
    {
        return a * t * t + b * t + c;
    }
    else if(mode == DATE_GAUSS_MODE_CURVE)
    {
        QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString();
        // Check if calib curve exists !
        double g = 0;
        if(mRefDatas.contains(ref_curve))
        {
            const QMap<double, double>& curve = mRefDatas[ref_curve]["G"];
            
            double tMinDef = curve.firstKey();
            double tMaxDef = curve.lastKey();
            
            if(t >= tMaxDef){
                g = interpolate(t, tMinDef, tMaxDef, curve[tMinDef], curve[tMaxDef]);
            }
            else if(t <= tMinDef){
                g = interpolate(t, tMinDef, tMaxDef, curve[tMinDef], curve[tMaxDef]);
            }
            else
            {
                double t_under = floor(t);
                double t_upper = t_under + 1;
                double g_under = curve[t_under];
                double g_upper = curve[t_upper];
                g = interpolate(t, t_under, t_upper, g_under, g_upper);
            }
        }
        return g;
    }
    else
    {
        return 0;
    }
}

double PluginGauss::getRefErrorAt(const QJsonObject& data, const double& t)
{
    QString mode = data[DATE_GAUSS_MODE_STR].toString();
    QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString();
    
    double error = 0;
    
    if(mode == DATE_GAUSS_MODE_CURVE && mRefDatas.contains(ref_curve))
    {
        const QMap<double, double>& curve = mRefDatas[ref_curve]["G"];
        const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        double tMinDef = curve.firstKey();
        double tMaxDef = curve.lastKey();
        
        if(t >= tMaxDef){
            error = 100;
        }
        else if(t <= tMinDef){
            error = 100;
        }
        else
        {
            double t_under = floor(t);
            double t_upper = t_under + 1;
            double g_under = curve[t_under];
            double g_upper = curve[t_upper];
            double g = interpolate(t, t_under, t_upper, g_under, g_upper);
            
            double g_sup_under = curveG95Sup[t_under];
            double g_sup_upper = curveG95Sup[t_upper];
            double g_sup = interpolate(t, t_under, t_upper, g_sup_under, g_sup_upper);
            
            error = (g_sup - g) / 1.96f;
        }
    }
    return error;
}

double PluginGauss::getLikelyhood(const double& t, const QJsonObject& data)
{
    QPair<double, double > result = getLikelyhoodArg(t, data);

    return exp(result.second) / sqrt(result.first);
}

QPair<double, double> PluginGauss::getLikelyhoodArg(const double& t, const QJsonObject& data)
{
    double age = data[DATE_GAUSS_AGE_STR].toDouble();
    double error = data[DATE_GAUSS_ERROR_STR].toDouble();
    
    double refValue = getRefValueAt(data, t);
    double refError = getRefErrorAt(data, t);
    
    double variance = refError * refError + error * error;
    double exponent = -0.5f * pow((age - refValue), 2.f) / variance;
    
    return qMakePair(variance, exponent);
}


QString PluginGauss::getName() const
{
    return QString("Gauss");
}
QIcon PluginGauss::getIcon() const
{
    return QIcon(":/gauss_w.png");
}
bool PluginGauss::doesCalibration() const
{
    return true;

}
bool PluginGauss::wiggleAllowed() const
{
    return false;
}
Date::DataMethod PluginGauss::getDataMethod() const
{
    return Date::eMHSymGaussAdapt;
}
QList<Date::DataMethod> PluginGauss::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHSymetric);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}
QString PluginGauss::csvHelp() const
{
    return "Calibration : g(t) = at^2 + bt + c\n";
}
QStringList PluginGauss::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Measure" << "Error (sd)" << "Ref. Curve" << "a" << "b" << "c";
    return cols;
}

int PluginGauss::csvMinColumns() const{
    return csvColumns().count() - 3;
}

PluginFormAbstract* PluginGauss::getForm()
{
    PluginGaussForm* form = new PluginGaussForm(this);
    return form;
}

/**
  * @todo for now, CSV imported data are only of equation type !
 We need to define a CSV format to allow curve mode.
*/
QJsonObject PluginGauss::fromCSV(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_GAUSS_AGE_STR, list[1].toDouble());
        json.insert(DATE_GAUSS_ERROR_STR, list[2].toDouble());
        
        if(list[3] == "equation" && list.size() >= csvMinColumns() + 3)
        {
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_EQ));
            json.insert(DATE_GAUSS_A_STR, list[4].toDouble());
            json.insert(DATE_GAUSS_B_STR, list[5].toDouble());
            json.insert(DATE_GAUSS_C_STR, list[6].toDouble());
        }
        else if(list[3] == "none" || list[3] == "")
        {
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_NONE));
        }
        else
        {
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_CURVE));
            json.insert(DATE_GAUSS_CURVE_STR, list[3]);
        }
    }
    return json;
}

QStringList PluginGauss::toCSV(const QJsonObject& data, const QLocale& csvLocale)
{
    QStringList list;
    list << csvLocale.toString(data[DATE_GAUSS_AGE_STR].toDouble());
    list << csvLocale.toString(data[DATE_GAUSS_ERROR_STR].toDouble());
    
    if(data[DATE_GAUSS_MODE_STR].toString() == DATE_GAUSS_MODE_NONE){
        list << "none";
    }
    else if(data[DATE_GAUSS_MODE_STR].toString() == DATE_GAUSS_MODE_EQ){
        list << "equation";
        list << csvLocale.toString(data[DATE_GAUSS_A_STR].toDouble());
        list << csvLocale.toString(data[DATE_GAUSS_B_STR].toDouble());
        list << csvLocale.toString(data[DATE_GAUSS_C_STR].toDouble());
    }
    else if(data[DATE_GAUSS_MODE_STR].toString() == DATE_GAUSS_MODE_CURVE){
        list << data[DATE_GAUSS_CURVE_STR].toString();
    }
    return list;
}

QString PluginGauss::getDateDesc(const Date* date) const
{
    QLocale locale=QLocale();    
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        
        double a = data[DATE_GAUSS_A_STR].toDouble();
        double b = data[DATE_GAUSS_B_STR].toDouble();
        double c = data[DATE_GAUSS_C_STR].toDouble();
        QString mode = data[DATE_GAUSS_MODE_STR].toString();
        QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString();
        
        result += QObject::tr("Age") + " : " + locale.toString(data[DATE_GAUSS_AGE_STR].toDouble());
        result += " ± " + locale.toString(data[DATE_GAUSS_ERROR_STR].toDouble());
        
        if(mode == DATE_GAUSS_MODE_NONE)
        {
            result += " (No calibration)";
        }
        if(mode == DATE_GAUSS_MODE_EQ)
        {
            QString aStr;
            if(a != 0.f)
            {
                if(a == -1.f) aStr += "-";
                else if(a != 1.f) aStr += locale.toString(a);
                aStr += "t²";
            }
            QString bStr;
            if(b != 0.f)
            {
                if(b == -1.f) bStr += "-";
                else if(b != 1.f) bStr += locale.toString(b);
                bStr += "t";
            }
            QString cStr;
            if(c != 0.f)
            {
                cStr +=locale.toString(c);
            }
            QString eq = aStr;
            if(!eq.isEmpty() && !bStr.isEmpty())
                eq += " + ";
            eq += bStr;
            if(!eq.isEmpty() && !cStr.isEmpty())
                eq += " + ";
            eq += cStr;
            
            result += ", " + QObject::tr("Ref. curve") + " : g(t) = " + eq;
        }
        else if(mode == DATE_GAUSS_MODE_CURVE)
        {            
            if(mRefDatas.contains(ref_curve) && !mRefDatas[ref_curve].isEmpty()) {
                result += ", " + tr("Ref. curve") + " : " + ref_curve;
            }
            else {
                result += ", " + tr("ERROR") +"-> "+ tr("Ref. curve") + " : " + ref_curve;
            }
        }
    }
    return result;
}

// ------------------------------------------------------------------

QString PluginGauss::getRefExt() const
{
    return "csv";
}

QString PluginGauss::getRefsPath() const
{
    QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
#endif
    QString calibPath = path + "/Calib/Gauss";
    return calibPath;
}

/**
 * @brief PluginGauss::loadRefFile the reference curve must be in english CSV mode_t
 * it's mean decimal separator is dot and value separator is coma
 * example file.csv
 * -1500,10,5
 * -1000,1500,10
 * -0,1500,10
 * 500,-1500,8
 * 1500,20,2
 * @param refFile
 * @return
 */
QMap<QString, QMap<double, double> > PluginGauss::loadRefFile(QFileInfo refFile)
{
    QFile file(refFile.absoluteFilePath());
    QMap<QString, QMap<double, double> > curves;
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMap<double, double> curveG;
        QMap<double, double> curveG95Sup;
        QMap<double, double> curveG95Inf;
        
        QLocale locale = QLocale(QLocale::English);

        QTextStream stream(&file);
        while(!stream.atEnd())
        {
            QString line = stream.readLine();
            if(!isComment(line))
            {
                QStringList values = line.split(",");
                if(values.size() >= 3)
                {
                    bool ok = true;
                    double t = locale.toDouble(values[0],&ok);
                    if(!ok) continue;
                    double g = locale.toDouble(values[1],&ok);
                    if(!ok) continue;
                    double gSup = g + 1.96 * locale.toDouble(values[2],&ok);
                    if(!ok) continue;
                    double gInf = g - 1.96 * locale.toDouble(values[2],&ok);
                    if(!ok) continue;
                    
                    curveG[t] = g;
                    curveG95Sup[t] = gSup;
                    curveG95Inf[t] = gInf;
                }
            }
        }
        file.close();
        // it is not a valid file
        if(curveG.isEmpty()) return curves;
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
                    if(iter != curveG.begin())
                    {
                        --iter;
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
                    else
                    {
                        curveG[t] = 0;
                        curveG95Sup[t] = 0;
                        curveG95Inf[t] = 0;
                    }
                }
                else
                {
                    curveG[t] = 0;
                    curveG95Sup[t] = 0;
                    curveG95Inf[t] = 0;
                }
            }
        }
        
        // Store the resulting curves :
        
        curves["G"] = curveG;
        curves["G95Sup"] = curveG95Sup;
        curves["G95Inf"] = curveG95Inf;
    }
    return curves;
}

QPair<double,double> PluginGauss::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    double tmin=0;
    double tmax=0;
    if(data[DATE_GAUSS_MODE_STR].toString()==DATE_GAUSS_MODE_CURVE){
        QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString().toLower();
        if( mRefDatas.contains(ref_curve)  && !mRefDatas[ref_curve].isEmpty() ) {

           tmin= mRefDatas[ref_curve]["G"].firstKey();
           tmax= mRefDatas[ref_curve]["G"].lastKey();
        }
        else{
            qDebug()<<"PluginGauss::getTminTmaxRefsCurve no ref curve";
        }
    }
    return qMakePair<double,double>(tmin,tmax);
}

// ------------------------------------------------------------------

GraphViewRefAbstract* PluginGauss::getGraphViewRef()
{
    //if(!mRefGraph) mRefGraph = new PluginGaussRefView(mLanguage);

    if(mRefGraph) delete mRefGraph;
    mRefGraph = new PluginGaussRefView();
    
    return mRefGraph;
}

PluginSettingsViewAbstract* PluginGauss::getSettingsView()
{
    return new PluginGaussSettingsView(this);
}

// ------------------------------------------------------------------

QJsonObject PluginGauss::checkValuesCompatibility(const QJsonObject& values){
    QJsonObject result = values;
    if(!values.contains(DATE_GAUSS_MODE_STR)){
        result.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_EQ));
    }
    return result;
}

bool PluginGauss::isDateValid(const QJsonObject& data, const ProjectSettings& settings){
    
    double age = data[DATE_GAUSS_AGE_STR].toDouble();
    double error = data[DATE_GAUSS_ERROR_STR].toDouble();
    
    QString mode = data[DATE_GAUSS_MODE_STR].toString();
    
    if(mode == DATE_GAUSS_MODE_NONE){
        return ((age - error < settings.mTmax) && (age + error > settings.mTmin));
    }
    else if(mode == DATE_GAUSS_MODE_EQ){
        double a = data[DATE_GAUSS_A_STR].toDouble();
        double b = data[DATE_GAUSS_B_STR].toDouble();
        double c = data[DATE_GAUSS_C_STR].toDouble();
        
        QVector<double> refValues;
        for(double t=settings.mTmin; t<=settings.mTmax; t+=settings.mStep)
            refValues.push_back(a * t * t + b * t + c);
        
        double min = vector_min_value(refValues);
        double max = vector_max_value(refValues);
        
        return ((age - error < max) && (age + error > min));
    }
    else if(mode == DATE_GAUSS_MODE_CURVE)
    {
        QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString().toLower();
        if(!mRefDatas.contains(ref_curve)) {
            qDebug()<<"in PluginGauss::isDateValid() unkowned curve"<<ref_curve;
            QMessageBox::warning(qApp->activeWindow(),tr("Curve error"),"in PluginGauss unkowned curve : "+ref_curve);
            return false;
        }
        double min = 0;
        double max = 0;
        
        if(mLastRefsMinMax.contains(ref_curve)
           && mLastRefsMinMax[ref_curve].first.first == settings.mTmin
           && mLastRefsMinMax[ref_curve].first.second == settings.mTmax)
        {
            min = mLastRefsMinMax[ref_curve].second.first;
            max = mLastRefsMinMax[ref_curve].second.second;
        }
        else
        {
            const QMap<double, double>& curveG95Inf = mRefDatas[ref_curve]["G95Inf"];
            const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
            
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
        return ((age - 1.96*error < max) && (age + 1.96*error > min));
    }
    else
    {
        return false;
    }
}

#endif
