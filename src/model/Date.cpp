#include "Date.h"
#include "Event.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "PluginManager.h"
#include "PluginUniform.h"
#include "../PluginAbstract.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include <QDebug>


Date::Date():
mName("No Named Date")
{
    mColor=Qt::blue;
    mTheta.mIsDate = true;
    mSigma.mIsDate = false;
    init();
    
}

Date::Date(PluginAbstract* plugin):
mName("No Named Date")
{
    init();
    mPlugin = plugin;
}

void Date::init()
{
    mId = 0;
    mMethod = eMHSymetric;
    updateti = fMHSymetric;
    
    mIsValid = true;
    mDelta = 0;
    mDeltaType = eDeltaFixed;
    mDeltaFixed = 0;
    mDeltaMin = 0;
    mDeltaMax = 0;
    mDeltaAverage = 0;
    mDeltaError = 0;
    mIsCurrent = false;
    mIsSelected = false;
    mSubDates.clear();

    mTminRefCurve = 0;
    mTmaxRefCurve = 0;

    mTminCalib = 0;
    mTmaxCalib = 0;

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
    mColor = date.mColor;
    
    mData = date.mData;
    mPlugin = date.mPlugin;
    mMethod = date.mMethod;
    mIsValid = date.mIsValid;
    
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
    
    mTminRefCurve = date.mTminRefCurve;
    mTmaxRefCurve = date.mTmaxRefCurve;

    mTminCalib = date.mTminCalib;
    mTmaxCalib = date.mTmaxCalib;

    mSubDates = date.mSubDates;
    
    updateti = date.updateti;
    
    mMixingLevel = date.mMixingLevel;
}

Date::~Date()
{
    
}

bool Date::isNull() const
{
    return mData.isEmpty() || (mPlugin == 0);
}
#pragma mark Properties
QColor Date::getEventColor() const
{
    return randomColor();
}

#pragma mark JSON
Date Date::fromJson(const QJsonObject& json)
{
    Date date;
    
    if(!json.isEmpty())
    {
        date.mId = json[STATE_ID].toInt();
        date.mName = json[STATE_NAME].toString();
        
        // Copy plugin specific values for this data :
        date.mData = json[STATE_DATE_DATA].toObject();
        
        date.mMethod = (DataMethod)json[STATE_DATE_METHOD].toInt();
        date.mIsValid = json[STATE_DATE_VALID].toBool();
        
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
        /*else if(date.mPlugin->getName() == "Typo"){
            date.mTminCalib = date.mData[DATE_UNIFORM_MIN_STR].toDouble();
            date.mTmaxCalib = date.mData[DATE_UNIFORM_MAX_STR].toDouble();
            date.mTminRefCurve = date.mTminCalib;
            date.mTmaxRefCurve = date.mTmaxCalib;
        }*/
        else
        {
            QPair<double, double> tminTmax = date.mPlugin->getTminTmaxRefsCurve(date.mData);
            date.mTminRefCurve = tminTmax.first;
            date.mTmaxRefCurve = tminTmax.second;
        }
        
        date.mSubDates.clear();
        QJsonArray subdates = json[STATE_DATE_SUB_DATES].toArray();
        for(int i=0; i<subdates.size(); ++i){
            QJsonObject d = subdates[i].toObject();
            date.mSubDates.push_back(Date::fromJson(d));
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
    date[STATE_DATE_VALID] = mIsValid;
    
    date[STATE_DATE_DELTA_TYPE] = mDeltaType;
    date[STATE_DATE_DELTA_FIXED] = mDeltaFixed;
    date[STATE_DATE_DELTA_MIN] = mDeltaMin;
    date[STATE_DATE_DELTA_MAX] = mDeltaMax;
    date[STATE_DATE_DELTA_AVERAGE] = mDeltaAverage;
    date[STATE_DATE_DELTA_ERROR] = mDeltaError;
    
    date[STATE_COLOR_RED] = mColor.red();
    date[STATE_COLOR_GREEN] = mColor.green();
    date[STATE_COLOR_BLUE] = mColor.blue();
    
    QJsonArray subdates;
    for(int i=0; i<mSubDates.size(); ++i){
        QJsonObject d = mSubDates[i].toJson();
        subdates.push_back(d);
    }
    date[STATE_DATE_SUB_DATES] = subdates;
    
    return date;
}

double Date::getLikelyhood(const double& t)
{
    double result = 0.f;
    if(mPlugin)
        result = mPlugin->getLikelyhood(t, mData);
    return result;
}

QPair<double, double > Date::getLikelyhoodArg(const double& t)
{
    if(mPlugin)   return mPlugin->getLikelyhoodArg(t,mData);
    else return QPair<double, double>();

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
    // we need to calculate a new mCalibration only, if mStep change or if there is no mCalibration
    /* if(mSettings.mStep == settings.mStep && !mCalibration.isEmpty()){
        return;
    }*/
    
    mCalibration.clear();
    mRepartition.clear();
    mCalibHPD.clear();

    mSettings = settings;
    
    // --------------------------------------------------
    //  Calibrate on the whole calibration period (= ref curve definition domain)
    // --------------------------------------------------
    if(mTmaxRefCurve > mTminRefCurve)
    {
        double nbRefPts = 1 + round((mTmaxRefCurve - mTminRefCurve) / mSettings.mStep);
        double v = getLikelyhood(mTminRefCurve);
        double lastRepVal = v;
        
        QVector<double> calibrationTemp;
        QVector<double> repartitionTemp;
        
        for(int i = 0; i < nbRefPts; ++i)
        {
            double t = mTminRefCurve + (double)i * mSettings.mStep;
            float lastV = v;
            v = getLikelyhood(t);
            
            calibrationTemp.append(v);
            double rep = lastRepVal;
            if(v != 0 && lastV != 0)
            {
                rep = lastRepVal + mSettings.mStep * (lastV + v) / 2.;
            }
            repartitionTemp.append(rep);
            lastRepVal = rep;
        }
        
        // ------------------------------------------------------------------
        //  Restrict the calib and repartition vectors to where data are
        // ------------------------------------------------------------------
        if(lastRepVal > 0)
        {
            const double threshold = 0.00005;
            const int minIdx = (int)floor(vector_interpolate_idx_for_value(threshold * lastRepVal, repartitionTemp));
            const int maxIdx = (int)ceil(vector_interpolate_idx_for_value((1 - threshold) * lastRepVal, repartitionTemp));
            
            mTminCalib = mTminRefCurve + minIdx * mSettings.mStep;
            mTmaxCalib = mTminRefCurve + maxIdx * mSettings.mStep;
            
            // Truncate both functions where data live
            mCalibration = calibrationTemp.mid(minIdx, (maxIdx - minIdx) + 1);
            mRepartition = repartitionTemp.mid(minIdx, (maxIdx - minIdx) + 1);
            
            // NOTE ABOUT THIS APPROMIATION :
            // By truncating the calib and repartition, the calib density's area is not 1 anymore!
            // It is now 1 - 2*threshold = 0,99999... We consider it to be 1 anyway!
            // By doing this, calib and repartition are stored on a restricted number of data
            // instead of storing them on the whole reference curve's period (as done for calibrationTemp & repartitionTemp above).
            
            // Stretch repartition curve so it goes from 0 to 1
            mRepartition = stretch_vector(mRepartition, 0, 1);
            
            // Approximation : even if the calib has been truncated, we consider its area to be = 1
            mCalibration = equal_areas(mCalibration, mSettings.mStep, 1.);
        }
        // ------------------------------------------------------------------
        //  Measure is very far from Ref curve on the whole ref curve preriod!
        //  => Calib values are very small, considered as being 0 even using "double" !
        //  => lastRepVal = 0, and impossible to truncate using it....
        //  => So,
        // ------------------------------------------------------------------
        else
        {
            mTminCalib = mTminRefCurve;
            mTmaxCalib = mTmaxRefCurve;
        }
    }
    else
    {
        // Impossible to calibrate because the plugin could not return any calib curve definition period.
        // This may be due to invalid ref curve files or to polynomial equations with only imaginary solutions (See Gauss Plugin...)
    }
}

QMap<double, double> Date::getCalibMap() const
{
    return vector_to_map(mCalibration, mTminCalib, mTmaxCalib, mSettings.mStep);
}

QPixmap Date::generateTypoThumb()
{
    if(mIsValid){
        //  No need to draw the graph on a large size
        //  These values are arbitary
        QSize size(200, 30);
        QPixmap thumb(size);

        double tLower = mData[DATE_UNIFORM_MIN_STR].toDouble();
        double tUpper = mData[DATE_UNIFORM_MAX_STR].toDouble();

       // bool isFixed = (tLower == tUpper);

        QPainter p;
        p.begin(&thumb);
        p.setRenderHint(QPainter::Antialiasing);

        double tmin = mSettings.mTmin;
        double tmax = mSettings.mTmax;
        // qDebug()<<" Date::generateCalibThumb"<<tmin<<tmax<<mSettings.mStep;

        GraphView graph;
        graph.setFixedSize(size);
        graph.setMargins(0, 0, 0, 0);

        graph.setRangeX(tmin, tmax);
        graph.setCurrentX(tmin, tmax);
        graph.setRangeY(0, 1.0f);

        graph.showXAxisArrow(false);
        graph.showXAxisTicks(false);
        graph.showXAxisSubTicks(false);
        graph.showXAxisValues(false);

        graph.showYAxisArrow(false);
        graph.showYAxisTicks(false);
        graph.showYAxisSubTicks(false);
        graph.showYAxisValues(false);

        graph.setXAxisMode(GraphView::eHidden);
        graph.setYAxisMode(GraphView::eHidden);

        QColor color = mPlugin->getColor();

        GraphCurve curve;
        curve.mBrush = color;

        curve.mPen = QPen(color, 2.f);

        curve.mIsHorizontalSections = true;

        curve.mSections.append(qMakePair(tLower,tUpper));
        graph.addCurve(curve);

        curve.mName = "Calibration";

        graph.repaint();

        graph.render(&p);
        p.end();

        return thumb;
    }
    else{
        // If date is invalid, return a null pixmap!
        return QPixmap();
    }

    //thumb.save("test.png");
    //thumb = graph.grab();

}




QPixmap Date::generateCalibThumb()
{
    if(mIsValid)
    {
        //  No need to draw the graph on a large size
        //  These values are arbitary
        QSize size(200, 30);
        
        double tmin = mSettings.mTmin;
        double tmax = mSettings.mTmax;
        
        GraphView graph;
        graph.setFixedSize(size);
        graph.setMargins(0, 0, 0, 0);
        
        graph.setRangeX(tmin, tmax);
        graph.setCurrentX(tmin, tmax);
        graph.setRangeY(0, 1.1f);
        
        graph.showXAxisArrow(false);
        graph.showXAxisTicks(false);
        graph.showXAxisSubTicks(false);
        graph.showXAxisValues(false);
        
        graph.showYAxisArrow(false);
        graph.showYAxisTicks(false);
        graph.showYAxisSubTicks(false);
        graph.showYAxisValues(false);
        
        graph.setXAxisMode(GraphView::eHidden);
        graph.setYAxisMode(GraphView::eHidden);
        
        QColor color = mPlugin->getColor();//  Painting::mainColorLight;
        
        GraphCurve curve;
        curve.mData = normalize_map(getCalibMap());
        curve.mName = "Calibration";
        curve.mPen = QPen(color, 2.f);
        curve.mBrush = color;
        curve.mIsHisto = false;
        curve.mIsRectFromZero = true; // For Typo !!
        
        graph.addCurve(curve);
        
        QPixmap thumb(size);
        if(graph.inherits("QOpenGLWidget"))
        {
            // Does not work if the graph has not been rendered before
            // (calling repaint() has no direct effect on QOpenGLWidget...)
            //graph.repaint();
            //QImage image = graph.grabFramebuffer();
            //thumb = QPixmap::fromImage(image);
            
            // This works but there are some drawing region issues...
            graph.setRendering(GraphView::eHD);
            graph.paintToDevice(&thumb);
        }
        else
        {
            QPainter p;
            p.begin(&thumb);
            p.setRenderHint(QPainter::Antialiasing);
            graph.repaint();
            graph.render(&p);
            p.end();
        }
        return thumb;
    }
    else{
        // If date is invalid, return a null pixmap!
        return QPixmap();
    }
    
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
    updateti(this,event);

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
           double lambdai = event->mTheta.mX - mTheta.mX;

            mDelta = Generator::gaussByDoubleExp(lambdai,mSigma.mX,mDeltaMin, mDeltaMax);
            break;
        }
        case eDeltaGaussian:
        {
            double lambdai = event->mTheta.mX - mTheta.mX;
            double w = (1/(mSigma.mX * mSigma.mX)) + (1/(mDeltaError * mDeltaError));
            double deltaAvg = (lambdai / (mSigma.mX * mSigma.mX) + mDeltaAverage / (mDeltaError * mDeltaError)) / w;
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

QStringList Date::toCSV(QLocale csvLocale) const
{
    QStringList csv;
    
    csv << mPlugin->getName();
    csv << mName;
    csv << mPlugin->toCSV(mData,csvLocale );
    
    if(mDeltaType == Date::eDeltaFixed)
    {
        if(mDeltaFixed != 0)
        {
            csv << "fixed";
            csv << csvLocale.toString(mDeltaFixed);
        }
    }
    else if(mDeltaType == Date::eDeltaRange)
    {
        csv << "range";
        csv << csvLocale.toString(mDeltaMin);
        csv << csvLocale.toString(mDeltaMax);
    }
    else if(mDeltaType == Date::eDeltaGaussian)
    {
        csv << "gaussian";
        csv << csvLocale.toString(mDeltaAverage);
        csv << csvLocale.toString(mDeltaError);
    }
    
    return csv;
}

#pragma mark sampling ti function
void Date::autoSetTiSampler(const bool bSet)
{
    // define sampling function
    // select if using getLikelyhooArg is possible, it's a faster way
    
    if (bSet && mPlugin!= 0 && mPlugin->withLikelyhoodArg()) {
         //   if (false) {
        switch(mMethod)
        {
            case eMHSymetric:
            {
                updateti = fMHSymetricWithArg;
                
                break;
            }
            case eInversion:
            {
                updateti = fInversionWithArg;
                
                break;
            }
                // Seul cas où le taux d'acceptation a du sens car on utilise sigmaMH :
            case eMHSymGaussAdapt:
            {
                updateti = fMHSymGaussAdaptWithArg;
                
                break;
            }
            default:
            {
                break;
            }
        }
       // qDebug()<<"Date::autoSetTiSampler()"<<this->mName<<"with getLikelyhoodArg";
    }
    else {
        switch(mMethod)
        {
            case eMHSymetric:
            {
                updateti = fMHSymetric;
                
                break;
            }
            case eInversion:
            {
                updateti = fInversion;
                
                break;
            }
                // Seul cas où le taux d'acceptation a du sens car on utilise sigmaMH :
            case eMHSymGaussAdapt:
            {
                updateti = fMHSymGaussAdapt;
                
                break;
            }
            default:
            {
                break;
            }
        }
        //qDebug()<<"Date::autoSetTiSampler()"<<this->mName<<"with getLikelyhood";
        
    }
  
}

/*
 * @brief MH proposal = prior distribution
 *
 */
void fMHSymetric(Date* date,Event* event)
{
//eMHSymetric:

        // Ici, le marcheur est forcément gaussien avec H(theta i) : double_exp (gaussien tronqué)
      /*   double tmin = date->mSettings.mTmin;
        double tmax = date->mSettings.mTmax;
         double theta = Generator::gaussByDoubleExp(event->mTheta.mX - date->mDelta, date->mSigma.mX, tmin, tmax);
         //rapport = G(theta_new) / G(theta_old)
         double rapport = date->getLikelyhoodFromCalib(theta) / date->getLikelyhoodFromCalib(date->mTheta.mX);
         date->mTheta.tryUpdate(theta, rapport);
    */
   
        double tiNew = Generator::gaussByBoxMuller(event->mTheta.mX - date->mDelta, date->mSigma.mX);
        double rapport = date->getLikelyhood(tiNew) / date->getLikelyhood(date->mTheta.mX);
        
        date->mTheta.tryUpdate(tiNew, rapport);
     

}
/*
 * @brief MH proposal = prior distribution
 * @brief identic as fMHSymetric but use getLikelyhoodArg, when plugin offer it
 *
 */
void fMHSymetricWithArg(Date* date,Event* event)
{
    
    double tiNew = Generator::gaussByBoxMuller(event->mTheta.mX - date->mDelta, date->mSigma.mX);
    
    QPair<double, double> argOld, argNew;
    
    argOld=date->getLikelyhoodArg(date->mTheta.mX);
    argNew=date->getLikelyhoodArg(tiNew);
    
    double rapport=sqrt(argOld.first/argNew.first)*exp(argNew.second-argOld.second);
    
    date->mTheta.tryUpdate(tiNew, rapport);
    
}

/**
 *  @brief Calculation of proposal density for time value t
 *
 */
double fProposalDensity(const double t, const double t0, Date* date)
{
    double tmin = date->mSettings.mTmin;
    double tmax = date->mSettings.mTmax;
    double level = date->mMixingLevel;
    double q1 = 0;

    double tminCalib = date->getTminCalib();
    double tmaxCalib = date->getTmaxCalib();

    /// ----q1------Defined only on Calibration range-----
    if(t > tminCalib && t < tmaxCalib){
        //double prop = (t - tmin) / (tmax - tmin);
        double prop = (t - tminCalib) / (tmaxCalib - tminCalib);
        double idx = prop * (date->mRepartition.size() - 1);
        int idxUnder = (int)floor(idx);
        
        //double step =(tmax-tmin+1)/date->mRepartition.size();
        double step =date->mSettings.mStep;

        q1 = (date->mRepartition[idxUnder+1]-date->mRepartition[idxUnder])/step;
    }
    /// ----q2 shrinkage-----------
    /*double t0 =(tmax+tmin)/2;
    double s = (tmax-tmin)/2;
    double q2= s / ( 2* pow((s+fabs(t-t0)), 2) );
     */
        
    /// ----q2 gaussian-----------
    //double t0 = (tmax+tmin)/2;
    double sigma = (tmax-tmin)/2;
    double q2 = exp(-0.5* pow((t-t0)/ sigma, 2)) / (sigma*sqrt(2*M_PI));
    
    return (level * q1 + (1 - level) * q2);
}

/**
 *  @brief MH proposal = Distribution of Calibrated date, ti is defined on set R (real numbers)
 *  @brief simulation according to uniform shrinkage with s parameter
 */
void fInversion(Date* date, Event* event)
{
    double u1 = Generator::randomUniform();
    double level=date->mMixingLevel;
    double tiNew;
    double tmin = date->mSettings.mTmin;
    double tmax = date->mSettings.mTmax;

    double tminCalib = date->getTminCalib();
    
    if (u1<level) { // tiNew always in the study period
        double idx = vector_interpolate_idx_for_value(u1, date->mRepartition);
        tiNew = tminCalib + idx *date->mSettings.mStep;
    }
    else {
        // -- gaussian
        double t0 = date->mTheta.mX;
        double s = (tmax-tmin)/2;
        
        tiNew=Generator::gaussByBoxMuller(t0, s);
        /*
        // -- double shrinkage
        double u2 = Generator::randomUniform();
        double t0 =(tmax+tmin)/2;
        double s = (tmax-tmin)/2;
        
        double tPrim= s* ( (1-u2)/u2 ); // simulation according to uniform shrinkage with s parameter
        
        double u3 = Generator::randomUniform();
        if (u3<0.5f) {
            tiNew= t0 + tPrim;
        }
        else {
            tiNew= t0 - tPrim;
        }
        */
    }
             
    double rapport1 = date->getLikelyhood(tiNew) / date->getLikelyhood(date->mTheta.mX);
    
    double rapport2 = exp((-0.5 / (date->mSigma.mX * date->mSigma.mX)) *
                          (pow(tiNew - (event->mTheta.mX - date->mDelta), 2) -
                           pow(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2))
                          );
    
    double rapport3 = fProposalDensity(date->mTheta.mX, tiNew, date) /
                        fProposalDensity(tiNew, date->mTheta.mX, date);
    
    date->mTheta.tryUpdate(tiNew, rapport1 * rapport2 * rapport3);
}

void fInversionWithArg(Date* date, Event* event)
{
    double u1 = Generator::randomUniform();
    double level=date->mMixingLevel;
    double tiNew;
    double tmin = date->mSettings.mTmin;
    double tmax = date->mSettings.mTmax;

    double tminCalib = date->getTminCalib();
    
    if (u1<level) { // tiNew always in the study period
        double u2 = Generator::randomUniform();
        double idx = vector_interpolate_idx_for_value(u2, date->mRepartition);
        tiNew = tminCalib + idx *date->mSettings.mStep;
    }
    else {
        // -- gaussian
        double t0 =(tmax+tmin)/2;
        double s = (tmax-tmin)/2;
        
        tiNew=Generator::gaussByBoxMuller(t0, s);
        /*
         // -- double shrinkage
         double u2 = Generator::randomUniform();
         double t0 =(tmax+tmin)/2;
         double s = (tmax-tmin)/2;
         
         double tPrim= s* ( (1-u2)/u2 ); // simulation according to uniform shrinkage with s parameter
         
         double u3 = Generator::randomUniform();
         if (u3<0.5f) {
         tiNew= t0 + tPrim;
         }
         else {
         tiNew= t0 - tPrim;
         }
         */
    }
    
    
    QPair<double, double> argOld, argNew;
    
    argOld=date->getLikelyhoodArg(date->mTheta.mX);
    argNew=date->getLikelyhoodArg(tiNew);
    
    double logGRapport= argNew.second-argOld.second;
    double logHRapport= (-0.5/(date->mSigma.mX * date->mSigma.mX)) * (pow(tiNew - (event->mTheta.mX - date->mDelta), 2)
                                                                      - pow(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2)
                                                                      );
    
    double rapport = sqrt(argOld.first/argNew.first) * exp(logGRapport+logHRapport);
    
    double rapportPD = fProposalDensity(date->mTheta.mX, tiNew, date) / fProposalDensity(tiNew, date->mTheta.mX, date);
    
    date->mTheta.tryUpdate(tiNew, rapport * rapportPD);
    
    //date->mTheta.tryUpdate(tiNew, exp(logHRapport));
    
}


/*
 * @brief MH proposal = adaptatif Gaussian random walk, ti is defined on set R (real numbers)
 */
void fMHSymGaussAdapt(Date* date, Event* event)
{
    /* double rapport = 0;
    // if(theta >= tmin && theta <= tmax)
    // {
    // rapport = (G(theta_new) / G(theta_old)) * (H(theta_new) / H(theta_old))
    //rapport = getLikelyhoodFromCalib(theta) / getLikelyhoodFromCalib(mTheta.mX); // rapport des G(theta i)
    */
    
    double tiNew = Generator::gaussByBoxMuller(date->mTheta.mX, date->mTheta.mSigmaMH);
    double rapport = date->getLikelyhood(tiNew) / date->getLikelyhood(date->mTheta.mX);
    rapport *= exp((-0.5/(date->mSigma.mX * date->mSigma.mX)) * (   pow(tiNew - (event->mTheta.mX - date->mDelta), 2)
                                                                  - pow(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2)
                                                                 ));
    
    date->mTheta.tryUpdate(tiNew, rapport);
}


/*
 * @brief MH proposal = adaptatif Gaussian random walk, ti is not constraint being on the study period
 *
 * @brief identic as fMHSymGaussAdapt but use getLikelyhoodArg, when plugin offer it
 */
void fMHSymGaussAdaptWithArg(Date* date, Event* event)
{
    double tiNew = Generator::gaussByBoxMuller(date->mTheta.mX, date->mTheta.mSigmaMH);
    
    QPair<double, double> argOld, argNew;
    
    argOld=date->getLikelyhoodArg(date->mTheta.mX);
    argNew=date->getLikelyhoodArg(tiNew);
    
    double logGRapport= argNew.second-argOld.second;
    double logHRapport= (-0.5/(date->mSigma.mX * date->mSigma.mX)) * (  pow(tiNew - (event->mTheta.mX - date->mDelta), 2)
                                                                      - pow(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2)
                                                                      );
    
    double rapport=sqrt(argOld.first/argNew.first)*exp(logGRapport+logHRapport);
    
    date->mTheta.tryUpdate(tiNew, rapport);
}
