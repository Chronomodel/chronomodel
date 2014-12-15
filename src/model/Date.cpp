#include "Date.h"
#include "Event.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include "Painting.h"
#include "ModelUtilities.h"
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
    mDelta = 0;
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
        qDebug() << json;
        
        date.mId = json[STATE_ID].toInt();
        date.mName = json[STATE_NAME].toString();
        date.mData = json[STATE_DATE_DATA].toObject();
        date.mMethod = (DataMethod)json[STATE_DATE_METHOD].toInt();
        
        date.mDeltaType = (Date::DeltaType)json[STATE_DATE_DELTA_TYPE].toInt();
        date.mDeltaFixed = json[STATE_DATE_DELTA_FIXED].toDouble();
        date.mDeltaMin = json[STATE_DATE_DELTA_MIN].toDouble();
        date.mDeltaMax = json[STATE_DATE_DELTA_MAX].toDouble();
        date.mDeltaAverage = json[STATE_DATE_DELTA_AVERAGE].toDouble();
        date.mDeltaError = json[STATE_DATE_DELTA_ERROR].toDouble();
        
        date.mPlugin = PluginManager::getPluginFromId(json[STATE_DATE_PLUGIN_ID].toString());
        
        date.mTheta.mProposal = ModelUtilities::getDataMethodText(date.mMethod);
    }
    
    return date;
}

QJsonObject Date::toJson() const
{
    QJsonObject date;
    date[STATE_ID] = mId;
    date[STATE_NAME] = mName;
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
    if(mPlugin)
    {
        return mPlugin->getDateDesc(this);
    }
    else
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
}

void Date::reset()
{
    mTheta.reset();
    mSigma.reset();
    mCalibration.clear();
    mRepartition.clear();
    mWiggle.reset();
}

void Date::calibrate(const float tmin, const float tmax, const float step)
{
    mCalibration.clear();
    mRepartition.clear();
    mCalibHPD.clear();
    
    if(mSubDates.size() == 0)
    {
        float lastRepVal = 0.f;
        
        for(float t=tmin; t<=tmax; t += step)
        {
            float v = getLikelyhood(t);
            mCalibration[t] = v;
            
            float repVal = lastRepVal + v;
            mRepartition[t] = repVal;
            lastRepVal = repVal;
        }
        mRepartition = normalize_map(mRepartition);
        mCalibration = equal_areas(mCalibration, 1.f);
    }
    else
    {
        // TODO
    }
}

QPixmap Date::generateCalibThumb(const float tmin, const float tmax)
{
    GraphView* graph = new GraphView();
    graph->setFixedSize(200, 30);
    graph->setRangeX(tmin, tmax);
    graph->setRangeY(0, 1.1f);
    graph->showAxis(false);
    graph->showScrollBar(false);
    graph->showGrid(false);
    graph->setMargins(5, 5, 5, 5);
    
    GraphCurve curve;
    curve.mData = normalize_map(mCalibration);
    curve.mName = "Calibration";
    curve.mPen.setColor(Painting::mainColorLight);
    curve.mPen.setWidthF(2.f);
    curve.mFillUnder = true;
    graph->addCurve(curve);
    
    QPixmap thumb(graph->size());
    graph->render(&thumb);
    delete graph;
    
    return thumb;
    //thumb.save("test.png");
    //thumb = graph.grab();
}

float Date::getLikelyhoodFromCalib(const float t, const float step)
{
    if(mCalibration.find(t) != mCalibration.end())
    {
        return mCalibration[t];
    }
    else
    {
        float t_under = floorf(t);
        float t_upper = ceilf(t);
        
        if(step > 1.f)
        {
            t_under = floorf(t_under / step) * step;
            t_upper = t_under + step;
        }
        
        if(mCalibration.find(t_under) == mCalibration.end())
            qDebug() << "ERROR : calling calib on unknown t_under";
        if(mCalibration.find(t_upper) == mCalibration.end())
            qDebug() << "ERROR : calling calib on unknown t_upper";
        
        float value_under = mCalibration[t_under];
        float value_upper = mCalibration[t_upper];
        
        float v = interpolate(t, t_under, t_upper, value_under, value_upper);
        return v;
    }
}

void Date::updateTheta(const float& tmin, const float& tmax, const float& step, Event* event)
{
    switch(mMethod)
    {
        case eMHIndependant:
        {
            // Ici, le marcheur est forcément gaussien avec H(theta i) : double_exp (gaussien tronqué)
            float theta = Generator::gaussByDoubleExp(event->mTheta.mX - mDelta, mSigma.mX, tmin, tmax);
            
            // rapport = G(theta_new) / G(theta_old)
            float rapport = getLikelyhoodFromCalib(theta, step) / getLikelyhoodFromCalib(mTheta.mX, step);
            
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
            float rapport = expf((-0.5/(mSigma.mX * mSigma.mX)) * (powf(theta - (event->mTheta.mX - mDelta), 2) - powf(mTheta.mX - (event->mTheta.mX - mDelta), 2)));
            
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
                rapport = getLikelyhoodFromCalib(theta, step) / getLikelyhoodFromCalib(mTheta.mX, step); // rapport des G(theta i)
                rapport *= expf((-0.5/(mSigma.mX * mSigma.mX)) * (powf(theta - (event->mTheta.mX - mDelta), 2) - powf(mTheta.mX - (event->mTheta.mX - mDelta), 2)));
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

void Date::initDelta(Event* event)
{
    switch(mDeltaType)
    {
        case eDeltaRange:
        {
            mDelta = Generator::randomUniform(mDeltaMin, mDeltaMax);
            break;
        }
        case eDeltaGaussian:
        {
            mDelta = event->mTheta.mX - mTheta.mX;
            break;
        }
        case eDeltaFixed:
        default:
        {
            mDelta = mDeltaFixed;
            break;
        }
    }
}

void Date::updateDelta(Event* event)
{
    switch(mDeltaType)
    {
        case eDeltaRange:
        {
            float delta = -1;
            while(delta < mDeltaMin || delta > mDeltaMax)
            {
                float x = Generator::gaussByBoxMuller(0, 1);
                float lambda = event->mTheta.mX - mTheta.mX;
                delta = mSigma.mX * x + lambda;
            }
            mDelta = delta;
            break;
        }
        case eDeltaGaussian:
        {
            float lambda = event->mTheta.mX - mTheta.mX;
            float w = (1/(mSigma.mX * mSigma.mX)) + (1/(mDeltaError * mDeltaError));
            float deltaAvg = (lambda / (mSigma.mX * mSigma.mX) + mDeltaAverage / (mDeltaError * mDeltaError)) / w;
            float x = Generator::gaussByBoxMuller(0, 1);
            float delta = deltaAvg + x / sqrtf(w);
            
            mDelta = delta;
            break;
        }
        case eDeltaFixed:
        default:
        {
            mDelta = mDeltaFixed;
            break;
        }
    }
}

void Date::updateSigma(Event* event)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    float lambda = powf(mTheta.mX - (event->mTheta.mX - mDelta), 2) / 2.;
    
    const int logVMin = -6;
    const int logVMax = 100;
    
    float V1 = mSigma.mX * mSigma.mX;
    float logV2 = Generator::gaussByBoxMuller(log10(V1), mSigma.mSigmaMH);
    float V2 = powf(10, logV2);
    
    float rapport = 0;
    if(logV2 >= logVMin && logV2 <= logVMax)
    {
        float x1 = expf(-lambda * (V1 - V2) / (V1 * V2));
        float x2 = powf((event->mS02 + V1) / (event->mS02 + V2), event->mAShrinkage + 1);
        rapport = x1 * sqrtf(V1/V2) * x2 * V2 / V1; // (V2 / V1) est le jacobien!
    }
    mSigma.tryUpdate(sqrtf(V2), rapport);
}

void Date::updateWiggle()
{
    mWiggle.mX = mTheta.mX + mDelta;
}

