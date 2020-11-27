/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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
#include "Project.h"
#include "MainWindow.h"

#include <QDebug>

Date::Date():
mName("No Named Date")
{
   // init();
    mColor = Qt::blue;
    mOrigin = eSingleDate;
    mPlugin = nullptr;
    mTheta.mSupport = MetropolisVariable::eR;
    mSigma.mSupport = MetropolisVariable::eRp;
    mWiggle.mSupport = MetropolisVariable::eR;

    mTheta.mFormat = DateUtils::eUnknown;
    mSigma.mFormat = DateUtils::eUnknown;
    mWiggle.mFormat = DateUtils::eUnknown;

    mId = -1;
    mUUID = QString("NONE");

    mMethod = eMHSymetric;
    updateti = fMHSymetric;

    mIsValid = false;
    mDelta = 0.;
    mDeltaType = eDeltaNone;
    mDeltaFixed = 0.;
    mDeltaMin = 0.;
    mDeltaMax = 0.;
    mDeltaAverage = 0.;
    mDeltaError = 0.;
    mIsCurrent = false;
    mIsSelected = false;
   // mSubDates.clear();

    mTminRefCurve = -INFINITY;
    mTmaxRefCurve = INFINITY;

    mCalibration = nullptr;
    mWiggleCalibration = nullptr;
}

Date::Date(PluginAbstract* plugin):
mName("No Named Date")
{
    init();
    mPlugin = plugin;
}

void Date::init()
{
    mColor = Qt::blue;
    mOrigin = eSingleDate;
    mPlugin = nullptr;
    mTheta.mSupport = MetropolisVariable::eR;
    mSigma.mSupport = MetropolisVariable::eRp;
    mWiggle.mSupport = MetropolisVariable::eR;

    mTheta.mFormat = DateUtils::eUnknown;
    mSigma.mFormat = DateUtils::eUnknown;
    mWiggle.mFormat = DateUtils::eUnknown;

    mId = -1;
    mUUID = QString("NONE");
    mMethod = eMHSymetric;
    updateti = fMHSymetric;

    mIsValid = true;
    mDelta = 0.;
    mDeltaType = eDeltaNone;
    mDeltaFixed = 0.;
    mDeltaMin = 0.;
    mDeltaMax = 0.;
    mDeltaAverage = 0.;
    mDeltaError = 0.;
    mIsCurrent = false;
    mIsSelected = false;
   // mSubDates.clear();

    mTminRefCurve = -INFINITY;
    mTmaxRefCurve = INFINITY;

    mCalibration = nullptr;
    mWiggleCalibration = nullptr;

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
    mWiggle = date.mWiggle;

    mId = date.mId;
    mUUID = date.mUUID;

    mName = date.mName;
    mColor = date.mColor;

    mData = date.mData;
    mOrigin = date.mOrigin;
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
    mWiggleCalibration = date.mWiggleCalibration;

    mCalibHPD = date.mCalibHPD;

    mTminRefCurve = date.mTminRefCurve;
    mTmaxRefCurve = date.mTmaxRefCurve;

    mSubDates = date.mSubDates;

    updateti = date.updateti;

    mMixingLevel = date.mMixingLevel;

    mSettings = date.mSettings;
}

Date::~Date()
{
    mPlugin = nullptr;
    mCalibration = nullptr;
    mWiggleCalibration = nullptr;

}

bool Date::isNull() const
{
    return mData.isEmpty() || (mPlugin == nullptr);
}

// Properties
QColor Date::getEventColor() const
{
    return randomColor();
}

// JSON

void Date::fromJson(const QJsonObject& json)
{
    Q_ASSERT(&json);
    mId = json.value(STATE_ID).toInt();
    mName = json.value(STATE_NAME).toString();

    mUUID = json.value(STATE_DATE_UUID).toString();
    qDebug() << mName << mId << " Date::fromJson STATE_DATE_UUID mUUID" << mUUID;
    if (mUUID.isEmpty())
        mUUID = QString::fromStdString( Generator::UUID());
    qDebug() << " Date::fromJson mUUID2" << mUUID;
    
    // Copy plugin specific values for this data :
    mData = json.value(STATE_DATE_DATA).toObject();
    mOrigin = (OriginType)json.value(STATE_DATE_ORIGIN).toInt();

    mMethod = (DataMethod)json.value(STATE_DATE_METHOD).toInt();
    mIsValid = json.value(STATE_DATE_VALID).toBool();

    mDeltaType = (DeltaType)json.value(STATE_DATE_DELTA_TYPE).toInt();
    mDeltaFixed = json.value(STATE_DATE_DELTA_FIXED).toDouble();
    mDeltaMin = json.value(STATE_DATE_DELTA_MIN).toDouble();
    mDeltaMax = json.value(STATE_DATE_DELTA_MAX).toDouble();
    mDeltaAverage = json.value(STATE_DATE_DELTA_AVERAGE).toDouble();
    mDeltaError = json.value(STATE_DATE_DELTA_ERROR).toDouble();

    QString pluginId = json.value(STATE_DATE_PLUGIN_ID).toString();
    mPlugin = PluginManager::getPluginFromId(pluginId);
    
    
    mSubDates = json.value(STATE_DATE_SUB_DATES).toArray();
    
    if (mPlugin == nullptr)
        throw QObject::tr("Data could not be loaded : invalid plugin : %1").arg(pluginId);
    else  {
        if (mOrigin == eSingleDate) {
            QPair<double, double> tminTmax = mPlugin->getTminTmaxRefsCurve(mData);
            mTminRefCurve = tminTmax.first;
            mTmaxRefCurve = tminTmax.second;
            
        } else if (mOrigin == eCombination) {
            QPair<double, double> tminTmax = mPlugin->getTminTmaxRefsCurveCombine(mSubDates);
            mTminRefCurve = tminTmax.first;
            mTmaxRefCurve = tminTmax.second;
        }
    }

    mTheta.mProposal = ModelUtilities::getDataMethodText(mMethod);
    mTheta.setName("Theta of date : "+ mName);
    mSigma.mProposal = ModelUtilities::getDataMethodText(Date::eMHSymGaussAdapt);
    mSigma.setName("Sigma of date : "+ mName);

    Project* project = MainWindow::getInstance()->getProject();
    mSettings = project->mModel->mSettings;
    
    QString toFind = mUUID;

    QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (toFind);
    if ( it!=project->mCalibCurves.end())
        mCalibration = & it.value();
    
    toFind = "WID::" + mUUID;
    it = project->mCalibCurves.find (toFind);
    if ( it!=project->mCalibCurves.end())
        mWiggleCalibration = & it.value();

}

QJsonObject Date::toJson() const
{
    QJsonObject date;
    date[STATE_ID] = mId;
    date[STATE_DATE_UUID] = mUUID;
    qDebug() << mName << mId << "Date::toJson  mUUID "<< date.value(STATE_DATE_UUID).toString();
    date[STATE_NAME] = mName;
    date[STATE_DATE_DATA] = mData;
    date[STATE_DATE_ORIGIN] = mOrigin;
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

    date[STATE_DATE_SUB_DATES] = mSubDates;
    return date;
}

long double Date::getLikelihood(const double& t) const
{
    //double result (0.);
    if (mPlugin) {
        if (mOrigin == eSingleDate) {
            return mPlugin->getLikelihood(t, mData);
            
        } else if (mOrigin == eCombination) {
            return mPlugin->getLikelihoodCombine(t, mSubDates);
            
        } else {
            
            return 0.l;
        }
    
    } else
         return 0.l;

}

QPair<long double, long double> Date::getLikelihoodArg(const double& t) const
{
    if (mPlugin)
        if (mOrigin == eSingleDate) {
            return mPlugin->getLikelihoodArg(t, mData);
            
        }  else if (mOrigin == eCombination) {
            return QPair<long double, long double>(log(mPlugin->getLikelihoodCombine(t, mSubDates)), 1.l);
                   
       } else {
           return QPair<long double, long double>();
       }
        
    else
        return QPair<double, double>();
}

QString Date::getDesc() const
{
    QString res;
    if (mPlugin)
        res = mPlugin->getDateDesc(this);

    else {
        QStringList params;
        QJsonObject::const_iterator iter;
        for (iter = mData.begin(); iter!=mData.end(); ++iter) {
            QString val;
            if (iter.value().isString())
                val = iter.value().toString();

            else if (iter.value().isDouble())
                val = QString::number(iter.value().toDouble());

            else if (iter.value().isBool())
                val = iter.value().toBool() ? QObject::tr("yes") : QObject::tr("no");
            params << iter.key() + " = " + val;
        }
        res = params.join(", ");
    }
  
    return res;
}

QString Date::getWiggleDesc() const
{
    QString res;
      
    switch (mDeltaType) {
        case Date::eDeltaFixed:
            res = res+ "Wiggle = " + QString::number(mDeltaFixed);
            break;
        case Date::eDeltaRange:
            res = res+ "Wiggle = U[" + QString::number(mDeltaMin) + " ; " + QString::number(mDeltaMax) + " ] ";
            break;
        case Date::eDeltaGaussian:
            res = res+ "Wiggle = N(" + QString::number(mDeltaAverage) + " ; " + QString::number(mDeltaError) + " ) ";
            break;
            
        default:
            res = "No Wiggle";
            break;
    }
    

    return res;
}

void Date::reset()
{
    mTheta.reset();
    mSigma.reset();

    mWiggle.reset();
}

void Date::calibrate(const ProjectSettings& settings, Project *project)
{
  // Check if the ref curve is in the plugin list
    qDebug()<<"Date::calibrate";

    if (mOrigin == eSingleDate) {
        const QStringList refsNames = mPlugin->getRefsNames();
        const QString dateRefName = mPlugin->getDateRefCurveName(this);
            
        if (!dateRefName.isEmpty() && !refsNames.contains(dateRefName) )
            return;
        
    }

    // add the calibration
    mSettings = settings;

    //const QString toFind (mUUID);
    QMap<QString, CalibrationCurve>::const_iterator it = project->mCalibCurves.find (mUUID);

    if ( it == project->mCalibCurves.end()) {
        qDebug()<<"Curve to create mUUID: "<< mUUID ;
        project->mCalibCurves.insert(mUUID, CalibrationCurve());
        
    } else if ( it->mDescription == getDesc() ) {
        // Controls whether the curve has already been calculated using the description
        qDebug() << "The curve already exists mUUID:" << mUUID;
        /* WIGGLE CALIBRATION CURVE */
        
        calibrateWiggle(settings, project);
        
        return;
    }
        
  // Update of the new calibration curve

    mCalibration = & (project->mCalibCurves[mUUID]);
    mCalibration -> mDescription = getDesc();
 
    mCalibration->mStep = mSettings.mStep;
    mCalibration->mPluginId = mPlugin->getId();
    mCalibration->mPlugin = mPlugin;
    mCalibration->mName = mName;

    mCalibHPD.clear();

    double tminCal;
    double tmaxCal;

    // --------------------------------------------------
    //  Calibrate on the whole calibration period (= ref curve definition domain)
    // --------------------------------------------------
   
    if (mTmaxRefCurve > mTminRefCurve) {
        QVector<double> calibrationTemp;
        QVector<double> repartitionTemp;

        const double nbRefPts = 1. + round((mTmaxRefCurve - mTminRefCurve) / double(settings.mStep));
        long double v = getLikelihood(mTminRefCurve);
        calibrationTemp.append(v);
        repartitionTemp.append(0.);
        long double lastRepVal = v;

        // We use long double type because
        // after several sums, the repartition can be in the double type range
        for (int i = 1; i <= nbRefPts; ++i) {
            const double t = mTminRefCurve + double (i) * settings.mStep;
            long double lastV = v;
            v = getLikelihood(t);

            calibrationTemp.append(double(v));
            long double rep = lastRepVal;
            if (v != 0.l && lastV != 0.l)
                rep = lastRepVal + (long double) (settings.mStep) * (lastV + v) / 2.l;

            repartitionTemp.append(double (rep));
            lastRepVal = rep;
        }

        // ------------------------------------------------------------------
        //  Restrict the calib and repartition vectors to where data are
        // ------------------------------------------------------------------

        if (repartitionTemp.last() > 0.) {
            const double threshold (0.00005);
            const int minIdx = int (floor(vector_interpolate_idx_for_value(threshold * lastRepVal, repartitionTemp)));
            const int maxIdx = int (ceil(vector_interpolate_idx_for_value((1. - threshold) * lastRepVal, repartitionTemp)));

            tminCal = mTminRefCurve + minIdx * settings.mStep;
            tmaxCal = mTminRefCurve + maxIdx * settings.mStep;

            // Truncate both functions where data live
            mCalibration->mCurve = calibrationTemp.mid(minIdx, (maxIdx - minIdx) + 1);
            mCalibration->mRepartition = repartitionTemp.mid(minIdx, (maxIdx - minIdx) + 1);

            /* NOTE ABOUT THIS APPROMIATION :
           By truncating the calib and repartition, the calib density's area is not 1 anymore!
           It is now 1 - 2*threshold = 0,99999... We consider it to be 1 anyway!
           By doing this, calib and repartition are stored on a restricted number of data
           instead of storing them on the whole reference curve's period (as done for calibrationTemp & repartitionTemp above).
             */

            // Stretch repartition curve so it goes from 0 to 1
            mCalibration->mRepartition = stretch_vector(mCalibration->mRepartition, 0., 1.);

            // Approximation : even if the calib has been truncated, we consider its area to be = 1
            mCalibration->mCurve = equal_areas(mCalibration->mCurve, settings.mStep, 1.);

        }
        // ------------------------------------------------------------------
        //  Measurement is very far from Ref curve on the whole ref curve preriod!
        //  => Calib values are very small, considered as being 0 even using "double" !
        //  => lastRepVal = 0, and impossible to truncate using it....
        //  => So,
        // ------------------------------------------------------------------
        else  {
            tminCal = mTminRefCurve;
            tmaxCal = mTmaxRefCurve;
        }

        mCalibration->mTmin = tminCal;
        mCalibration->mTmax = tmaxCal;

    }
    else {
        // Impossible to calibrate because the plugin could not return any calib curve definition period.
        // This may be due to invalid ref curve files or to polynomial equations with only imaginary solutions (See Gauss Plugin...)
    }

    
    /* WIGGLE CALIBRATION CURVE */
    if (mDeltaType != Date::eDeltaNone) {
        calibrateWiggle(settings, project);
    }
    
    
   // qDebug()<<"Date::calibrate in project"<<project->mCalibCurves[toFind].mName;
}

void Date::calibrateWiggle(const ProjectSettings& settings, Project *project)
{
  // Check if the ref curve is in the plugin list
    qDebug()<<"Date::calibrate Wiggle";
    if (mDeltaType == Date::eDeltaNone) {
        mWiggleCalibration = nullptr;
        return;
    }
    // add the calibration
    mSettings = settings;
 
    const QString toFind ("WID::" + mUUID);
    QMap<QString, CalibrationCurve>::const_iterator it = project->mCalibCurves.find (toFind);

    
    if ( it == project->mCalibCurves.end()) {
        qDebug()<<"Curve to create Wiggle: "<< toFind ;
        project->mCalibCurves.insert(toFind, CalibrationCurve());
        
        
    } else if ( it->mDescription == getWiggleDesc() ) {
        // Controls whether the curve has already been calculated using the description
        qDebug() << "The curve already exists Wiggle:" << toFind;
        return;
    }
        
    
    
  // Update of the new calibration curve

    mWiggleCalibration = & (project->mCalibCurves[toFind]);
    mWiggleCalibration->mDescription = getWiggleDesc();

    mWiggleCalibration->mStep = mSettings.mStep;
    mWiggleCalibration->mPluginId = mPlugin->getId();
    mWiggleCalibration->mPlugin = mPlugin;
    mWiggleCalibration->mName = mName;

   // mCalibHPD.clear();

    double tminCal ( -INFINITY);
    double tmaxCal ( INFINITY);

    // --------------------------------------------------
    //  Calibrate on the whole calibration period (= ref curve definition domain)
    // --------------------------------------------------
    switch (mDeltaType) {
        case Date::eDeltaFixed:
               
           /* if (mTmaxRefCurve > mTminRefCurve) {
                QVector<double> calibrationTemp;
                QVector<double> repartitionTemp;

                const double nbRefPts = 1. + round((mTmaxRefCurve - mTminRefCurve) / double(mSettings.mStep));
                long double v = getLikelihood(mTminRefCurve - mDeltaFixed);
                calibrationTemp.append(v);
                repartitionTemp.append(0.);
                long double lastRepVal = v;

                // We use long double type because
                // after several sums, the repartition can be in the double type range
                for (int i = 1; i <= nbRefPts; ++i) {
                    const double t = mTminRefCurve + double (i) * mSettings.mStep - mDeltaFixed;
                    long double lastV = v;
                    v = getLikelihood(t - mDeltaFixed);

                    calibrationTemp.append(double(v));
                    long double rep = lastRepVal;
                    if (v != 0.l && lastV != 0.l)
                        rep = lastRepVal + (long double) (mSettings.mStep) * (lastV + v) / 2.l;

                    repartitionTemp.append(double (rep));
                    lastRepVal = rep;
                }
             

                if (repartitionTemp.last() > 0.) {
                    const double threshold (0.00005);
                    const int minIdx = int (floor(vector_interpolate_idx_for_value(threshold * lastRepVal, repartitionTemp)));
                    const int maxIdx = int (ceil(vector_interpolate_idx_for_value((1. - threshold) * lastRepVal, repartitionTemp)));

                    tminCal = mTminRefCurve + minIdx * settings.mStep;
                    tmaxCal = mTminRefCurve + maxIdx * settings.mStep;

                    // Truncate both functions where data live
                    mWiggleCalibration->mCurve = calibrationTemp.mid(minIdx, (maxIdx - minIdx) + 1);
                    mWiggleCalibration->mRepartition = repartitionTemp.mid(minIdx, (maxIdx - minIdx) + 1);
                }
              */
                mWiggleCalibration->mCurve = mCalibration->mCurve;
                mWiggleCalibration->mRepartition = mCalibration->mRepartition;
                
                
                mWiggleCalibration->mTmin = mCalibration->mTmin + mDeltaFixed;
                mWiggleCalibration->mTmax = mCalibration->mTmax + mDeltaFixed;
         //   }
            break;
                
                
        case Date::eDeltaRange:
            mWiggleCalibration->mCurve = mCalibration->mCurve;
            mWiggleCalibration->mRepartition = mCalibration->mRepartition;
            
            mWiggleCalibration->mTmin = mCalibration->mTmin + mDeltaFixed;
            mWiggleCalibration->mTmax = mCalibration->mTmax + mDeltaFixed;
          
            break;
            
        case Date::eDeltaGaussian:
            mWiggleCalibration->mCurve = mCalibration->mCurve;
            mWiggleCalibration->mRepartition = mCalibration->mRepartition;
            
            mWiggleCalibration->mTmin = mCalibration->mTmin + mDeltaFixed;
            mWiggleCalibration->mTmax = mCalibration->mTmax + mDeltaFixed;
          
            break;
            
        default:
            mWiggleCalibration->mCurve = mCalibration->mCurve;
            mWiggleCalibration->mRepartition = mCalibration->mRepartition;
            mWiggleCalibration->mTmin = mCalibration->mTmin ;
            mWiggleCalibration->mTmax = mCalibration->mTmax ;
            break;
    }
    
    

        

   // qDebug()<<"Date::calibrate in project"<<project->mCalibCurves[toFind].mName;
}

const QMap<double, double> Date::getRawCalibMap() const
{
    return vector_to_map(mCalibration->mCurve, mCalibration->mTmin, mCalibration->mTmax, mCalibration->mStep);
}

const QMap<double, double> Date::getFormatedCalibMap() const
{
    if (mCalibration->mCurve.isEmpty())
        return QMap<double, double>();

    QMap<double, double> calib = vector_to_map(mCalibration->mCurve, mCalibration->mTmin, mCalibration->mTmax, mCalibration->mStep);

    QMap<double, double> formatedCalib;
    QMap<double, double>::const_iterator iter = calib.cbegin();
    while (iter!= calib.constEnd()) {
        formatedCalib.insert(DateUtils::convertToAppSettingsFormat(iter.key()), iter.value());
        ++iter;
    }
    return formatedCalib;

}

const QMap<double, double> Date::getRawWiggleCalibMap() const
{
    return vector_to_map(mWiggleCalibration->mCurve, mWiggleCalibration->mTmin, mWiggleCalibration->mTmax, mWiggleCalibration->mStep);
}

const QMap<double, double> Date::getFormatedWiggleCalibMap() const
{
    if (mWiggleCalibration == nullptr || mWiggleCalibration->mCurve.isEmpty())
        return QMap<double, double>();

    QMap<double, double> calib = vector_to_map(mWiggleCalibration->mCurve, mWiggleCalibration->mTmin, mWiggleCalibration->mTmax, mWiggleCalibration->mStep);

    QMap<double, double> formatedWiggleCalib;
    QMap<double, double>::const_iterator iter = calib.cbegin();
    while (iter!= calib.constEnd()) {
        formatedWiggleCalib.insert(DateUtils::convertToAppSettingsFormat(iter.key()), iter.value());
        ++iter;
    }
    return formatedWiggleCalib;

}

QVector<double> Date::getFormatedRepartition() const
{
    if (DateUtils::convertToAppSettingsFormat(mCalibration->mTmin)>DateUtils::convertToAppSettingsFormat(mCalibration->mTmax)) {
       // reverse the QVector and complement, we suppose it's the same step
        QVector<double> repart;
        double lastValue = mCalibration->mRepartition.last();
        QVector<double>::const_iterator iter = mCalibration->mRepartition.cend()-1;
        while (iter!=mCalibration->mRepartition.cbegin()-1) {
             repart.append(lastValue-(*iter));
             --iter;
        }
        return repart;

    } else
        return mCalibration->mRepartition;

}


double Date::getFormatedTminRefCurve() const
{
    return qMin(DateUtils::convertToAppSettingsFormat(getTminRefCurve()),DateUtils::convertToAppSettingsFormat(getTmaxRefCurve()));
}

double Date::getFormatedTmaxRefCurve() const
{
    return qMax(DateUtils::convertToAppSettingsFormat(getTminRefCurve()),DateUtils::convertToAppSettingsFormat(getTmaxRefCurve()));
}

double Date::getFormatedTminCalib() const
{
    return qMin(DateUtils::convertToAppSettingsFormat(mCalibration->mTmin),DateUtils::convertToAppSettingsFormat(mCalibration->mTmax));
}

double Date::getFormatedTmaxCalib()const
{
    return qMax(DateUtils::convertToAppSettingsFormat(mCalibration->mTmin),DateUtils::convertToAppSettingsFormat(mCalibration->mTmax));
}

void Date::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mTheta.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    mSigma.generateHistos(chains, fftLen, bandwidth);

    if ( !( mDeltaType == Date::eDeltaNone ) )
        mWiggle.generateHistos(chains, fftLen, bandwidth);

}

QPixmap Date::generateUnifThumb()
{
    if (mIsValid){
        //  No need to draw the graph on a large size
        //  These values are arbitary
        const QSize size(1000, 150);
        QPixmap thumb(size);

        const double tLower = mData.value(DATE_UNIFORM_MIN_STR).toDouble();
        const double tUpper = mData.value(DATE_UNIFORM_MAX_STR).toDouble();

        const double tmin = mSettings.mTmin;
        const double tmax = mSettings.mTmax;

        if (tLower>tmax ||tmax<tmin) {
            return QPixmap();

        } else {
            QPainter p;
            p.begin(&thumb);
            p.setRenderHint(QPainter::Antialiasing);

            GraphView graph;
            graph.setFixedSize(size);
            graph.setMargins( 0, 0, 0, 0 );

            graph.setRangeX(tmin, tmax);
            graph.setCurrentX(tmin, tmax);
            graph.setRangeY( 0., 1. );

            graph.showXAxisArrow(false);
            graph.showXAxisTicks(false);
            graph.showXAxisSubTicks(false);
            graph.showXAxisValues(false);

            graph.showYAxisArrow(false);
            graph.showYAxisTicks(false);
            graph.showYAxisSubTicks(false);
            graph.showYAxisValues(false);
            graph.showYAxisLine(false);

            graph.setXAxisMode(GraphView::eHidden);
            graph.setYAxisMode(GraphView::eHidden);

            const QColor color = mPlugin->getColor();

            GraphCurve curve;
            curve.mBrush = color;
            curve.mPen = QPen(color, 2.);
            curve.mIsHorizontalSections = true;

            const double tminDisplay = qBound(tmin, tLower, tmax);
            const double tmaxDisplay = qBound(tmin, tUpper, tmax);

            curve.mSections.append(qMakePair(tminDisplay, tmaxDisplay));
            graph.addCurve(curve);

            curve.mName = "Calibration";

            graph.repaint();

            graph.render(&p);
            p.end();

            return thumb;
        }

    } else {
        // If date is invalid, return a null pixmap!
        return QPixmap();
    }

}




QPixmap Date::generateCalibThumb()
{
    if (mIsValid) {
        //  No need to draw the graph on a large size
        //  These values are arbitary
        const QSize size(1000, 150);

        const double tmin = mSettings.mTmin;
        const double tmax = mSettings.mTmax;

        GraphCurve curve;
        QMap<double, double> calib = normalize_map(getMapDataInRange(getRawCalibMap(), tmin, tmax));
        qDebug()<<"generateThumb mName"<<mCalibration->mName<<mCalibration->mCurve.size()<<calib.size();
        curve.mData = calib;

        if (curve.mData.isEmpty())
            return QPixmap();

        curve.mName = "Calibration";

        const QColor color = mPlugin->getColor();

        curve.mPen = QPen(color, 2.f);
        curve.mBrush = color;
        curve.mIsHisto = false;
        curve.mIsRectFromZero = true; // For Unif, Typo !!

        GraphView graph;

        graph.addCurve(curve);
        
        // Drawing the wiggle
        if (mDeltaType != eDeltaNone) {
            GraphCurve curveWiggle;
            QMap<double, double> calibWiggle = normalize_map(getMapDataInRange(getRawWiggleCalibMap(), tmin, tmax));
            curveWiggle.mData = calibWiggle;

            curveWiggle.mName = "Wiggle";
            curveWiggle.mPen = QPen(QColor(150, 150, 150), 2.f);
            curveWiggle.mBrush = QColor(150, 150, 150);//color;
            curveWiggle.mIsHisto = false;
            curveWiggle.mIsRectFromZero = true; 
            graph.addCurve(curveWiggle);
        }
        
        
        graph.setFixedSize(size);
        graph.setMargins(0, 0, 0, 0);

        graph.setRangeX(tmin, tmax);
        graph.setCurrentX(tmin, tmax);
        graph.setRangeY(0, 1.);

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
        graph.showYAxisLine(false);


        QPixmap thumb(size);
        QPainter p;
        p.begin(&thumb);
        //p.setRenderHint(QPainter::SmoothPixmapTransform);//don't work on pixmap
        graph.repaint();
        graph.render(&p);
        p.end();

        return thumb;
    } else {
        // If date is invalid, return a null pixmap!
        return QPixmap();
    }

}

double Date::getLikelihoodFromCalib(const double t)
{
    const double tmin (mCalibration->mTmin);
    const double tmax (mCalibration->mTmax);

    // We need at least two points to interpolate
    if (mCalibration->mCurve.size() < 2 || t < tmin || t > tmax)
        return 0.;

    double prop = (t - tmin) / (tmax - tmin);
    double idx = prop * (mCalibration->mCurve.size() - 1); // tricky : if (tmax - tmin) = 2000, then calib size is 2001 !
    int idxUnder = (int)floor(idx);
    int idxUpper = idxUnder + 1;

    // Important pour le créneau : pas d'interpolation autour des créneaux!
    double v (0.);
    if (mCalibration->mCurve[idxUnder] != 0. && mCalibration->mCurve[idxUpper] != 0.)
        v = interpolate((double) idx, (double)idxUnder, (double)idxUpper, mCalibration->mCurve[idxUnder], mCalibration->mCurve[idxUpper]);
    
    return v;
}


void Date::updateTheta(Event* event)
{
    updateti(this, event);
}
/**
 * @brief Date::initDelta Init the wiggle shift
 */
void Date::initDelta(Event*)
{
    switch (mDeltaType) {
        case eDeltaNone:
            break;
        
        case eDeltaRange:
            mDelta = Generator::randomUniform(mDeltaMin, mDeltaMax);
            break;
        
        case eDeltaGaussian: {
            // change init of Delta in case of gaussian function since 2015/06 with PhL
            //mDelta = event->mTheta.mX - mTheta.mX;
            const double tmin = mSettings.mTmin;
            const double tmax = mSettings.mTmax;
            mDelta = Generator::gaussByDoubleExp(mDeltaAverage,mDeltaError, tmin, tmax);
            }
            break;
        
        case eDeltaFixed:
            mDelta = mDeltaFixed;
            break;
        
    }

    mWiggle.mLastAccepts.clear();
}

void Date::updateDelta(Event* event)
{
    const double lambdai = event->mTheta.mX - mTheta.mX;
    
    switch (mDeltaType) {
        case eDeltaNone:
            break;
        
        case eDeltaRange:
            mDelta = Generator::gaussByDoubleExp(lambdai, mSigma.mX, mDeltaMin, mDeltaMax);
            break;
        
        case eDeltaGaussian: {
           // const double lambdai = event->mTheta.mX - mTheta.mX;
            const double w = ( 1/(mSigma.mX * mSigma.mX) ) + ( 1/(mDeltaError * mDeltaError) );
            const double deltaAvg = (lambdai / (mSigma.mX * mSigma.mX) + mDeltaAverage / (mDeltaError * mDeltaError)) / w;
            const double x = Generator::gaussByBoxMuller(0, 1);
            const double delta = deltaAvg + x / sqrt(w);

            mDelta = delta;
        }
            break;
        
        case eDeltaFixed:
            mDelta = mDeltaFixed;
            break;
        
    }
}

void Date::updateSigmaJeffreys(Event* event)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double lambda = pow(mTheta.mX - (event->mTheta.mX - mDelta), 2) / 2.;

   
    const double a (.0001); //precision
    const double b (pow(mSettings.mTmax - mSettings.mTmin, 2.));

    const double V1 = mSigma.mX * mSigma.mX;

    double V2 (0.);
    do {
        const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigma.mSigmaMH);
        V2 = pow(10, logV2);
    } while ((V2<a) || (V2>b));
    
    
    const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
    const double x2 = V1/V2;
    const double rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1; // (V2 / V1) est le jacobien!
    //qDebug() <<"Date:: updateSigmaJeffreys"<<V2<< rapport;
    
    mSigma.tryUpdate(sqrt(V2), rapport);
}


void Date::updateSigmaShrinkage(Event* event)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double lambda = pow(mTheta.mX - (event->mTheta.mX - mDelta), 2) / 2.;

    const int logVMin (-6);
    const int logVMax (100);

    const double V1 = mSigma.mX * mSigma.mX;
    const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigma.mSigmaMH);
    const double V2 = pow(10, logV2);

    double rapport (0.);
    if (logV2 >= logVMin && logV2 <= logVMax) {
        const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
        const double x2 = pow((event->mS02 + V1) / (event->mS02 + V2), event->mAShrinkage + 1.);
        rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1; // (V2 / V1) est le jacobien!
 
    } else {
        qDebug()<<"Date::updateSigma x1 x2 rapport rejet";
    }
    
    mSigma.tryUpdate(sqrt(V2), rapport);
}

void Date::updateSigmaReParam(Event* event)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double lambda = pow(mTheta.mX - (event->mTheta.mX - mDelta), 2) / 2.;

    const int VMin (0);

    const double V1 = mSigma.mX * mSigma.mX;
  
    const double r1 = pow( (mTheta.mX - (event->mTheta.mX - mDelta))/mSigma.mX, 2);
    const double r2 = Generator::gaussByBoxMuller(r1, mSigma.mSigmaMH);
    const double V2 = pow( (mTheta.mX - (event->mTheta.mX - mDelta)), 2)/ r2;

    double rapport (0.);
    if (V2 > VMin ) {
        const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
        const double x2 = pow((event->mS02 + V1) / (event->mS02 + V2), event->mAShrinkage + 1.);
        
        rapport = x1 * sqrt(V1/V2) * x2 * pow(V2 / V1, 2.); // (V2 / V1) est le jacobien!
 
    } else {
        qDebug()<<"Date::updateSigmaReParam x1 x2 rapport rejet";
    }
    
    mSigma.tryUpdate(sqrt(V2), rapport);
}

void Date::updateWiggle()
{
    mWiggle.mX = mTheta.mX + mDelta;
}

// CSV dates
Date Date::fromCSV(const QStringList &dataStr, const QLocale &csvLocale)
{
    Date date;
    const QString pluginName = dataStr.first();

    PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
    if (plugin) {
        QStringList dataTmp = dataStr.mid(1,dataStr.size()-1);
        date.mName = dataTmp.at(0);
        date.mPlugin = plugin;
        date.mMethod = plugin->getDataMethod();
        date.mData = plugin->fromCSV(dataTmp, csvLocale);

        if (plugin->wiggleAllowed()) {
            int firstColNum = plugin->csvMinColumns() + plugin->csvOptionalColumns();
            if (dataTmp.size() >= firstColNum + 2) {
                QString deltaType = dataTmp.at(firstColNum);
                QString delta1 = dataTmp.at(firstColNum + 1);
                QString delta2 = "0";
                if (dataTmp.size() >= firstColNum + 3) {
                    delta2 = dataTmp.at(firstColNum + 2);
                }
                if (!isComment(deltaType) && !isComment(delta1) && !isComment(delta2)) {
                    if (deltaType == "fixed") {
                        date.mDeltaType = Date::eDeltaFixed;
                        date.mDeltaFixed = csvLocale.toDouble(delta1);
                        
                    } else if (deltaType == "range") {
                        date.mDeltaType = Date::eDeltaRange;
                        date.mDeltaMin = csvLocale.toDouble(delta1);
                        date.mDeltaMax = csvLocale.toDouble(delta2);
                        
                    } else if (deltaType == "gaussian") {
                        date.mDeltaType = Date::eDeltaGaussian;
                        date.mDeltaAverage = csvLocale.toDouble(delta1);
                        date.mDeltaError = csvLocale.toDouble(delta2);
                    }
                }
            }

        }
        date.mIsValid = plugin->isDateValid(date.mData,date.mSettings);
    }

    plugin = nullptr;
    return date;
}

QStringList Date::toCSV(const QLocale &csvLocale) const
{
    QStringList csv;

    csv << mPlugin->getName();
    csv << mName;
    csv << mPlugin->toCSV(mData,csvLocale );

    if (mDeltaType == Date::eDeltaNone) {
        csv << "none";

    } else if (mDeltaType == Date::eDeltaFixed) {
        csv << "fixed";
        csv << csvLocale.toString(mDeltaFixed);

    } else if (mDeltaType == Date::eDeltaRange) {
        csv << "range";
        csv << csvLocale.toString(mDeltaMin);
        csv << csvLocale.toString(mDeltaMax);

    } else if (mDeltaType == Date::eDeltaGaussian) {
        csv << "gaussian";
        csv << csvLocale.toString(mDeltaAverage);
        csv << csvLocale.toString(mDeltaError);
    }

    return csv;
}

void Date::autoSetTiSampler(const bool bSet)
{
    // define sampling function
    // select if using getLikelyhooArg is possible, it's a faster way

    if (bSet && mPlugin!= 0 && mPlugin->withLikelihoodArg() && mOrigin==Date::eSingleDate) {
         //   if (false) {
        switch (mMethod) {
            case eMHSymetric:
                updateti = fMHSymetricWithArg;
                break;
            
            case eInversion:
                updateti = fInversionWithArg;
                break;
            
                // Seul cas où le taux d'acceptation a du sens car on utilise sigmaMH :
            case eMHSymGaussAdapt:
                updateti = fMHSymGaussAdaptWithArg;
                break;
            
            default:
                break;
            
        }
       // qDebug()<<"Date::autoSetTiSampler()"<<this->mName<<"with getLikelyhoodArg";
    } else {
        switch (mMethod) {
            case eMHSymetric:
                updateti = fMHSymetric;
                break;
            
            case eInversion:
                updateti = fInversion;
                break;
            
                // only case with acceptation rate, because we use sigmaMH :
            case eMHSymGaussAdapt:
                updateti = fMHSymGaussAdapt;
                break;
            
            default:
                break;
            }
        }


    }



/**
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

        const double tiNew = Generator::gaussByBoxMuller(event->mTheta.mX - date->mDelta, date->mSigma.mX);
        const double rapport = date->getLikelihood(tiNew) / date->getLikelihood(date->mTheta.mX);

        date->mTheta.tryUpdate(tiNew, rapport);


}
/**
 * @brief MH proposal = prior distribution
 * @brief identic as fMHSymetric but use getLikelyhoodArg, when plugin offer it
 *
 */
void fMHSymetricWithArg(Date* date,Event* event)
{

    const double tiNew = Generator::gaussByBoxMuller(event->mTheta.mX - date->mDelta, date->mSigma.mX);

    QPair<long double, long double> argOld, argNew;

    argOld = date->getLikelihoodArg(date->mTheta.mX);
    argNew = date->getLikelihoodArg(tiNew);

    const long double rapport=sqrt(argOld.first/argNew.first)*exp(argNew.second-argOld.second);

    date->mTheta.tryUpdate(tiNew, (double)rapport);

}

/**
 *  @brief Calculation of proposal density for time value t
 *
 */
double fProposalDensity(const double t, const double t0, Date* date)
{
    (void) t0;
    const double tmin (date->mSettings.mTmin);
    const double tmax (date->mSettings.mTmax);
    const double level (date->mMixingLevel);
    double q1 (0.);

    const double tminCalib (date->mCalibration->mTmin);
    const double tmaxCalib (date->mCalibration->mTmax);

    // ----q1------Defined only on Calibration range-----
    if (t > tminCalib && t < tmaxCalib){
        //double prop = (t - tmin) / (tmax - tmin);
        const double prop = (t - tminCalib) / (tmaxCalib - tminCalib);
        const double idx = prop * (date->mCalibration->mRepartition.size() - 1);
        const int idxUnder = (int)floor(idx);

       const double step (date->mCalibration->mStep);

        q1 = (date->mCalibration->mRepartition[idxUnder+1] - date->mCalibration->mRepartition[idxUnder])/step;
        
      //  q1 = interpolate(idx, double(idxUnder), double(idxUnder+1), date->mCalibration->mCurve[idxUnder], date->mCalibration->mCurve[idxUnder+1]);
    }
    // ----q2 shrinkage-----------
    /*double t0 =(tmax+tmin)/2;
    double s = (tmax-tmin)/2;
    double q2= s / ( 2* pow((s+fabs(t-t0)), 2) );
     */

    // ----q2 gaussian-----------
    //double t0 = (tmax+tmin)/2;
    
    //modif 2020-09-28
//    const double sigma = (qMax(tmax, tmaxCalib)  - qMin(tminCalib, tmin)) / 2.;
//    const double mean = (qMax(tmax, tmaxCalib)  + qMin(tminCalib, tmin)) / 2.;
//    const double q2 = Generator::gaussByBoxMuller(mean,  pow(sigma, 2.));
//    
    // origin
    const double sigma = qMax(tmax - tmin, tmaxCalib - tminCalib) / 2;
    const double q2 = exp(-0.5* pow((t-t0)/ sigma, 2)) / (sigma*sqrt(2*M_PI));

    return (level * q1 + (1 - level) * q2);
}

/**
 *  @brief MH proposal = Distribution of Calibrated date, ti is defined on set R (real numbers)
 *  @brief simulation according to uniform shrinkage with s parameter
 */
void fInversion(Date* date, Event* event)
{
    const double u1 = Generator::randomUniform();
    const double level (date->mMixingLevel);
    double tiNew;
    const double tmin (date->mSettings.mTmin);
    const double tmax (date->mSettings.mTmax);

    const double tminCalib = date->mCalibration->mTmin;

    if (u1<level) { // tiNew always in the study period
        const double idx = vector_interpolate_idx_for_value(u1, date->mCalibration->mRepartition);
        tiNew = tminCalib + idx *date->mCalibration->mStep;
    } else {
        // -- gaussian
        const double t0 (date->mTheta.mX);
        const double s = (tmax-tmin)/2.;

        tiNew = Generator::gaussByBoxMuller(t0, s);
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

    const double rapport1 = date->getLikelihood(tiNew) / date->getLikelihood(date->mTheta.mX);

    const double rapport2 = exp((-0.5 / (date->mSigma.mX * date->mSigma.mX)) *
                          (pow(tiNew - (event->mTheta.mX - date->mDelta), 2) -
                           pow(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2))
                          );

    const double rapport3 = fProposalDensity(date->mTheta.mX, tiNew, date) /
                        fProposalDensity(tiNew, date->mTheta.mX, date);


    date->mTheta.tryUpdate(tiNew, rapport1 * rapport2 * rapport3);
}

void fInversionWithArg(Date* date, Event* event)
{
    const double u1 = Generator::randomUniform();
    const double level (date->mMixingLevel);
    double tiNew;
    const double tmin (date->mSettings.mTmin);
    const double tmax (date->mSettings.mTmax);

    const double tminCalib = date->mCalibration->mTmin;

    if (u1<level) { // tiNew always in the study period
        const double u2 = Generator::randomUniform();
        const double idx = vector_interpolate_idx_for_value(u2, date->mCalibration->mRepartition);
        tiNew = tminCalib + idx *date->mCalibration->mStep;


    } else {
        // -- gaussian
        const double t0 =(tmax+tmin)/2.;
        const double s = (tmax-tmin)/2.;

        tiNew = Generator::gaussByBoxMuller(t0, s);
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


    QPair<long double, long double> argOld, argNew;

    argOld = date->getLikelihoodArg(date->mTheta.mX);
    argNew = date->getLikelihoodArg(tiNew);

    const long double logGRapport = argNew.second-argOld.second;
    const long double logHRapport = (-0.5l/powl(date->mSigma.mX, 2.)) * (  powl(tiNew - (event->mTheta.mX - date->mDelta), 2.) - powl(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2.) ); // modif 2020-09-28

    const long double rapport = sqrt(argOld.first/argNew.first) * exp(logGRapport+logHRapport);

    const long double rapportPD = fProposalDensity(date->mTheta.mX, tiNew, date) / fProposalDensity(tiNew, date->mTheta.mX, date);
    qDebug()<<"fInversionWithArg "<<   double(argOld.first)<< double(argNew.first) <<double(logGRapport)<< double(logHRapport)<<double(rapport) << date->mSigma.mX<< date->mTheta.mX<< tiNew;
    date->mTheta.tryUpdate(tiNew, (double)(rapport * rapportPD));
    
    qDebug()<<"fInversionWithArg "<< (tiNew==date->mTheta.mX?true:false);


}


/**
 * @brief MH proposal = adaptatif Gaussian random walk, ti is defined on set R (real numbers)
 */
void fMHSymGaussAdapt(Date* date, Event* event)
{
    const double tiNew = Generator::gaussByBoxMuller(date->mTheta.mX, date->mTheta.mSigmaMH);
    double rapport = date->getLikelihood(tiNew) / date->getLikelihood(date->mTheta.mX);
    rapport *= exp((-0.5/(date->mSigma.mX * date->mSigma.mX)) * (   pow(tiNew - (event->mTheta.mX - date->mDelta), 2)
                                                                  - pow(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2)
                                                                 ));

    date->mTheta.tryUpdate(tiNew, rapport);
}


/**
 * @brief MH proposal = adaptatif Gaussian random walk, ti is not constraint being on the study period
 *
 * @brief identic as fMHSymGaussAdapt but use getLikelyhoodArg, when plugin offer it
 */
void fMHSymGaussAdaptWithArg(Date* date, Event* event)
{
    const double tiNew = Generator::gaussByBoxMuller(date->mTheta.mX, date->mTheta.mSigmaMH);

    QPair<long double, long double> argOld, argNew;

    argOld = date->getLikelihoodArg(date->mTheta.mX);
    argNew = date->getLikelihoodArg(tiNew);

    const long double logGRapport = argNew.second - argOld.second;
    const long double logHRapport = (-0.5 / (date->mSigma.mX * date->mSigma.mX)) * (  pow(tiNew - (event->mTheta.mX - date->mDelta), 2)
                                                                      - pow(date->mTheta.mX - (event->mTheta.mX - date->mDelta), 2)
                                                                      );

    const long double rapport = sqrt(argOld.first / argNew.first) * exp(logGRapport+logHRapport);

    date->mTheta.tryUpdate(tiNew, (double) rapport);

}
