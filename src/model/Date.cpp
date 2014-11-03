#include "Date.h"
#include "Event.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include <QDebug>


Date::Date()
{
    init();
}

Date::Date(PluginAbstract* plugin)
{
    init();
    mPlugin = plugin;
}

void Date::init()
{
    mId = 0;
    mMethod = eMHIndependant;
    mDeltaType = eDeltaFixed;
    mDeltaFixed = 0;
    mDeltaMin = 0;
    mDeltaMax = 0;
    mDeltaAverage = 0;
    mDeltaError = 0;
    mIsCurrent = false;
    mIsSelected = false;
}

Date::Date(const Date& date)
{
    copyFrom(date);
}

Date& Date::operator=(const Date& date)
{
    copyFrom(date);
    return *this;
}

void Date::copyFrom(const Date& date)
{
    mTheta = date.mTheta;
    mSigma = date.mSigma;
    mDelta = date.mDelta;
    
    mId = date.mId;
    mName = date.mName;
    mData = date.mData;
    mPlugin = date.mPlugin;
    mMethod = date.mMethod;
    
    mDeltaType = date.mDeltaType;
    mDeltaFixed = date.mDeltaFixed;
    mDeltaMin = date.mDeltaMin;
    mDeltaMax = date.mDeltaMax;
    mDeltaAverage = date.mDeltaAverage;
    mDeltaError = date.mDeltaError;
    
    mIsCurrent = date.mIsCurrent;
    mIsSelected = date.mIsSelected;
    
    mCalibration = date.mCalibration;
    mRepartition = date.mRepartition;
    mCalibHPD = date.mCalibHPD;
    
    mCalibThumb = date.mCalibThumb;
    mSubDates = date.mSubDates;
}

Date::~Date()
{
    
}

bool Date::isNull()
{
    return mData.isEmpty() || (mPlugin == 0);
}

Date Date::fromJson(const QJsonObject& json)
{
    Date date;
    
    if(!json.isEmpty())
    {
        date.mId = json[STATE_DATE_ID].toInt();
        date.mName = json[STATE_DATE_NAME].toString();
        date.mData = json[STATE_DATE_DATA].toObject();
        date.mMethod = (DataMethod)json[STATE_DATE_METHOD].toInt();
        
        date.mDeltaType = (Date::DeltaType)json[STATE_DATE_DELTA_TYPE].toInt();
        date.mDeltaFixed = json[STATE_DATE_DELTA_FIXED].toDouble();
        date.mDeltaMin = json[STATE_DATE_DELTA_MIN].toDouble();
        date.mDeltaMax = json[STATE_DATE_DELTA_MAX].toDouble();
        date.mDeltaAverage = json[STATE_DATE_DELTA_AVERAGE].toDouble();
        date.mDeltaError = json[STATE_DATE_DELTA_ERROR].toDouble();
        
        date.mPlugin = PluginManager::getPluginFromId(json[STATE_DATE_PLUGIN_ID].toString());
    }
    
    return date;
}

QJsonObject Date::toJson() const
{
    QJsonObject date;
    date[STATE_DATE_ID] = mId;
    date[STATE_DATE_NAME] = mName;
    date[STATE_DATE_DATA] = mData;
    date[STATE_DATE_PLUGIN_ID] = mPlugin->getId();
    date[STATE_DATE_METHOD] = mMethod;
    
    date[STATE_DATE_DELTA_TYPE] = mDeltaType;
    date[STATE_DATE_DELTA_FIXED] = mDeltaFixed;
    date[STATE_DATE_DELTA_MIN] = mDeltaMin;
    date[STATE_DATE_DELTA_MAX] = mDeltaMax;
    date[STATE_DATE_DELTA_AVERAGE] = mDeltaAverage;
    date[STATE_DATE_DELTA_ERROR] = mDeltaError;
    
    return date;
}

float Date::getLikelyhood(const float& t)
{
    float result = 0.f;
    if(mPlugin)
        result = mPlugin->getLikelyhood(t, mData);
    return result;
}

QString Date::getDesc() const
{
    QStringList params;
    QJsonObject::const_iterator iter;
    for(iter = mData.begin(); iter!=mData.end(); ++iter)
    {
        QString val;
        if(iter.value().isString())
            val = iter.value().toString();
        else if(iter.value().isDouble())
            val = QString::number(iter.value().toDouble());
        else if(iter.value().isBool())
            val = iter.value().toBool() ? QObject::tr("yes") : QObject::tr("no");
        params << iter.key() + " = " + val;
    }
    return params.join(", ");
}

void Date::reset()
{
    mTheta.reset();
    mSigma.reset();
    mCalibration.clear();
    mRepartition.clear();
}

void Date::calibrate(const float& tmin, const float& tmax, const float& step)
{
    mCalibration.clear();
    mRepartition.clear();
    mCalibHPD.clear();
    
    if(mSubDates.size() == 0)
    {
        mCalibration[tmin] = getLikelyhood(tmin);
        mRepartition[tmin] = mCalibration[tmin];
        
        for(int t=tmin; t<=tmax; t += step)
        {
            float v = getLikelyhood(t);
            mCalibration[t] = v;
            mRepartition[t] = mRepartition[t-1] + v;
        }
    }
    else
    {
        for(int i=0; i<mSubDates.size(); ++i)
        {
            mSubDates[i]->calibrate(tmin, tmax, step);
        }
        
        float v = 1.f;
        for(int i=0; i<mSubDates.size(); ++i)
            v *= mSubDates[i]->getLikelyhoodFromCalib(tmin);
        mCalibration[tmin] = v;
        mRepartition[tmin] = mCalibration[tmin];
        
        for(int t=tmin+1; t<=tmax; ++t)
        {
            v = 1.f;
            for(int i=0; i<mSubDates.size(); ++i)
                v *= mSubDates[i]->getLikelyhoodFromCalib(t);
                
            mCalibration[t] = v;
            mRepartition[t] = mRepartition[t-1] + v;
        }
        
        for(int i=tmin+1; i<=tmax; ++i)
        {
            float v = getLikelyhood(i);
            mCalibration[i] = v;
            mRepartition[i] = mRepartition[i-1] + v;
        }
    }
    
    //mCalibration = QMap<float, float>(normalize_map(mCalibration));
    mRepartition = QMap<float, float>(normalize_map(mRepartition));
    
    GraphView* graph = new GraphView();
    graph->setFixedSize(200, 50);
    graph->setRangeX(tmin, tmax);
    graph->setRangeY(0, 1);
    graph->showAxis(false);
    graph->showScrollBar(false);
    graph->showGrid(false);
    
    GraphCurve curve;
    curve.mData = mCalibration;
    curve.mName = "Calibration";
    curve.mPen.setColor(Qt::blue);
    curve.mPen.setWidth(1);
    curve.mFillUnder = true;
    graph->addCurve(curve);
    
    mCalibThumb = QPixmap(graph->size());
    graph->render(&mCalibThumb);
    //mCalibThumb.save("test.png");
    //mCalibThumb = graph.grab();
}

float Date::getLikelyhoodFromCalib(const float t)
{
    if(mCalibration.find(t) != mCalibration.end())
    {
        return mCalibration[t];
    }
    else
    {
        float t_under = floorf(t);
        float t_upper = ceilf(t);
        
        float value_under = mCalibration[t_under];
        float value_upper = mCalibration[t_upper];
        
        float v = interpolate(t, t_under, t_upper, value_under, value_upper);
        return v;
    }
}

void Date::updateTheta(const float& tmin, const float& tmax, Event& event)
{
    switch(mMethod)
    {
        case eMHIndependant:
        {
            // Ici, le marcheur est forcément gaussien avec H(theta i) : double_exp (gaussien tronqué)
            float theta = Generator::gaussByDoubleExp(event.mTheta.mX + mDelta.mX, mSigma.mX, tmin, tmax);
            
            // rapport = G(theta_new) / G(theta_old)
            float rapport = getLikelyhoodFromCalib(theta) / getLikelyhoodFromCalib(mTheta.mX);
            
            mTheta.tryUpdate(theta, rapport);
            break;
        }
        case eInversion:
        {
            // 3eme méthode : marche aléatoire G(theta i),
            // utilisation de la courbe cumulative avec interpolation linéaire
            float u = Generator::randomUniform();
            float theta = map_interpolate_key_for_value(u, mRepartition);
            
            // rapport = H(theta_new) / H(theta_old)
            float rapport = exp((-0.5/(mSigma.mX * mSigma.mX)) * (pow(theta - (event.mTheta.mX + mDelta.mX), 2) - pow(mTheta.mX - (event.mTheta.mX + mDelta.mX), 2)));
            
            mTheta.tryUpdate(theta, rapport);
            break;
        }
            // Seul cas où le taux d'acceptation a du sens car on utilise sigmaMH :
        case eMHSymGaussAdapt:
        {
            float theta = Generator::gaussByBoxMuller(mTheta.mX, mTheta.mSigmaMH);
            
            float rapport = 0;
            if(theta >= tmin && theta <= tmax)
            {
                // rapport = (G(theta_new) / G(theta_old)) * (H(theta_new) / H(theta_old))
                rapport = getLikelyhoodFromCalib(theta) / getLikelyhoodFromCalib(mTheta.mX); // rapport des G(theta i)
                rapport *= exp((-0.5/(mSigma.mX * mSigma.mX)) * (pow(theta - (event.mTheta.mX + mDelta.mX), 2) - pow(mTheta.mX - (event.mTheta.mX + mDelta.mX), 2)));
            }
            
            mTheta.tryUpdate(theta, rapport);
            break;
        }
        default:
        {
            break;
        }
    }
}

void Date::updateDelta()
{
    switch(mDeltaType)
    {
        case eDeltaRange:
        {
            // TODO
            mDelta.tryUpdate(mDeltaFixed, 1);
            break;
        }
        case eDeltaGaussian:
        {
            // TODO
            mDelta.tryUpdate(mDeltaFixed, 1);
            break;
        }
        case eDeltaFixed:
        default:
        {
            mDelta.tryUpdate(mDeltaFixed, 1);
            break;
        }
    }
}

void Date::updateSigma(Event& event)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    float lambda = pow(mTheta.mX - (event.mTheta.mX + mDelta.mX), 2) / 2.;
    
    const int logVMin = -6;
    const int logVMax = 100;
    
    float V1 = mSigma.mX * mSigma.mX;
    float logV2 = Generator::gaussByBoxMuller(log10(V1), mSigma.mSigmaMH);
    float V2 = pow(10, logV2);
    
    float rapport = 0;
    if(logV2 >= logVMin && logV2 <= logVMax)
    {
        float x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
        float x2 = pow((event.mS02 + V1) / (event.mS02 + V2), event.mAShrinkage + 1);
        rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1; // (V2 / V1) est le jacobien!
    }
    mSigma.tryUpdate(sqrt(V2), rapport);
}

