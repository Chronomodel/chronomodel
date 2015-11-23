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

double PluginGauss::getLikelyhood(const double& t, const QJsonObject& data)
{
    QPair<double, double > result = getLikelyhoodArg(t, data);
    
    return exp(result.second) / sqrt(result.first);
}

QPair<double, double > PluginGauss::getLikelyhoodArg(const double& t, const QJsonObject& data)
{
    
    double age = data[DATE_GAUSS_AGE_STR].toDouble();
    double error = data[DATE_GAUSS_ERROR_STR].toDouble();
    double a = data[DATE_GAUSS_A_STR].toDouble();
    double b = data[DATE_GAUSS_B_STR].toDouble();
    double c = data[DATE_GAUSS_C_STR].toDouble();
    QString mode = data[DATE_GAUSS_MODE_STR].toString();
    QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString();
    
    double variance;
    double exponent;
    
    if(mode == DATE_GAUSS_MODE_NONE){
        a = 0;
        b = 1;
        c = 0;
    }
    

    if(mode == DATE_GAUSS_MODE_EQ || mode == DATE_GAUSS_MODE_NONE){
        variance=sqrt(error);
        exponent=-0.5f * pow((age - (a * t * t + b * t + c)) / error, 2.f);
        
    }
    else if(mode == DATE_GAUSS_MODE_CURVE){
        // Check if calib curve exists !
        if(mRefDatas.find(ref_curve) != mRefDatas.end())
        {
            const QMap<double, double>& curveG = mRefDatas[ref_curve]["G"];
            const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
            
            double tMinDef=curveG.firstKey();
            double tMaxDef=curveG.lastKey();
            double g=0;
            
            if(t>tMaxDef){
                g=interpolate(t, tMinDef, tMaxDef, curveG[tMinDef], curveG[tMaxDef]);
                double e = (curveG95Sup[tMaxDef] - curveG[tMaxDef]) / 1.96f;
                variance = e * e + error * error;
            }
            else if (t<tMinDef){
                g=interpolate(t, tMinDef, tMaxDef, curveG[tMinDef], curveG[tMaxDef]);
                double e = (curveG95Sup[tMinDef] - curveG[tMinDef]) / 1.96f;
                variance = e * e + error * error;
            }
            else {
                double t_under = floor(t);
                double t_upper = t_under + 1;
                variance=10E4;
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
                }
            
            }
            double exponent = -0.5f * pow(g - age, 2.f) / variance;
            return qMakePair(variance,exponent);
        }
        
    }
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
    cols << "Name" << "Age" << "Error (sd)" << "a" << "b" << "c";
    return cols;
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
        json.insert(DATE_GAUSS_A_STR, list[3].toDouble());
        json.insert(DATE_GAUSS_B_STR, list[4].toDouble());
        json.insert(DATE_GAUSS_C_STR, list[5].toDouble());
        
        // TODO : for now, CSV imported data are only of equation type !
        // We need to define a CSV format to allow curve mode.
        json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_EQ));
        json.insert(DATE_GAUSS_CURVE_STR, QString(""));
    }
    return json;
}

QStringList PluginGauss::toCSV(const QJsonObject& data, const QLocale& csvLocale)
{
    QStringList list;
    list << csvLocale.toString(data[DATE_GAUSS_AGE_STR].toDouble());
    list << csvLocale.toString(data[DATE_GAUSS_ERROR_STR].toDouble());
    list << csvLocale.toString(data[DATE_GAUSS_A_STR].toDouble());
    list << csvLocale.toString(data[DATE_GAUSS_B_STR].toDouble());
    list << csvLocale.toString(data[DATE_GAUSS_C_STR].toDouble());
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
            result += ", " + QObject::tr("Ref. curve") + " : " + ref_curve;
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

QMap<QString, QMap<double, double> > PluginGauss::loadRefFile(QFileInfo refFile)
{
    QFile file(refFile.absoluteFilePath());
    QMap<QString, QMap<double, double> > curves;
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
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
                    double t = values[0].toDouble();
                    
                    double g = values[1].toDouble();
                    double gSup = g + 1.96 * values[2].toDouble();
                    double gInf = g - 1.96 * values[2].toDouble();
                    
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
                    if(iter != curveG.begin())
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
    else if(mode == DATE_GAUSS_MODE_CURVE){
       // QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString();
        // check valid curve
        QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString().toLower();
        if(mRefDatas.find(ref_curve) == mRefDatas.end()) {
            qDebug()<<"in PluginGauss::isDateValid() unkowned curve"<<ref_curve;
            return false;
        }
        const QMap<double, double>& curveG95Inf = mRefDatas[ref_curve]["G95Inf"];
        const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        QMap<double, double>::const_iterator iter = curveG95Sup.constFind(settings.mTmin);
        if(iter != curveG95Sup.constEnd()){
            double max = iter.value();
            while(iter != curveG95Sup.constEnd() && iter.key() <= settings.mTmax){
                max = qMax(max, iter.value());
                ++iter;
            }
            
            iter = curveG95Inf.constFind(settings.mTmin);
            if(iter != curveG95Inf.constEnd()){
                double min = iter.value();
                while(iter != curveG95Inf.constEnd() && iter.key() <= settings.mTmax){
                    min = qMin(min, iter.value());
                    ++iter;
                }
                return ((age - 1.96*error < max) && (age + 1.96*error > min));
            }
        }
    }
    return false;
}

#endif
