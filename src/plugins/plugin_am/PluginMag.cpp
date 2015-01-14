#include "PluginMag.h"
#if USE_PLUGIN_AM

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "PluginMagForm.h"
#include "PluginMagRefView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


PluginMag::PluginMag()
{
    loadRefDatas();
}

double PluginMag::getLikelyhood(const double& t, const QJsonObject& data)
{
    double is_inc = data[DATE_AM_IS_INC_STR].toBool();
    double is_dec = data[DATE_AM_IS_DEC_STR].toBool();
    double is_int = data[DATE_AM_IS_INT_STR].toBool();
    double alpha = data[DATE_AM_ERROR_STR].toDouble();
    double inc = data[DATE_AM_INC_STR].toDouble();
    double dec = data[DATE_AM_DEC_STR].toDouble();
    double intensity = data[DATE_AM_INTENSITY_STR].toDouble();
    QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString().toLower();
    
    double result = 0.f;
    
    if(mRefDatas.find(ref_curve) != mRefDatas.end())
    {
        const QMap<double, double>& curveG = mRefDatas[ref_curve]["G"];
        const QMap<double, double>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        double t_under = floor(t);
        double t_upper = t_under + 1;
        
        if(curveG.find(t_under) != curveG.end() &&
           curveG.find(t_upper) != curveG.end())
        {
            double g_under = curveG[t_under];
            double g_upper = curveG[t_upper];
            double g = interpolate(t, t_under, t_upper, g_under, g_upper);
            
            double g_sup_under = curveG95Sup[t_under];
            double g_sup_upper = curveG95Sup[t_upper];
            double g_sup = interpolate(t, t_under, t_upper, g_sup_under, g_sup_upper);
            
            double e = (g_sup - g) / 1.96f;
            
            // pour la combinaison, il faut multiplier les 2 cas suivants :
            if(is_inc)
            {
                double variance = (e * e) + (alpha * alpha) / (2.448f * 2.448f);
                result = exp(-0.5f * pow(g - inc, 2.f) / variance) / sqrt(variance);
            }
            else if(is_dec)
            {
                double variance = e * e + pow(alpha / (2.448f * cos(inc * M_PI / 180.f)), 2.f);
                result = exp(-0.5f * pow(g - dec, 2.f) / variance) / sqrt(variance);
            }
            else if(is_int)
            {
                double error = alpha;
                double variance = e * e + error * error;
                result = exp(-0.5f * pow(g - intensity, 2.f) / variance) / sqrt(variance);
            }
        }
    }
    return result;
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
    methods.append(Date::eMHIndependant);
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

QStringList PluginMag::toCSV(const QJsonObject& data)
{
    QStringList list;
    list << (data[DATE_AM_IS_INC_STR].toBool() ? "inclination" : (data[DATE_AM_IS_DEC_STR].toBool() ? "declination" : "intensity"));
    list << QString::number(data[DATE_AM_INC_STR].toDouble());
    list << QString::number(data[DATE_AM_DEC_STR].toDouble());
    list << QString::number(data[DATE_AM_INTENSITY_STR].toDouble());
    list << QString::number(data[DATE_AM_ERROR_STR].toDouble());
    list << data[DATE_AM_REF_CURVE_STR].toString();
    return list;
}

QString PluginMag::getDateDesc(const Date* date) const
{
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
            result += QObject::tr("Inclination") + " : " + QString::number(inc);
            result += ", " + QObject::tr("Alpha95") + " : " + QString::number(alpha);
        }
        else if(is_dec)
        {
            result += QObject::tr("Declination") + " : " + QString::number(dec);
            result += ", " + QObject::tr("Alpha95") + " : " + QString::number(alpha);
        }
        else if(is_int)
        {
            result += QObject::tr("Intensity") + " : " + QString::number(intensity);
            result += ", " + QObject::tr("Error") + " : " + QString::number(alpha);
        }
        result += ", " + QObject::tr("Ref. curve : ") + " : " + ref_curve;
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

void PluginMag::loadRefDatas()
{
    QString path = QDir::currentPath();
    QString calibPath = getRefsPath();
    
    QDir calibDir(calibPath);
    
    QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
    for(int i=0; i<files.size(); ++i)
    {
        if(files[i].suffix().toLower() == "ref")
        {
            QFile file(files[i].absoluteFilePath());
            if(file.open(QIODevice::ReadOnly))
            {
                QMap<QString, QMap<double, double>> curves;
                
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
                    
                    if(!isComment(line))
                    {
                        QStringList values = line.split(",");
                        if(values.size() >= 3)
                        {
                            int t = values[0].toInt();
                            
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
            }
        }
    }
}

GraphViewRefAbstract* PluginMag::getGraphViewRef()
{
    if(!mRefGraph)
        mRefGraph = new PluginMagRefView();
    return mRefGraph;
}

const QMap<QString, QMap<double, double> >& PluginMag::getRefData(const QString& name)
{
    return mRefDatas[name.toLower()];
}

#endif
