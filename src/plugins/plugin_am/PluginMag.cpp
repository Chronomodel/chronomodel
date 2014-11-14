#include "PluginMag.h"
#if USE_PLUGIN_AM

#include "StdUtilities.h"
#include "PluginMagForm.h"
#include "PluginMagRefView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>

#define DATE_AM_IS_INC_STR "is_inc"
#define DATE_AM_IS_DEC_STR "is_dec"
#define DATE_AM_IS_INT_STR "is_int"
#define DATE_AM_ERROR_STR "error"
#define DATE_AM_INC_STR "inc"
#define DATE_AM_DEC_INC_STR "dec_inc"
#define DATE_AM_DEC_DEC_STR "dec_dec"
#define DATE_AM_INTENSITY_STR "intensity"
#define DATE_AM_REF_CURVE_STR "ref_curve"



PluginMag::PluginMag()
{
    loadRefDatas();
}

float PluginMag::getLikelyhood(const float& t, const QJsonObject& data)
{
    float is_inc = data[DATE_AM_IS_INC_STR].toBool();
    float is_dec = data[DATE_AM_IS_DEC_STR].toBool();
    float is_int = data[DATE_AM_IS_INT_STR].toBool();
    float error = data[DATE_AM_ERROR_STR].toDouble();
    float inc = data[DATE_AM_INC_STR].toDouble();
    float dec_inc = data[DATE_AM_DEC_INC_STR].toDouble();
    float dec_dec = data[DATE_AM_DEC_DEC_STR].toDouble();
    float intensity = data[DATE_AM_INTENSITY_STR].toDouble();
    QString ref_curve = data[DATE_AM_REF_CURVE_STR].toString();
    
    const QMap<float, float>& curveG = mRefDatas[ref_curve]["G"];
    const QMap<float, float>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
    
    float result = 0.f;
    
    if(curveG.find(t) != curveG.end())
    {
        float g = curveG[t];
        float e = (curveG95Sup[t] - curveG[t]) / 1.96f;
        
        // pour la combinaison, il faut multiplier les 2 cas suivants :
        if(is_inc)
        {
            float variance = e * e + error * error / (2.448 * 2.448);
            result = exp(-0.5 * pow(g - inc, 2) / variance) / sqrt(variance);
        }
        else if(is_dec)
        {
            float variance = e * e + pow(error / (2.448 * cos(dec_inc * M_PI / 180.)), 2);
            result = exp(-0.5 * pow(g - dec_dec, 2) / variance) / sqrt(variance);
        }
        else if(is_int)
        {
            // TODO
            float variance = e * e + error * error;
            result = sqrt(variance) * exp(-0.5 * pow(g - intensity, 2) / variance);
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
        << "inclination? (1|0)"
        << "declination? (1|0)"
        << "intensity? (1|0)"
        << "Inclination value (if inclination)"
        << "Declination value (if declination)"
        << "Inclination value (if declination)"
        << "Intensity value (if intensity)"
        << "Error (or alpha 95)"
        << "Reference curve (file name)";
    return cols;
}


PluginFormAbstract* PluginMag::getForm()
{
    PluginMagForm* form = new PluginMagForm(this);
    return form;
}

QJsonObject PluginMag::dataFromList(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_AM_IS_INC_STR, list[1].toInt() != 0);
        json.insert(DATE_AM_IS_DEC_STR, list[2].toInt() != 0);
        json.insert(DATE_AM_IS_INT_STR, list[3].toInt() != 0);
        json.insert(DATE_AM_INC_STR, list[4].toFloat());
        json.insert(DATE_AM_DEC_DEC_STR, list[5].toFloat());
        json.insert(DATE_AM_DEC_INC_STR, list[6].toFloat());
        json.insert(DATE_AM_INTENSITY_STR, list[7].toFloat());
        json.insert(DATE_AM_ERROR_STR, list[8].toFloat());
        json.insert(DATE_AM_REF_CURVE_STR, list[9]);
    }
    return json;
}

// ------------------------------------------------------------------

QStringList PluginMag::getRefsNames() const
{
    QStringList refNames;
    typename QMap< QString, QMap<QString, QMap<float, float> > >::const_iterator it = mRefDatas.begin();
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
    path = dir.absolutePath();
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
        QFile file(files[i].absoluteFilePath());
        if(file.open(QIODevice::ReadOnly))
        {
            QMap<QString, QMap<float, float>> curves;
            
            QMap<float, float> curveG;
            QMap<float, float> curveG95Sup;
            QMap<float, float> curveG95Inf;
            
            QTextStream stream(&file);
            while(!stream.atEnd())
            {
                QString line = stream.readLine();
                
                if(line.contains("reference", Qt::CaseInsensitive) && line.contains("point", Qt::CaseInsensitive))
                {
                    // TODO : start loading points
                    break;
                }
                
                if(line.left(1) != "#" && line.left(2) != "//")
                {
                    QStringList values = line.split(",");
                    if(values.size() >= 3)
                    {
                        int t = values[0].toInt();
                        
                        float g = values[1].toFloat();
                        float gSup = g + 1.96f * values[2].toFloat();
                        float gInf = g - 1.96f * values[2].toFloat();
                        
                        curveG[t] = g;
                        curveG95Sup[t] = gSup;
                        curveG95Inf[t] = gInf;
                    }
                }
            }
            file.close();
            
            // The curves do not have 1-year precision!
            // We have to interpolate in the blanks
            
            float tmin = curveG.firstKey();
            float tmax = curveG.lastKey();
            
            for(float t=tmin; t<tmax; ++t)
            {
                if(curveG.find(t) == curveG.end())
                {
                    // This actually return the iterator with the nearest greater key !!!
                    QMap<float, float>::const_iterator iter = curveG.lowerBound(t);
                    float t_upper = iter.key();
                    --iter;
                    float t_under = iter.key();
                    
                    //qDebug() << t_under << " < " << t << " < " << t_upper;
                    
                    float g_under = curveG[t_under];
                    float g_upper = curveG[t_upper];
                    
                    float gsup_under = curveG95Sup[t_under];
                    float gsup_upper = curveG95Sup[t_upper];
                    
                    float ginf_under = curveG95Inf[t_under];
                    float ginf_upper = curveG95Inf[t_upper];
                    
                    curveG[t] = interpolate(t, t_under, t_upper, g_under, g_upper);
                    curveG95Sup[t] = interpolate(t, t_under, t_upper, gsup_under, gsup_upper);
                    curveG95Inf[t] = interpolate(t, t_under, t_upper, ginf_under, ginf_upper);
                }
            }
            
            // Store the resulting curves :
            
            curves["G"] = curveG;
            curves["G95Sup"] = curveG95Sup;
            curves["G95Inf"] = curveG95Inf;
            
            mRefDatas[files[i].fileName()] = curves;
        }
    }
}

GraphViewRefAbstract* PluginMag::getGraphViewRef()
{
    if(!mRefGraph)
        mRefGraph = new PluginMagRefView();
    return mRefGraph;
}

const QMap<QString, QMap<float, float> >& PluginMag::getRefData(const QString& name)
{
    return mRefDatas[name];
}

#endif
