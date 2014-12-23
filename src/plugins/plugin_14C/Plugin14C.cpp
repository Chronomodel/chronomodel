#include "Plugin14C.h"
#if USE_PLUGIN_14C

#include "QtUtilities.h"
#include "StdUtilities.h"
#include "Plugin14CForm.h"
#include "Plugin14CRefView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


Plugin14C::Plugin14C()
{
    loadRefDatas();
}

float Plugin14C::getLikelyhood(const float& t, const QJsonObject& data)
{
    float age = data[DATE_14C_AGE_STR].toDouble();
    float error = data[DATE_14C_ERROR_STR].toDouble();
    QString ref_curve = data[DATE_14C_REF_CURVE_STR].toString().toLower();
    
    float result = 0;
    
    // Check if calib curve exists !
    if(mRefDatas.find(ref_curve) != mRefDatas.end())
    {
        const QMap<float, float>& curveG = mRefDatas[ref_curve]["G"];
        const QMap<float, float>& curveG95Sup = mRefDatas[ref_curve]["G95Sup"];
        
        float t_under = floorf(t);
        float t_upper = t_under + 1;
        
        if(curveG.find(t_under) != curveG.end() &&
           curveG.find(t_upper) != curveG.end())
        {
            float g_under = curveG[t_under];
            float g_upper = curveG[t_upper];
            float g = interpolate(t, t_under, t_upper, g_under, g_upper);
            
            float g_sup_under = curveG95Sup[t_under];
            float g_sup_upper = curveG95Sup[t_upper];
            float g_sup = interpolate(t, t_under, t_upper, g_sup_under, g_sup_upper);
            
            float e = (g_sup - g) / 1.96f;
            float variance = e * e + error * error;
            
            result = expf(-0.5f * powf(g - age, 2.f) / variance) / sqrtf(variance);
        }
        else
        {
            //qDebug() << "failed";
        }
    }
    
    return result;
}


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
    methods.append(Date::eMHIndependant);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}
QStringList Plugin14C::csvColumns() const
{
    QStringList cols;
    cols << "Name" << "Age" << "Error" << "Reference curve";
    return cols;
}


PluginFormAbstract* Plugin14C::getForm()
{
    Plugin14CForm* form = new Plugin14CForm(this);
    return form;
}

QJsonObject Plugin14C::dataFromList(const QStringList& list)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_14C_AGE_STR, list[1].toDouble());
        json.insert(DATE_14C_ERROR_STR, list[2].toDouble());
        json.insert(DATE_14C_REF_CURVE_STR, list[3].toLower());
        
        qDebug() << list;
        qDebug() << json;
    }
    return json;
}

QString Plugin14C::getDateDesc(const Date* date) const
{
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        result += QObject::tr("Age") + " : " + QString::number(data[DATE_14C_AGE_STR].toDouble());
        result += " ± " + QString::number(data[DATE_14C_ERROR_STR].toDouble());
        result += ", " + QObject::tr("Ref. curve") + " : " + data[DATE_14C_REF_CURVE_STR].toString().toLower();
    }
    return result;
}

// ------------------------------------------------------------------

QStringList Plugin14C::getRefsNames() const
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

void Plugin14C::loadRefDatas()
{
    QString calibPath = getRefsPath();
    QDir calibDir(calibPath);
    
    QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
    for(int i=0; i<files.size(); ++i)
    {
        if(files[i].suffix().toLower() == "14c")
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
                    if(!isComment(line))
                    {
                        QStringList values = line.split(",");
                        if(values.size() >= 3)
                        {
                            int t = 1950 - values[0].toInt();
                            
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
                        if(iter != curveG.end())
                        {
                            float t_upper = iter.key();
                            --iter;
                            if(iter != curveG.begin())
                            {
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

GraphViewRefAbstract* Plugin14C::getGraphViewRef()
{
    if(!mRefGraph)
        mRefGraph = new Plugin14CRefView();
    return mRefGraph;
}

const QMap<QString, QMap<float, float> >& Plugin14C::getRefData(const QString& name)
{
    return mRefDatas[name.toLower()];
}

#endif
