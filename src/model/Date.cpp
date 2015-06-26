#include "Date.h"
#include "Event.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include <QDebug>


Date::Date()
{
    mTheta.mIsDate = true;
    mSigma.mIsDate = false;
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
    mCalibSum = 0;
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
        date.mId = json[STATE_ID].toInt();
        date.mName = json[STATE_NAME].toString();
        //qDebug() <<"date.name" << date.mName;
        
        // Copy plugin specific values for this data :
        date.mData = json[STATE_DATE_DATA].toObject();
        
        date.mMethod = (DataMethod)json[STATE_DATE_METHOD].toInt();
        
        date.mDeltaType = (Date::DeltaType)json[STATE_DATE_DELTA_TYPE].toInt();
        date.mDeltaFixed = json[STATE_DATE_DELTA_FIXED].toDouble();
        date.mDeltaMin = json[STATE_DATE_DELTA_MIN].toDouble();
        date.mDeltaMax = json[STATE_DATE_DELTA_MAX].toDouble();
        date.mDeltaAverage = json[STATE_DATE_DELTA_AVERAGE].toDouble();
        date.mDeltaError = json[STATE_DATE_DELTA_ERROR].toDouble();
        
        QString pluginId = json[STATE_DATE_PLUGIN_ID].toString();
        date.mPlugin = PluginManager::getPluginFromId(pluginId);
        if(date.mPlugin == 0)
        {
            throw QObject::tr("Data could not be loaded : invalid plugin : ") + pluginId;
        }
        
        date.mTheta.mProposal = ModelUtilities::getDataMethodText(date.mMethod);
        date.mSigma.mProposal = ModelUtilities::getDataMethodText(Date::eMHSymGaussAdapt);
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

double Date::getLikelyhood(const double& t)
{
    double result = 0.f;
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

void Date::calibrate(const ProjectSettings& settings)
{
    mCalibration.clear();
    mRepartition.clear();
    mCalibHPD.clear();
    mSettings = settings;
    mCalibSum = 0;
   // mCalibration.erase(mCalibration.begin(), mCalibration.end());
    double tmin = mSettings.mTmin;
    double tmax = mSettings.mTmax;
    double step = mSettings.mStep;
    double nbPts = 1 + round((tmax - tmin) / step);
  //  qDebug()<<" Date::calibrate"<<tmin<<tmax<<step<<nbPts<<"size"<<mCalibration.size();
    if(mSubDates.size() == 0) // not a combination !
    {
        double v = getLikelyhood(tmin);
        double lastRepVal = v;
        
        mCalibration.append(v);
        mRepartition.append(0);
        mCalibSum += v;
        
        for(int i = 1; i < nbPts; ++i)
        {
            double t = tmin + (double)i * step;
            
            float lastV = v;
            v = getLikelyhood(t);
            mCalibration.append(v);
            mCalibSum += v;
            
            double rep = lastRepVal;
            if(v != 0 && lastV != 0)
            {
                rep = lastRepVal + step * (lastV + v) / 2.;
            }
            mRepartition.append(rep);
            lastRepVal = rep;
        }
        
        // La courbe de répartition est transformée de sorte que sa valeur maximale soit 1
        mRepartition = normalize_vector(mRepartition);
        
        // La courbe de calibration est transformée de sorte que l'aire sous la courbe soit 1
        mCalibration = equal_areas(mCalibration, step, 1.);
          //  qDebug()<<" Date::calibrate end"<<tmin<<tmax<<step<<nbPts<<"size"<<mCalibration.size();
    }
    else
    {
        // TODO : combination
    }
}

QMap<double, double> Date::getCalibMap() const
{
 
    return vector_to_map(mCalibration, mSettings.mTmin, mSettings.mTmax, mSettings.mStep);
}

QPixmap Date::generateCalibThumb()
{
    double tmin = mSettings.mTmin;
    double tmax = mSettings.mTmax;
   // qDebug()<<" Date::generateCalibThumb"<<tmin<<tmax<<mSettings.mStep;
    GraphView* graph = new GraphView();
    graph->setFixedSize(200, 30);
    graph->setMargins(0, 0, 0, 0);
    
    graph->setRangeX(tmin, tmax);
    graph->setCurrentX(tmin, tmax);
    graph->setRangeY(0, 1.1f);
    
    graph->showAxisArrows(false);
    graph->showAxisLines(false);
    graph->setXAxisMode(GraphView::eHidden);
    graph->setYAxisMode(GraphView::eHidden);
    
    QColor color = mPlugin->getColor();//  Painting::mainColorLight;
    QColor HPDColor(color);
    //HPDColor.setAlpha(100);
    
    GraphCurve curve;
    //QMap<double, double> mDataCalib;
   // mDataCalib  = getCalibMap();
    curve.mData = normalize_map(getCalibMap());
    
    curve.mName = "Calibration";
    curve.mPen.setColor(color);
    curve.mPen.setWidthF(2.f);
    curve.mBrush.setColor(HPDColor);
    curve.mFillUnder = true;
    curve.mIsHisto = false;
    curve.mIsRectFromZero = true; // For Typo !!
    
    graph->addCurve(curve);
    graph->repaint();
    QPixmap thumb(graph->size());
    QPainter p;
    p.begin(&thumb);
    graph->render(&p);
    p.end();
   // delete graph;
    
    return thumb;
    //thumb.save("test.png");
    //thumb = graph.grab();
}

double Date::getLikelyhoodFromCalib(const double t)
{
    double tmin = mSettings.mTmin;
    double tmax = mSettings.mTmax;
    
    // We need at least two points to interpolate
    if(mCalibration.size() < 2 || t < tmin || t > tmax)
        return 0;
    
    double prop = (t - tmin) / (tmax - tmin);
    double idx = prop * (mCalibration.size() - 1); // tricky : if (tmax - tmin) = 2000, then calib size is 2001 !
    int idxUnder = (int)floor(idx);
    int idxUpper = idxUnder + 1;
    
    // Important pour le créneau : pas d'interpolation autour des créneaux!
    double v = 0.;
    if(mCalibration[idxUnder] != 0 && mCalibration[idxUpper] != 0)
        v = interpolate(idx, (double)idxUnder, (double)idxUpper, mCalibration[idxUnder], mCalibration[idxUpper]);
    return v;
}

void Date::updateTheta(Event* event)
{
    double tmin = mSettings.mTmin;
    double tmax = mSettings.mTmax;
    double step = mSettings.mStep;
    
    switch(mMethod)
    {
        case eMHIndependant:
        {
            // Ici, le marcheur est forcément gaussien avec H(theta i) : double_exp (gaussien tronqué)
            double theta = Generator::gaussByDoubleExp(event->mTheta.mX - mDelta, mSigma.mX, tmin, tmax);
            
            // rapport = G(theta_new) / G(theta_old)
            double rapport = getLikelyhoodFromCalib(theta) / getLikelyhoodFromCalib(mTheta.mX);
            
            mTheta.tryUpdate(theta, rapport);
            break;
        }
        case eInversion:
        {
            // 3eme méthode : marche aléatoire G(theta i),
            // utilisation de la courbe cumulative avec interpolation linéaire
            double u = Generator::randomUniform();
            double idx = vector_interpolate_idx_for_value(u, mRepartition);
            double theta = tmin + idx * step;
            
            // rapport = H(theta_new) / H(theta_old)
            double rapport = exp((-0.5/(mSigma.mX * mSigma.mX)) * (pow(theta - (event->mTheta.mX - mDelta), 2) - pow(mTheta.mX - (event->mTheta.mX - mDelta), 2)));
            
            mTheta.tryUpdate(theta, rapport);
            break;
        }
            // Seul cas où le taux d'acceptation a du sens car on utilise sigmaMH :
        case eMHSymGaussAdapt:
        {
            double theta = Generator::gaussByBoxMuller(mTheta.mX, mTheta.mSigmaMH);
            
            double rapport = 0;
            if(theta >= tmin && theta <= tmax)
            {
                // rapport = (G(theta_new) / G(theta_old)) * (H(theta_new) / H(theta_old))
                rapport = getLikelyhoodFromCalib(theta) / getLikelyhoodFromCalib(mTheta.mX); // rapport des G(theta i)
                rapport *= exp((-0.5/(mSigma.mX * mSigma.mX)) * (pow(theta - (event->mTheta.mX - mDelta), 2) - pow(mTheta.mX - (event->mTheta.mX - mDelta), 2)));
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

void Date::initDelta(Event*)
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
            // change init of Delta in case of gaussian function since 2015/06 with PhL
            //mDelta = event->mTheta.mX - mTheta.mX;
            double tmin = mSettings.mTmin;
            double tmax = mSettings.mTmax;
            mDelta = Generator::gaussByDoubleExp(mDeltaAverage,mDeltaError,tmin, tmax);
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
           /* double delta;
            double lambdai = event->mTheta.mX - mTheta.mX;
            do
            {
                double x = Generator::gaussByBoxMuller(0, 1);

                double lambdai = event->mTheta.mX - mTheta.mX;
                delta = mSigma.mX * x + lambdai;
            }while(delta < mDeltaMin || delta > mDeltaMax);

            mDelta = delta;*/

            double lambdai = event->mTheta.mX - mTheta.mX;

            mDelta = Generator::gaussByDoubleExp(lambdai,mSigma.mX,mDeltaMin, mDeltaMax);
            break;
        }
        case eDeltaGaussian:
        {
            double lambda = event->mTheta.mX - mTheta.mX;
            double w = (1/(mSigma.mX * mSigma.mX)) + (1/(mDeltaError * mDeltaError));
            double deltaAvg = (lambda / (mSigma.mX * mSigma.mX) + mDeltaAverage / (mDeltaError * mDeltaError)) / w;
            double x = Generator::gaussByBoxMuller(0, 1);
            double delta = deltaAvg + x / sqrt(w);
            
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
    double lambda = pow(mTheta.mX - (event->mTheta.mX - mDelta), 2) / 2.;
    
    const int logVMin = -6;
    const int logVMax = 100;
    
    double V1 = mSigma.mX * mSigma.mX;
    double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigma.mSigmaMH);
    double V2 = pow(10, logV2);
    
    double rapport = 0;
    if(logV2 >= logVMin && logV2 <= logVMax)
    {
        double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
        double x2 = pow((event->mS02 + V1) / (event->mS02 + V2), event->mAShrinkage + 1);
        rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1; // (V2 / V1) est le jacobien!
    }
    mSigma.tryUpdate(sqrt(V2), rapport);
}

void Date::updateWiggle()
{
    mWiggle.mX = mTheta.mX + mDelta;
}

#pragma mark CSV dates
Date Date::fromCSV(QStringList dataStr)
{
    Date date;
    QString pluginName = dataStr.takeFirst();
    PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
    if(plugin)
    {
        date.mName = dataStr[0];
        date.mPlugin = plugin;
        date.mMethod = plugin->getDataMethod();
        date.mData = plugin->fromCSV(dataStr);
        
        int minColNum = plugin->csvMinColumns();
        if(dataStr.size() >= minColNum + 2)
        {
            QString deltaType = dataStr[minColNum];
            QString delta1 = dataStr[minColNum + 1];
            QString delta2 = "0";
            if(dataStr.size() >= minColNum + 3)
                delta2 = dataStr[minColNum + 2];
            
            if(!isComment(deltaType) && !isComment(delta1) && !isComment(delta2))
            {
                if(deltaType == "fixed")
                {
                    date.mDeltaType = Date::eDeltaFixed;
                    date.mDeltaFixed = delta1.toDouble();
                }
                else if(deltaType == "range")
                {
                    date.mDeltaType = Date::eDeltaRange;
                    date.mDeltaMin = delta1.toDouble();
                    date.mDeltaMax = delta2.toDouble();
                }
                else if(deltaType == "gaussian")
                {
                    date.mDeltaType = Date::eDeltaGaussian;
                    date.mDeltaAverage = delta1.toDouble();
                    date.mDeltaError = delta2.toDouble();
                }
            }
        }
    }
    return date;
}

QStringList Date::toCSV() const
{
    QStringList csv;
    
    csv << mPlugin->getName();
    csv << mName;
    csv << mPlugin->toCSV(mData);
    
    if(mDeltaType == Date::eDeltaFixed)
    {
        if(mDeltaFixed != 0)
        {
            csv << "fixed";
            csv << QString::number(mDeltaFixed);
        }
    }
    else if(mDeltaType == Date::eDeltaRange)
    {
        csv << "range";
        csv << QString::number(mDeltaMin);
        csv << QString::number(mDeltaMax);
    }
    else if(mDeltaType == Date::eDeltaGaussian)
    {
        csv << "gaussian";
        csv << QString::number(mDeltaAverage);
        csv << QString::number(mDeltaError);
    }
    
    return csv;
}
