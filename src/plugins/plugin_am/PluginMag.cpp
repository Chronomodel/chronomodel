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
double PluginMag::getLikelyhood(const double& t, const QJsonObject& data)
{
    QPair<double, double > result = getLikelyhoodArg(t, data);
    return exp(result.second) / sqrt(result.first);
}

QPair<double, double > PluginMag::getLikelyhoodArg(const double& t, const QJsonObject& data)
{
    double is_inc = data[DATE_AM_IS_INC_STR].toBool();
    double is_dec = data[DATE_AM_IS_DEC_STR].toBool();
    double is_int = data[DATE_AM_IS_INT_STR].toBool();
    double alpha = data[DATE_AM_ERROR_STR].toDouble();
    double inc = data[DATE_AM_INC_STR].toDouble();
    double dec = data[DATE_AM_DEC_STR].toDouble();
    double intensity = data[DATE_AM_INTENSITY_STR].toDouble();
    QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    
    double variance;
    double exponent;
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
    
    
    
    
    if(mRefDatas.find(ref_curve) != mRefDatas.end())
    {
        const QMap<double, double>& curveG = mRefDatas[ref_curve]["G"];
        const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        
        double tMinDef=curveG.firstKey();
        double tMaxDef=curveG.lastKey();
        double g=0;
        
        if(t>tMaxDef){
            g= curveG[tMaxDef];
            variance=10E4;
            //variance=curveG95Sup[tMaxDef]-curveG[tMaxDef];
        }
        else if (t<tMinDef){
            g= curveG[tMinDef];
            variance=10E4;
            //variance=curveG95Sup[tMinDef]-curveG[tMinDef];
        }
        else {
            double t_under = floor(t);
            double t_upper = t_under + 1;
            variance=10E4;
            if(curveG.find(t_under) != curveG.end() &&
               curveG.find(t_upper) != curveG.end()) {
                
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
        exponent=-0.5f * pow(g - mesure, 2.f) / variance;
        return qMakePair(variance, exponent);
    }
    else {
        return QPair<double,double>();
    }
    
}

bool PluginMag::isDateValid(const QJsonObject& data, const ProjectSettings& settings){
    // check valid curve
    QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    if(mRefDatas.find(ref_curve) == mRefDatas.end() && mRefDatas.end().key().toLower()!=ref_curve) {
        qDebug()<<"in PluginMag::isDateValid() unkowned curve"<<ref_curve;
        return false;
    }

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


        QMap<double, double>::const_iterator iter = curveG95Sup.constFind(settings.mTmin);
        if(iter == curveG95Sup.constEnd()){
                double t1 = curveG95Sup.firstKey();
                if(t1 < settings.mTmax){
                    iter = curveG95Sup.constBegin();
                }
                else return false;
         }

         max = iter.value();
         while(iter != curveG95Sup.constEnd() && iter.key() <= settings.mTmax) {
                max = qMax(max, iter.value());
                ++iter;
         }

         // Find min
         iter = curveG95Inf.constFind(settings.mTmin);
         if(iter == curveG95Inf.constEnd()){
                    double t1 = curveG95Inf.firstKey();
                    if(t1 < settings.mTmax){
                        iter = curveG95Inf.constBegin();
                    }
                    else return false;
          }

          min = iter.value();
          while(iter != curveG95Inf.constEnd() && iter.key() <= settings.mTmax) {
                min = qMin(min, iter.value());
                ++iter;
          }

        // Store min & max
        mLastRefsMinMax[ref_curve].first.first = settings.mTmin;
        mLastRefsMinMax[ref_curve].first.second = settings.mTmax;
        mLastRefsMinMax[ref_curve].second.first = min;
        mLastRefsMinMax[ref_curve].second.second = max;
    }
    return !(min == 0 && max == 0) && ((mesure - 1.96f*error < max) && (mesure + 1.96f*error > min));
}


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
        result += ", " + QObject::tr("Ref. curve") + " : " + ref_curve;
    }
    return result;
}

// ------------------------------------------------------------------

QStringList PluginMag::getRefsNames() const
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

QMap<QString, QMap<double, double> > PluginMag::loadRefFile(QFileInfo refFile)
{
    QFile file(refFile.absoluteFilePath());
    QMap<QString, QMap<double, double> > curves;
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {


        QMap<double, double> curveG;
        QMap<double, double> curveG95Sup;
        QMap<double, double> curveG95Inf;

        QTextStream stream(&file);
        while(!stream.atEnd()) {
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
                    int t = values[0].toInt();

                    double g = values[1].toDouble(&ok);
                    if(!ok) {
                        qDebug()<<"in PluginMag::loadRefDatas() g unvalid value";
                        continue;
                    }
                    double gSup = g + 1.96f * values[2].toDouble(&ok);
                    if(!ok) {
                        qDebug()<<"in PluginMag::loadRefDatas() gSup unvalid value";
                        continue;
                    }
                    double gInf = g - 1.96f * values[2].toDouble(&ok);
                    if(!ok) {
                        qDebug()<<"in PluginMag::loadRefDatas() gInf unvalid value";
                        continue;
                    }
                    curveG[t] = g;
                    curveG95Sup[t] = gSup;
                    curveG95Inf[t] = gInf;
                }
            }
        }
        file.close();

        // The curves do not have 1-year precision!
        // We have to interpolate in the blanks
        if(!curveG.isEmpty()) {
            double tmin = curveG.firstKey();
            double tmax = curveG.lastKey();

            for(double t=tmin; t<tmax; ++t)
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
    }
    return curves;
}


void PluginMag::loadRefDatas()
{
    QString path = QDir::currentPath();
    QString calibPath = getRefsPath();
    
    QDir calibDir(calibPath);
    
    QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
    for(int i=0; i<files.size(); ++i) {
        if(files[i].suffix().toLower() == "ref")
        {
          /*  QFile file(files[i].absoluteFilePath());
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
                            int t = values[0].toInt();
                            
                            double g = values[1].toDouble(&ok);
                            if(!ok) {
                                qDebug()<<"in PluginMag::loadRefDatas() g unvalid value";
                                continue;
                            }
                            double gSup = g + 1.96f * values[2].toDouble(&ok);
                            if(!ok) {
                                qDebug()<<"in PluginMag::loadRefDatas() gSup unvalid value";
                                continue;
                            }
                            double gInf = g - 1.96f * values[2].toDouble(&ok);
                            if(!ok) {
                                qDebug()<<"in PluginMag::loadRefDatas() gInf unvalid value";
                                continue;
                            }
                            curveG[t] = g;
                            curveG95Sup[t] = gSup;
                            curveG95Inf[t] = gInf;
                        }
                    }
                }
                file.close();
                
                // The curves do not have 1-year precision!
                // We have to interpolate in the blanks
                if(curveG.isEmpty()) continue;
                double tmin = curveG.firstKey();
                double tmax = curveG.lastKey();
                
                for(double t=tmin; t<tmax; ++t)
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

                mRefDatas[files[i].fileName().toLower()] = curves;
              */
                QMap<QString, QMap<double, double> > curves = loadRefFile(files[i].absoluteFilePath());
                if(!curves.isEmpty()) {
                   mRefDatas[files[i].fileName().toLower()] = curves;
                }
           // }
        }
    }
}

GraphViewRefAbstract* PluginMag::getGraphViewRef()
{
    /*if(!mRefGraph)
        mRefGraph = new PluginMagRefView(mLanguage);
    */
    if(mRefGraph) delete mRefGraph;
    mRefGraph = new PluginMagRefView();
    
    return mRefGraph;
}

const QMap<QString, QMap<double, double> >& PluginMag::getRefData(const QString& name)
{
    return mRefDatas[name.toLower()];
}

PluginSettingsViewAbstract* PluginMag::getSettingsView()
{
    return new PluginMagSettingsView(this);
}

#endif
