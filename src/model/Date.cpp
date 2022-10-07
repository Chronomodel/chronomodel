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
//#include "AbstractItem.h"
//#include "Event.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "PluginManager.h"
#include "PluginUniform.h"
#include "../PluginAbstract.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "ProjectSettings.h"
#include "ModelUtilities.h"
#include "Project.h"
#include "MainWindow.h"
#include "GraphCurve.h"

#if USE_FFT
#include "fftw3.h"
#endif

#include <QDebug>

Dato::Dato():
    mName("No Named Dato")
{

}
Dato::~Dato()
{}

void Dato::fromJson(const QJsonObject& json)
{
    //Q_ASSERT(&json);
 //   mId = json.value(STATE_ID).toInt();
    mName = json.value(STATE_NAME).toString();
 /*   mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                    json.value(STATE_COLOR_GREEN).toInt(),
                    json.value(STATE_COLOR_BLUE).toInt());



    mUUID = json.value(STATE_DATE_UUID).toString();

    if (mUUID.isEmpty())
        mUUID = QString::fromStdString( Generator::UUID());

    // Copy plugin specific values for this data :
    mData = json.value(STATE_DATE_DATA).toObject();
    mOrigin = (OriginType)json.value(STATE_DATE_ORIGIN).toInt();

    QString pluginId = json.value(STATE_DATE_PLUGIN_ID).toString();
    mPlugin = PluginManager::getPluginFromId(pluginId);
    mMethod = (DataMethod)json.value(STATE_DATE_METHOD).toInt();
    mIsValid = json.value(STATE_DATE_VALID).toBool();

    mDeltaType = (DeltaType)json.value(STATE_DATE_DELTA_TYPE).toInt();
    mDeltaFixed = json.value(STATE_DATE_DELTA_FIXED).toDouble();
    mDeltaMin = json.value(STATE_DATE_DELTA_MIN).toDouble();
    mDeltaMax = json.value(STATE_DATE_DELTA_MAX).toDouble();
    mDeltaAverage = json.value(STATE_DATE_DELTA_AVERAGE).toDouble();
    mDeltaError = json.value(STATE_DATE_DELTA_ERROR).toDouble();

    mIsCurrent = false;
    mIsSelected = false;

    Project* project = MainWindow::getInstance()->getProject();
    mSettings = ProjectSettings::fromJson(project->mState.value(STATE_SETTINGS).toObject()); // ProjectSettings::fromJson is static
    mSubDates = json.value(STATE_DATE_SUB_DATES).toArray();

    mMixingLevel = project->mState.value(STATE_MCMC_MIXING).toDouble();

    if (mPlugin == nullptr)
        throw QObject::tr("Data could not be loaded : invalid plugin : %1").arg(pluginId);

    else  {
        if (mOrigin == eSingleDate) {
            QPair<double, double> tminTmax = mPlugin->getTminTmaxRefsCurve(mData);
            mTminRefCurve = tminTmax.first;
            mTmaxRefCurve = tminTmax.second;

        } else if (mOrigin == eCombination) {
            double tmin (+INFINITY);
            double tmax (-INFINITY);
            TDate sd;
            for (auto&& d : mSubDates ) {

                const bool hasWiggle (d.toObject().value(STATE_DATE_DELTA_TYPE).toInt() != TDate::eDeltaNone);
                QString toFind;
                if (hasWiggle) {
                    toFind = "WID::" + d.toObject().value(STATE_DATE_UUID).toString();

                } else {
                     toFind = d.toObject().value(STATE_DATE_UUID).toString();
                }

                QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (toFind);

                if ( it != project->mCalibCurves.end()) {
                    CalibrationCurve* d_mCalibration = & it.value();
                    tmin = std::min(d_mCalibration->mTmin, tmin);
                    tmax = std::max(d_mCalibration->mTmax, tmax);

                } else {

                    sd.fromJson(d.toObject());
                    sd.calibrate(mSettings, project);
                    tmin = std::min(sd.mCalibration->mTmin, tmin);
                    tmax = std::max(sd.mCalibration->mTmax, tmax);
                }






            }
            mTminRefCurve = tmin;
            mTmaxRefCurve = tmax;
        }
    }

    mTheta.mProposal = ModelUtilities::getDataMethodText(mMethod);
    mTheta.setName("Theta of date : "+ mName);
    mSigma.mProposal = ModelUtilities::getDataMethodText(TDate::eMHSymGaussAdapt);
    mSigma.setName("Sigma of date : "+ mName);



    QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (mUUID);
    if ( it != project->mCalibCurves.end()) {
        mCalibration = & it.value();

     } else {
        mCalibration = nullptr;
     }

    QString toFind = "WID::" + mUUID;
    it = project->mCalibCurves.find (toFind);
    if ( it != project->mCalibCurves.end()) {
        mWiggleCalibration = & it.value();

    } else {
        mWiggleCalibration = nullptr;
    }
*/
}




Date::Date():
mName("No Named Date")
{
    mColor = Qt::blue;
    mOrigin = eSingleDate;
    mPlugin = nullptr;
    mTi.mSupport = MetropolisVariable::eR;
    mSigmaTi.mSupport = MetropolisVariable::eRp;
    mWiggle.mSupport = MetropolisVariable::eR;

    mTi.mFormat = DateUtils::eUnknown;
    mSigmaTi.mFormat = DateUtils::eUnknown;
    mWiggle.mFormat = DateUtils::eUnknown;

    mId = -1;
    mUUID = QString("NONE");

    mTi.mSamplerProposal = MHVariable::eMHSymetric;
    mSigmaTi.mSamplerProposal = MHVariable::eMHAdaptGauss;
    //updateti = fMHSymetric;

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
   // mSubTDates.clear();

    mTminRefCurve = -INFINITY;
    mTmaxRefCurve = INFINITY;

    mCalibration = nullptr;
    mWiggleCalibration = nullptr;
}

Date::Date(const QJsonObject& json)
{
    fromJson(json);
}

Date::Date(PluginAbstract* plugin):
mName("No Named TDate")
{
    init();
    mPlugin = plugin;
}

void Date::init()
{
    mColor = Qt::blue;
    mOrigin = eSingleDate;
    mPlugin = nullptr;
    mTi.mSupport = MetropolisVariable::eR;
    mTi.mSamplerProposal = MHVariable::eMHSymetric;

    mSigmaTi.mSupport = MetropolisVariable::eRp;
    mWiggle.mSupport = MetropolisVariable::eR;

    mTi.mFormat = DateUtils::eUnknown;
    mSigmaTi.mFormat = DateUtils::eUnknown;
    mWiggle.mFormat = DateUtils::eUnknown;

    mId = -1;
    mUUID = QString("NONE");
    //mMethod = eMHSymetric;
    //updateti = fMHSymetric;

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
   // mSubTDates.clear();

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
    mTi = date.mTi;
    mSigmaTi = date.mSigmaTi;
    mWiggle = date.mWiggle;
    mDelta = date.mDelta;

    mId = date.mId;
    mUUID = date.mUUID;

    mName = date.mName;
    mColor = date.mColor;

    mData = date.mData;
    mOrigin = date.mOrigin;
    mPlugin = date.mPlugin;
    //mMethod = date.mMethod;
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
    mSettings = date.mSettings;

    mSubDates = date.mSubDates;
    mMixingLevel = date.mMixingLevel;

    updateti = date.updateti;

    mTminRefCurve = date.mTminRefCurve;
    mTmaxRefCurve = date.mTmaxRefCurve;
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
    //Q_ASSERT(&json);

    mId = json.value(STATE_ID).toInt();
    mName = json.value(STATE_NAME).toString();
    mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                    json.value(STATE_COLOR_GREEN).toInt(),
                    json.value(STATE_COLOR_BLUE).toInt());



    mUUID = json.value(STATE_DATE_UUID).toString();

    if (mUUID.isEmpty())
        mUUID = QString::fromStdString( Generator::UUID());

    // Copy plugin specific values for this data :
    mData = json.value(STATE_DATE_DATA).toObject();
    mOrigin = (OriginType)json.value(STATE_DATE_ORIGIN).toInt();

    QString pluginId = json.value(STATE_DATE_PLUGIN_ID).toString();
    mPlugin = PluginManager::getPluginFromId(pluginId);
    //mMethod = (DataMethod)json.value(STATE_DATE_METHOD).toInt();

    mIsValid = json.value(STATE_DATE_VALID).toBool();

    mDeltaType = (DeltaType)json.value(STATE_DATE_DELTA_TYPE).toInt();
    mDeltaFixed = json.value(STATE_DATE_DELTA_FIXED).toDouble();
    mDeltaMin = json.value(STATE_DATE_DELTA_MIN).toDouble();
    mDeltaMax = json.value(STATE_DATE_DELTA_MAX).toDouble();
    mDeltaAverage = json.value(STATE_DATE_DELTA_AVERAGE).toDouble();
    mDeltaError = json.value(STATE_DATE_DELTA_ERROR).toDouble();

    mIsCurrent = false;
    mIsSelected = false;
     
    Project* project = MainWindow::getInstance()->getProject();
    mSettings = ProjectSettings::fromJson(project->mState.value(STATE_SETTINGS).toObject()); // ProjectSettings::fromJson is static
    mSubDates = json.value(STATE_DATE_SUB_DATES).toArray();

    mMixingLevel = project->mState.value(STATE_MCMC_MIXING).toDouble();

    if (mPlugin == nullptr)
        throw QObject::tr("Data could not be loaded : invalid plugin : %1").arg(pluginId);

    else  {
        if (mOrigin == eSingleDate) {
            QPair<double, double> tminTmax = mPlugin->getTminTmaxRefsCurve(mData);
            mTminRefCurve = tminTmax.first;
            mTmaxRefCurve = tminTmax.second;
            
        } else if (mOrigin == eCombination) {
            double tmin (+INFINITY);
            double tmax (-INFINITY);
            Date sd;
            for (auto&& d : mSubDates ) {

                const bool hasWiggle (d.toObject().value(STATE_DATE_DELTA_TYPE).toInt() != eDeltaNone);
                QString toFind;
                if (hasWiggle) {
                    toFind = "WID::" + d.toObject().value(STATE_DATE_UUID).toString();

                } else {
                     toFind = d.toObject().value(STATE_DATE_UUID).toString();
                }

                QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (toFind);

                if ( it != project->mCalibCurves.end()) {
                    CalibrationCurve* d_mCalibration = & it.value();
                    tmin = std::min(d_mCalibration->mTmin, tmin);
                    tmax = std::max(d_mCalibration->mTmax, tmax);

                } else {
                    /* When reading the .chr file without the presence of the .cal file, there is no calibration for the subdates
                     *  and the display of the curves in CalibrationView crashes.
                     */
                    sd.fromJson(d.toObject());
                    sd.calibrate(mSettings, project);
                    tmin = std::min(sd.mCalibration->mTmin, tmin);
                    tmax = std::max(sd.mCalibration->mTmax, tmax);
                }


            /*    } else {
                     //toFind = d.toObject().value(STATE_DATE_UUID).toString();
               }
            */
             /*   auto djson = d.toObject();
                    QPair<double, double> tminTmax = mPlugin->getTminTmaxRefsCurve( djson.value(STATE_DATE_DATA).toObject());// d.toObject());
                    tmin = std::max(tminTmax.first, tmin);
                    tmax = std::min(tminTmax.second, tmax);
              */



            }
            mTminRefCurve = tmin;
            mTmaxRefCurve = tmax;
        }
    }

    //mTheta.mProposal = ModelUtilities::getDataMethodText(mMethod);
    mTi.setName("Theta of date : "+ mName);
    mTi.mSamplerProposal = (MHVariable::SamplerProposal)json.value(STATE_DATE_SAMPLER).toInt();

    mSigmaTi.mSamplerProposal = MHVariable::eMHSymGaussAdapt;
    mSigmaTi.setName("Sigma of date : "+ mName);



    QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (mUUID);
    if ( it != project->mCalibCurves.end()) {
        mCalibration = & it.value();

     } else {
        mCalibration = nullptr;
     }

    QString toFind = "WID::" + mUUID;
    it = project->mCalibCurves.find (toFind);
    if ( it != project->mCalibCurves.end()) {
        mWiggleCalibration = & it.value();

    } else {
        mWiggleCalibration = nullptr;
    }

}

QJsonObject Date::toJson() const
{
    QJsonObject date;
    date[STATE_ID] = mId;
    date[STATE_DATE_UUID] = mUUID;
    date[STATE_NAME] = mName;
    date[STATE_DATE_DATA] = mData;
    date[STATE_DATE_ORIGIN] = mOrigin;
    date[STATE_DATE_PLUGIN_ID] = mPlugin->getId();
    date[STATE_DATE_SAMPLER] = mTi.mSamplerProposal;
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


/**
 * @brief Date::getLikelihood is called by TDate::calibrate. When creating the calibration curve mCalibrate does not exist.
 * Then the calibration curve is used to determine the likelihood.
 * @param t
 * @return
 */
long double Date::getLikelihood(const double& t) const
{
     if (mPlugin) {
        if (mOrigin == eSingleDate) {
            return mPlugin->getLikelihood(t, mData);
            
        } else if (mOrigin == eCombination) { 
            // If needed run the wiggle calculation
            if (mCalibration==nullptr || mCalibration->mCurve.isEmpty()) {
                return mPlugin->getLikelihoodCombine(t, mSubDates, mSettings.mStep);

            } else {
                return getLikelihoodFromCalib(t);
            }
            
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
                     if (mCalibration->mCurve.isEmpty()) {
                         return QPair<long double, long double>(log(mPlugin->getLikelihoodCombine(t, mSubDates, mSettings.mStep)), 1.l);
                     } else {
                         return QPair<long double, long double>(log(getLikelihoodFromCalib(t)), 1.l);
                     }
                   
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


// Replaces the function ModelUtilities::getDeltaText(d))
QString Date::getWiggleDesc() const
{
    QString res;
      
    switch (mDeltaType) {
        case Date::eDeltaFixed:
            res = "Wiggle = " + QString::number(mDeltaFixed);
        break;
        case Date::eDeltaRange:
            res = "Wiggle = U[" + QString::number(mDeltaMin) + " ; " + QString::number(mDeltaMax) + " ] " ;
        break;
        case Date::eDeltaGaussian:
            res = "Wiggle = N(" + QString::number(mDeltaAverage) + " ; " + QString::number(mDeltaError) + " ) ";
        break;
        case Date::eDeltaNone:
        default:
            res = "";
        break;
    }
    

    return res;
}

QString Date::getWiggleDesc(const QJsonObject& json)
{
    QString res;

    switch (json.value(STATE_DATE_DELTA_TYPE).toInt()) {
        case Date::eDeltaFixed:
            res = "Wiggle = " + QString::number(json.value(STATE_DATE_DELTA_FIXED).toDouble());
        break;
        case Date::eDeltaRange:
            res = "Wiggle = U[" + QString::number(json.value(STATE_DATE_DELTA_MIN).toDouble()) + " ; " + QString::number(json.value(STATE_DATE_DELTA_MAX).toDouble()) + " ] " ;
        break;
        case Date::eDeltaGaussian:
            res = "Wiggle = N(" + QString::number(json.value(STATE_DATE_DELTA_AVERAGE).toDouble()) + " ; " + QString::number(json.value(STATE_DATE_DELTA_ERROR).toDouble()) + " ) ";
        break;
        case Date::eDeltaNone:
        default:
            res = "";
        break;
    }


    return res;
}




void Date::reset()
{
    mTi.reset();
    mSigmaTi.reset();
    mWiggle.reset();
}

/**
 * @brief TDate::calibrate
 * Function that calculates the calibrated density and updates the wiggle density if necessary
 * @param settings
 * @param project
 * @param truncate Restrict the calib and repartition vectors to where data are
 */
void Date::calibrate(const ProjectSettings& settings, Project *project, bool truncate)
{
  // Check if the ref curve is in the plugin list

    if (mOrigin == eSingleDate) {
        const QStringList refsNames = mPlugin->getRefsNames();
        const QString dateRefName = mPlugin->getDateRefCurveName(this);
            
        if (!dateRefName.isEmpty() && !refsNames.contains(dateRefName) )
            return;
        
    }
    
    // add the calibration
    QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (mUUID);

    if ( it == project->mCalibCurves.end()) {
        qDebug()<<"Curve to create mUUID: "<< mUUID ;
        project->mCalibCurves.insert(mUUID, CalibrationCurve());
        
    } else if ( it->mDescription == getDesc() ) {
        // Controls whether the curve has already been calculated using the description
        calibrateWiggle(settings, project);
        
        return;
    }
        
  // Update of the new calibration curve

    mCalibration = & (project->mCalibCurves[mUUID]);
    mCalibration -> mDescription = getDesc();

    mCalibration->mStep = settings.mStep;
    mCalibration->mPluginId = mPlugin->getId();
    mCalibration->mPlugin = mPlugin;
    mCalibration->mName = mName;

    mCalibHPD.clear();

    double tminCal;
    double tmaxCal;

    /* --------------------------------------------------
     *  Calibrate on the whole calibration period (= ref curve definition domain)
     * -------------------------------------------------- */
   
    if (mTmaxRefCurve > mTminRefCurve) {
        QVector<double> calibrationTemp;
        QVector<double> repartitionTemp;

        const double nbRefPts = 1. + round((mTmaxRefCurve - mTminRefCurve) / settings.mStep);
        long double v = getLikelihood(mTminRefCurve);
        calibrationTemp.append(v);
        repartitionTemp.append(0.);
        long double lastRepVal = v;
/* Ne PAS mettre de progressDialog dans cette fonction car il crée un appel à EventItem::SetEvent() qui renvoie un updateProject() et reboucle
 */

 /* Warning: If the progress dialog is modal (see QProgressDialog::QProgressDialog()), setValue() calls QCoreApplication::processEvents(),
 * so take care that this does not cause undesirable re-entrancy in your code. For example, don't use a QProgressDialog inside a paintEvent()!
 *
           QProgressDialog *progress = new QProgressDialog(QTranslator::tr("Calibration generation") + " : "+ mName, QTranslator::tr("Wait") , 1, 10);
          progress->blockSignals(true);
           progress->setWindowModality(Qt::WindowModal);
           progress->setCancelButton(nullptr);
           progress->setMinimumDuration(5);
           progress->setMinimum(0);
           progress->setMaximum(nbRefPts);
           progress->setMinimumWidth(int (progress->fontMetrics().boundingRect(progress->labelText()).width() * 1.5));
*/

        /* We use long double type because
         * after several sums, the repartition can be in the double type range
        */
        double t;
        long double lastV;
        long double rep;

        for (int i = 1; i <= nbRefPts; ++i) {
//            progress->setValue(i);

            t = mTminRefCurve + double (i) * settings.mStep;
            lastV = v;
            v = getLikelihood(t);

            calibrationTemp.append(double(v));
            rep = lastRepVal;
            if (v != 0.l && lastV != 0.l)
                rep = lastRepVal + (long double) (settings.mStep) * (lastV + v) / 2.l;

            repartitionTemp.append(double (rep));
            lastRepVal = rep;
        }
//        progress->close();
 //       delete progress;
        /* ------------------------------------------------------------------
         *  Restrict the calib and repartition vectors to where data are
         * ------------------------------------------------------------------ */

        if (repartitionTemp.last() > 0.) {
            if (truncate) {
                const double threshold (0.00001);
                const int minIdx = int (floor(vector_interpolate_idx_for_value(threshold * lastRepVal, repartitionTemp)));
                const int maxIdx = int (ceil(vector_interpolate_idx_for_value((1. - threshold) * lastRepVal, repartitionTemp)));

                tminCal = mTminRefCurve + minIdx * settings.mStep;
                tmaxCal = mTminRefCurve + maxIdx * settings.mStep;

                // Truncate both functions where data live
                mCalibration->mCurve = calibrationTemp.mid(minIdx, (maxIdx - minIdx) + 1);
                mCalibration->mRepartition = repartitionTemp.mid(minIdx, (maxIdx - minIdx) + 1);

                /* NOTE ABOUT THIS APPROMIATION :
                 * By truncating the calib and repartition, the calib density's area is not 1 anymore!
                 * It is now 1 - 2*threshold = 0,99999... We consider it to be 1 anyway!
                 * By doing this, calib and repartition are stored on a restricted number of data
                 * instead of storing them on the whole reference curve's period (as done for calibrationTemp & repartitionTemp above).
                 */
              } else {
                tminCal = mTminRefCurve;
                tmaxCal = mTminRefCurve;

            }
            // Stretch repartition curve so it goes from 0 to 1
            mCalibration->mRepartition = stretch_vector(mCalibration->mRepartition, 0., 1.);

            // Approximation : even if the calib has been truncated, we consider its area to be = 1
            mCalibration->mCurve = equal_areas(mCalibration->mCurve, settings.mStep, 1.);

        }
        /* ------------------------------------------------------------------
         *  Measurement is very far from Ref curve on the whole ref curve preriod!
         *  => Calib values are very small, considered as being 0 even using "double" !
         *  => lastRepVal = 0, and impossible to truncate using it....
         *  => So,
         * ------------------------------------------------------------------ */
        else  {
            tminCal = mTminRefCurve;
            tmaxCal = mTmaxRefCurve;
        }

        mCalibration->mTmin = tminCal;
        mCalibration->mTmax = tmaxCal;
        // If the calibration curve changes, the wiggle curve must be recalculated.
        if (mWiggleCalibration != nullptr)  {
            const QString toFind ("WID::" + mUUID);
            QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (toFind);
            project->mCalibCurves.erase(it);
        }
    }
    else {
        // Impossible to calibrate because the plugin could not return any calib curve definition period.
        // This may be due to invalid ref curve files or to polynomial equations with only imaginary solutions (See Gauss Plugin...)
    }

    
    /* WIGGLE CALIBRATION CURVE */
    if (mDeltaType != eDeltaNone) {
        calibrateWiggle(settings, project);
    }
    
}


/**
 * @brief TDate::calibrateWiggle Function that calculates the wiggle density according to the defined wiggle type
 * @param settings
 * @param project
 */
void Date::calibrateWiggle( const ProjectSettings& settings, Project *project)
{
  // Check if the ref curve is in the plugin list
      if (mDeltaType == Date::eDeltaNone) {
        mWiggleCalibration = nullptr;
        return;
    }
    // add the calibration

    // We need to keep the calibration curve and then the whole wiggle on the whole support,
    // to allow a more accurate combination when the densities are far away.




    const QString toFind ("WID::" + mUUID);
    QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (toFind);

    
    if ( it == project->mCalibCurves.end()) {
        qDebug()<<" Curve to create Wiggle: "<< toFind ;
        qDebug() << "create Wiggle descript: " << getWiggleDesc() ;
        project->mCalibCurves.insert(toFind, CalibrationCurve());
        
        
    } else if ( it->mDescription == getWiggleDesc() ) {
        // Controls whether the curve has already been calculated using the description
        qDebug() << "The curve already exists Wiggle:" << toFind ;
        qDebug() << "Wiggle descript: " << getWiggleDesc() ;
        return;
        
    }
        
    
    
  // Update of the new calibration curve, on the whole reference curve
    QVector<double> calibrationTemp;
    QPair<double, double> tminTmax = mPlugin->getTminTmaxRefsCurve(mData);
    const double minRefCurve = tminTmax.first;
    const double maxRefCurve = tminTmax.second;

    const double nbRefPts = 1. + round((maxRefCurve - minRefCurve) / double(settings.mStep));
    calibrationTemp.append(getLikelihood(minRefCurve));

    /* We use long double type because
     * after several sums, the repartition can be in the double type range
    */
    double t;
    for (int i = 1; i <= nbRefPts; ++i) {
        t = minRefCurve + double (i) * settings.mStep;
        calibrationTemp.append(double(getLikelihood(t)));
    }


    mWiggleCalibration = & (project->mCalibCurves[toFind]);

    mWiggleCalibration->mDescription = getWiggleDesc();

    mWiggleCalibration->mStep = settings.mStep;
    mWiggleCalibration->mPluginId = mPlugin->getId();
    mWiggleCalibration->mPlugin = mPlugin;
    mWiggleCalibration->mName = mName;


    /* --------------------------------------------------
     *  Calibrate on the whole calibration period (= ref curve definition domain)
     * -------------------------------------------------- */
    switch (mDeltaType) {
        case eDeltaFixed:
        {
                mWiggleCalibration->mCurve = calibrationTemp;
                mWiggleCalibration->mTmin = minRefCurve + mDeltaFixed;
                mWiggleCalibration->mTmax = maxRefCurve + mDeltaFixed;
            }
            break;
                
                
        case eDeltaRange:
        {
        /* ----- FFT -----
         http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        https://jperalta.wordpress.com/2006/12/12/using-fftw3/
        https://dsp.stackexchange.com/questions/22145/perform-convolution-in-frequency-domain-using-fftw
         */

        const int inputSize (calibrationTemp.size());

        const double L ((mDeltaMax-mDeltaMin+1) / mSettings.mStep);

        const int gateSize (std::max(inputSize, int(L)) );
        const int paddingSize (3*gateSize);

        const int N ( inputSize + 4*paddingSize);
        const int NComplex (2* (N/2)+1);

        double *inputReal;
        inputReal = new double [N];

        fftw_complex *inputComplex;
        inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);

        for (int i (0); i< paddingSize; i++) {
            inputReal[i] = 0.;
        }
        for (int i (0); i< inputSize; i++) {
            inputReal[i+paddingSize] = calibrationTemp[i];
        }
        for (int i ( inputSize+paddingSize); i< N; i++) {
            inputReal[i] = 0.;
        }
       fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);
       fftw_execute(plan_input);


       // ---- gate
       double *gateReal;
       gateReal = new double [N];

       fftw_complex *gateComplex;
       gateComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);


       for (int i (0); i< (N-L)/2; i++) {
           gateReal[i] = 0.;
       }
       for (int i ((N-L)/2); i< ((N+L)/2); i++) {
           gateReal[i] = 1.;
       }
       for (int i ((N+L)/2); i< N; i++) {
           gateReal[i] = 0.;
       }

      fftw_plan plan_gate = fftw_plan_dft_r2c_1d(N, gateReal, gateComplex, FFTW_ESTIMATE);
      fftw_execute(plan_gate);


       /*
        * The value of inputComplex[i=0] is a constant of the offset of the signal
        */

        double *outputReal;
        outputReal = new double [N];

        fftw_complex *outputComplex;
        outputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);


        for (int i= 0; i<NComplex; ++i) {
            outputComplex[i][0] = gateComplex[i][0] * inputComplex[i][0] - gateComplex[i][1] * inputComplex[i][1];
            outputComplex[i][1] = gateComplex[i][0] * inputComplex[i][1] + gateComplex[i][1] * inputComplex[i][0];
        }

        fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, outputComplex, outputReal, FFTW_ESTIMATE);
        fftw_execute(plan_output);

/*
 * This code corresponds to the theoretical formula in Fourier space.
 * But it does not work with uniform densities, because it does not handle the padding correctly.
 * The problem also exists for densities with a very small support .

   double factor;
        for (int i(0); i<NComplex; ++i) {

            factor = sinc((double)i, L/(double)inputSize );

            outputComplex[i][0] = inputComplex[i][0]* factor;
            outputComplex[i][1] = inputComplex[i][1]* factor;
        }

        fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, outputComplex, outputReal, FFTW_ESTIMATE);
        fftw_execute(plan_output);
*/


        QVector<double> curve;

        for ( int i = 0; i < N ; i++) {
            curve.append(outputReal[i]);
        }



        mWiggleCalibration->mCurve = equal_areas(curve, mSettings.mStep, 1.);

        mWiggleCalibration->mTmin = minRefCurve - (double)(3*paddingSize +inputSize/2)*mSettings.mStep + (mDeltaMin+mDeltaMax)/2.;
        mWiggleCalibration->mTmax = mWiggleCalibration->mTmin + curve.size()*mSettings.mStep;

        fftw_destroy_plan(plan_input);
        fftw_destroy_plan(plan_output);
        fftw_free(inputComplex);
        delete [] inputReal;
        delete [] outputReal;
        fftw_cleanup();
    }
            break;
            
        case eDeltaGaussian:
        {
            /* ----- FFT -----
             http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
            https://jperalta.wordpress.com/2006/12/12/using-fftw3/
             */
            qDebug() <<"wiggle eDeltaGaussian";
            //  data
            const int inputSize (calibrationTemp.size());

            const double sigma = mDeltaError/mSettings.mStep;
            const int gaussSize (std::max(inputSize, int(3*sigma)) );
            const int paddingSize (2*gaussSize);

            const int N ( gaussSize + 2*paddingSize);
            const int NComplex (2* (N/2)+1);

            double *inputReal;
            inputReal = new double [N];

            fftw_complex *inputComplex;
            inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);
            

            // on peut utiliser std::copy
            for (int i  = 0; i< paddingSize; i++) {
                inputReal[i] = 0;
            }
            for (int i = 0; i< inputSize; i++) {
                inputReal[i+paddingSize] = calibrationTemp[i];
            }
            for (int i ( inputSize+paddingSize); i< N; i++) {
                inputReal[i] = 0;
            }
           fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);
           fftw_execute(plan_input);

           for (int i = 0; i < NComplex; ++i) {
               const double s = 2. * (double)M_PI * (double)i / (double)N;
               const double factor = exp(-0.5 * pow(s, 2.) * pow(sigma, 2.));

               inputComplex[i][0] *= factor;
               inputComplex[i][1] *= factor;
           }
            

            double *outputReal;
            outputReal = new double [N];


            fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, inputComplex, outputReal, FFTW_ESTIMATE);
            fftw_execute(plan_output);
            
            QVector<double> curve;

            for ( int i = 0; i < N ; i++) {
                curve.append(outputReal[i]);
            }



            mWiggleCalibration->mCurve = equal_areas(curve, mSettings.mStep, 1.);

            mWiggleCalibration->mTmin = minRefCurve - paddingSize*mSettings.mStep + mDeltaAverage;
            mWiggleCalibration->mTmax = minRefCurve + curve.size()*mSettings.mStep+ mDeltaAverage;

            fftw_destroy_plan(plan_input);
            fftw_destroy_plan(plan_output);
            fftw_free(inputComplex);
            delete [] inputReal;
            delete [] outputReal;
            fftw_cleanup();
        }
            break;
            
        default:
            mWiggleCalibration->mCurve = calibrationTemp;
            mWiggleCalibration->mTmin = minRefCurve ;
            mWiggleCalibration->mTmax = maxRefCurve ;
            break;
    }
    
   // mWiggleCalibration->mCurve = equal_areas(mWiggleCalibration->mCurve, settings.mStep, 1.);

    mWiggleCalibration->mRepartition = mWiggleCalibration->mCurve;
    /* We use long double type because
     after several sums, the repartition can be in the double type range
    */
    long double v(0.l);
    long double lastV (0.l);
    long double rep;
    long double lastRep (0.l);
    QVector<double>::iterator itR = mWiggleCalibration->mRepartition.begin();

    for (QVector<double>::iterator itt (itR); itt != mWiggleCalibration->mRepartition.end(); ++itt) {
        lastV = v;
        v = (long double) (*itt );

        rep = lastRep;
        if (v != 0.l && lastV != 0.l)
            rep = lastRep + (long double) (mSettings.mStep) * (lastV + v) / 2.l;

        *itt = rep;

        lastRep = rep;
    }

     mWiggleCalibration->mRepartition = stretch_vector(mWiggleCalibration->mRepartition, 0., 1.);

    /* ------------------------------------------------------------------
     * Restrict the calib and repartition vectors to where data are
     * ------------------------------------------------------------------ */
    if (mWiggleCalibration->mRepartition.last() > 0.) {

        const double threshold (0.00001);
        const int minIdx = int (floor(vector_interpolate_idx_for_value(threshold * lastRep, mWiggleCalibration->mRepartition)));
        const int maxIdx = int (ceil(vector_interpolate_idx_for_value((1. - threshold) * lastRep, mWiggleCalibration->mRepartition)));

        const double tminCal = mWiggleCalibration->mTmin + minIdx * mSettings.mStep;
        const double tmaxCal = mWiggleCalibration->mTmin + maxIdx * mSettings.mStep;

        // Truncate both functions where data live
        mWiggleCalibration->mCurve = mWiggleCalibration->mCurve.mid(minIdx, (maxIdx - minIdx) + 1);
        mWiggleCalibration->mRepartition = mWiggleCalibration->mRepartition.mid(minIdx, (maxIdx - minIdx) + 1);

        // Stretch repartition curve so it goes from 0 to 1
        mWiggleCalibration->mRepartition = stretch_vector(mWiggleCalibration->mRepartition, 0., 1.);

        // Approximation : even if the calib has been truncated, we consider its area to be = 1
        mWiggleCalibration->mCurve = equal_areas(mWiggleCalibration->mCurve, settings.mStep, 1.);

        mWiggleCalibration->mTmin = tminCal;
        mWiggleCalibration->mTmax = tmaxCal;


    }

     //  qDebug()<<"TDate::mWiggleCalibration in project "<<project->mCalibCurves[toFind].mDescription;
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

/**
 * @fn Date::getFormatedCalibToShow
 * @brief The goal is to have a light curve in memory for a faster drawing
 * @return const QMap<double, double>
 */

const QMap<double, double> Date::getFormatedCalibToShow() const
{
    if (mCalibration->mCurve.isEmpty())
        return QMap<double, double>();

    QMap<double, double> calib = getRawCalibMap();


    if (mCalibration->mRepartition.last() > 0.) {
        double tminCal, tmaxCal;
        QVector<double> curve;
        const double threshold  = 0.01 * (*std::max_element(mCalibration->mCurve.begin(), mCalibration->mCurve.end()));

        int minIdx = 0;
        for (auto& v : mCalibration->mCurve) {
            if (v >threshold) break;
            minIdx++;
        }

        int maxIdx = mCalibration->mCurve.size()-1;
        for (auto itv = mCalibration->mCurve.rbegin(); itv!= mCalibration->mCurve.rend(); itv++) {
            if (*itv >threshold) break;
            maxIdx--;
        }

        tminCal = mCalibration->mTmin + minIdx * mCalibration->mStep;
        tmaxCal = mCalibration->mTmin + maxIdx * mCalibration->mStep;

        curve = mCalibration->mCurve.mid(minIdx, (maxIdx - minIdx) + 1);
        curve = equal_areas(curve, mCalibration->mStep, 1.);
        calib = vector_to_map(curve, tminCal, tmaxCal, mCalibration->mStep );

    } else {
        calib = vector_to_map(mCalibration->mCurve, mCalibration->mTmin, mCalibration->mTmax, mCalibration->mStep);
    }

    return DateUtils::convertMapToAppSettingsFormat(calib);
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



const QMap<double, double> Date::getFormatedWiggleCalibToShow() const
{
    if (mWiggleCalibration == nullptr || mWiggleCalibration->mCurve.isEmpty())
        return QMap<double, double>();

    QMap<double, double> calib = getRawWiggleCalibMap();


    double tminCal, tmaxCal;
    QVector<double> curve;
    const double threshold  = 0.01 * (*std::max_element(mWiggleCalibration->mCurve.begin(), mWiggleCalibration->mCurve.end()));

    int minIdx = 0;
    for (auto& v : mWiggleCalibration->mCurve) {
        if (v >threshold) break;
        minIdx++;
    }

    int maxIdx = mWiggleCalibration->mCurve.size()-1;
    for (auto itv = mWiggleCalibration->mCurve.rbegin(); itv!= mWiggleCalibration->mCurve.rend(); itv++) {
        if (*itv >threshold) break;
        maxIdx--;
    }

    tminCal = mWiggleCalibration->mTmin + minIdx * mWiggleCalibration->mStep;
    tmaxCal = mWiggleCalibration->mTmin + maxIdx * mWiggleCalibration->mStep;

    curve = mWiggleCalibration->mCurve.mid(minIdx, (maxIdx - minIdx) + 1);
    curve = equal_areas(curve, mWiggleCalibration->mStep, 1.);
    calib = vector_to_map(curve, tminCal, tmaxCal, mWiggleCalibration->mStep );
   // calib = normalize_map(calib);



    return DateUtils::convertMapToAppSettingsFormat(std::move(calib));
}


QVector<double> Date::getFormatedRepartition() const
{
    if (DateUtils::convertToAppSettingsFormat(mCalibration->mTmin)>DateUtils::convertToAppSettingsFormat(mCalibration->mTmax)) {
       // reverse the QVector and complement, we suppose it's the same step
        QVector<double> repart;
        double lastValue = mCalibration->mRepartition.last();
        QVector<double>::const_iterator iter = mCalibration->mRepartition.cend()-1;
        while (iter != mCalibration->mRepartition.cbegin()-1) {
             repart.append(lastValue-(*iter));
             --iter;
        }
        return repart;

    } else
        return mCalibration->mRepartition;

}


double Date::getFormatedTminRefCurve() const
{
    return qMin(DateUtils::convertToAppSettingsFormat(getTminRefCurve()), DateUtils::convertToAppSettingsFormat(getTmaxRefCurve()));
}

double Date::getFormatedTmaxRefCurve() const
{
    return qMax(DateUtils::convertToAppSettingsFormat(getTminRefCurve()), DateUtils::convertToAppSettingsFormat(getTmaxRefCurve()));
}

double Date::getFormatedTminCalib() const
{
    return qMin(DateUtils::convertToAppSettingsFormat(mCalibration->mTmin), DateUtils::convertToAppSettingsFormat(mCalibration->mTmax));
}

double Date::getFormatedTmaxCalib()const
{
    return qMax(DateUtils::convertToAppSettingsFormat(mCalibration->mTmin), DateUtils::convertToAppSettingsFormat(mCalibration->mTmax));
}

void Date::generateHistos(const QList<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mTi.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    mSigmaTi.generateHistos(chains, fftLen, bandwidth);

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

            const double tminDisplay = qBound(tmin, tLower, tmax);
            const double tmaxDisplay = qBound(tmin, tUpper, tmax);

            const GraphCurve curve = horizontalSection(qMakePair(tminDisplay, tmaxDisplay), "Calibration", color, QBrush(color));

            graph.addCurve(curve);

            // Drawing the wiggle
            if (mDeltaType != eDeltaNone) {

                QMap<double, double> calibWiggle = normalize_map(getMapDataInRange(getRawWiggleCalibMap(), tmin, tmax));
                GraphCurve curveWiggle = densityCurve(calibWiggle, "Wiggle", Qt::red);

                graph.addCurve(curveWiggle);
            }
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


/**
 * @fn Date::generateCalibThumb()
 * @brief Uses the calibration curve already calculated to update the thumbnail.
 * @return
 */
QPixmap Date::generateCalibThumb()
{
    if (mIsValid) {
        //  No need to draw the graph on a large size
        //  These values are arbitary
        const QSize size(1000, 150);

        const double tmin = mSettings.mTmin;
        const double tmax = mSettings.mTmax;


        QMap<double, double> calib = normalize_map(getMapDataInRange(getRawCalibMap(), tmin, tmax));
        qDebug()<<"[Date::generateCalibThumb] mName "<<mCalibration->mName<<mCalibration->mCurve.size()<<calib.size();
        if (calib.isEmpty())
            return QPixmap();

        const QColor color = mPlugin->getColor();
        const GraphCurve curve = densityCurve(calib, "Calibration", mPlugin->getColor(), Qt::SolidLine, color);

        GraphView graph;

        graph.addCurve(curve);
        
        // Drawing the wiggle
        if (mDeltaType != eDeltaNone) {

            const QMap<double, double> calibWiggle = normalize_map(getMapDataInRange(getRawWiggleCalibMap(), tmin, tmax));

            GraphCurve curveWiggle = densityCurve(calibWiggle, "Wiggle", Qt::blue, Qt::SolidLine, QBrush(Qt::NoBrush));

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
        graph.showInfos(false);
        graph.repaint();
        graph.render(&p);
        p.end();

        return thumb;

    } else {
        // If date is invalid, return a null pixmap!
        return QPixmap();
    }

}

double Date::getLikelihoodFromCalib(const double &t) const
{
    return interpolate_value_from_curve(t, mCalibration->mCurve, mCalibration->mTmin, mCalibration->mTmax);

   /* const double tmin (mCalibration->mTmin);
    const double tmax (mCalibration->mTmax);

    // We need at least two points to interpolate
    if (mCalibration->mCurve.size() < 2 || t < tmin || t > tmax)
        return 0.;

    const double prop = (t - tmin) / (tmax - tmin);
    const double idx = prop * (mCalibration->mCurve.size() - 1); // tricky : if (tmax - tmin) = 2000, then calib size is 2001 !
    const int idxUnder = (int)floor(idx);
    const int idxUpper = (int)ceil(idx);//idxUnder + 1;

    if (idxUnder == idxUpper) {
        return mCalibration->mCurve[idxUnder];

    } else if (mCalibration->mCurve[idxUnder] != 0. && mCalibration->mCurve[idxUpper] != 0.) {
        // Important for gate: no interpolation around gates
        return interpolate((double) idx, (double)idxUnder, (double)idxUpper, mCalibration->mCurve[idxUnder], mCalibration->mCurve[idxUpper]);

    } else {
        return 0.;
    }
    */

}

double Date::getLikelihoodFromWiggleCalib(const double &t) const
{
    // test si mWiggleCalibration existe, sinon calcul de la valeur
    if (mWiggleCalibration == nullptr || mWiggleCalibration->mCurve.isEmpty()) {

        if (mDeltaType == eDeltaRange) {
            long double d = mPlugin->getLikelihood(t, mData);
            long double r (mDeltaMin);
            while (r < mDeltaMax) {
                d += mPlugin->getLikelihood(t + r, mData);
                r += mSettings.mStep;
            }
            return d;

        } else if (mDeltaType == eDeltaGaussian) {
            long double d = mPlugin->getLikelihood(t, mData);
            long double r (-5*mDeltaError);
            while (r < (5*mDeltaError)) {
                d += mPlugin->getLikelihood(t + mDeltaAverage + r, mData) * expl((-0.5l) * powl(r, 2.l) / powl(mDeltaError, 2.l)) /sqrt(mDeltaError);
                r += mSettings.mStep;
            }
            return d;

        } else if (mDeltaType == eDeltaFixed) {
            return mPlugin->getLikelihood(t + mDeltaFixed, mData);

        } else {
            return mPlugin->getLikelihood(t, mData);
        }

    } else {
        return interpolate_value_from_curve(t, mWiggleCalibration->mCurve, mWiggleCalibration->mTmin, mWiggleCalibration->mTmax);
    }

/*
    const double tmin (mWiggleCalibration->mTmin);
    const double tmax (mWiggleCalibration->mTmax);

    // We need at least two points to interpolate
    if (mWiggleCalibration->mCurve.size() < 2 || t < tmin || t > tmax)
        return 0.;

    const double prop = (t - tmin) / (tmax - tmin);
    const double idx = prop * (mWiggleCalibration->mCurve.size() - 1); // tricky : if (tmax - tmin) = 2000, then calib size is 2001 !
    const int idxUnder = (int)floor(idx);
    const int idxUpper = (int)ceil(idx);//idxUnder + 1;

    if (idxUnder == idxUpper) {
        return mWiggleCalibration->mCurve[idxUnder];

    } else if (mWiggleCalibration->mCurve[idxUnder] != 0. && mWiggleCalibration->mCurve[idxUpper] != 0.) {
        // Important for gate: no interpolation around gates
        return interpolate((double) idx, (double)idxUnder, (double)idxUpper, mWiggleCalibration->mCurve[idxUnder], mWiggleCalibration->mCurve[idxUpper]);
    } else {
        return 0. ;
    }
    */
}

void Date::updateTheta(Event* event)
{
    (this->*updateti) (event);
}
/**
 * @brief TDate::initDelta Init the wiggle shift
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
            const double tmin = mDeltaAverage - 5*mDeltaError;
            const double tmax = mDeltaAverage + 5*mDeltaError;
            mDelta = Generator::gaussByDoubleExp(mDeltaAverage, mDeltaError, tmin, tmax);
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
    const double lambdai = event->mTheta.mX - mTi.mX;
    
    switch (mDeltaType) {
        case eDeltaNone:
            break;
        
        case eDeltaRange:
            mDelta = Generator::gaussByDoubleExp(lambdai, mSigmaTi.mX, mDeltaMin, mDeltaMax);
            break;
        
        case eDeltaGaussian: {
           // const double lambdai = event->mTheta.mX - mTheta.mX;
            const double w = ( 1/(mSigmaTi.mX * mSigmaTi.mX) ) + ( 1/(mDeltaError * mDeltaError) );
            const double deltaAvg = (lambdai / (mSigmaTi.mX * mSigmaTi.mX) + mDeltaAverage / (mDeltaError * mDeltaError)) / w;
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
    const double lambda = pow(mTi.mX - (event->mTheta.mX - mDelta), 2) / 2.;

   
    const double a (.0001); //precision
    const double b (pow(mSettings.mTmax - mSettings.mTmin, 2.));

    const double V1 = mSigmaTi.mX * mSigmaTi.mX;

    double V2 (0.);
    do {
        const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigmaTi.mSigmaMH);
        V2 = pow(10, logV2);
    } while ((V2<a) || (V2>b));
    
    
    const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
    const double x2 = V1/V2;
    const double rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1; // (V2 / V1) est le jacobien!
    //qDebug() <<"TDate:: updateSigmaJeffreys"<<V2<< rapport;
    
    mSigmaTi.tryUpdate(sqrt(V2), rapport);
}

void Date::updateSigmaShrinkage(Event* event)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double lambda = pow(mTi.mX - (event->mTheta.mX - mDelta), 2.) / 2.;

    const int logVMin = -6;
    const int logVMax = 100;

    const double V1 = mSigmaTi.mX * mSigmaTi.mX;
    const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigmaTi.mSigmaMH);
    const double V2 = pow(10, logV2);

    double rapport  = -1.;
    if (logV2 >= logVMin && logV2 <= logVMax) {
        const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
        const double x2 = pow((event->mS02 + V1) / (event->mS02 + V2), event->mAShrinkage + 1.);
        rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1 ; // (V2 / V1) est le jacobien!

    }
  #ifdef DEBUG
    else {
 //       qDebug()<<"TDate::updateSigma x1 x2 rapport rejet";
    }
 #endif

    mSigmaTi.tryUpdate(sqrt(V2), rapport);
}

void Date::updateSigmaReParam(Event* event)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double lambda = pow(mTi.mX - (event->mTheta.mX - mDelta), 2) / 2.;

    const int VMin (0);

    const double V1 = mSigmaTi.mX * mSigmaTi.mX;
  
    const double r1 = pow( (mTi.mX - (event->mTheta.mX - mDelta))/mSigmaTi.mX, 2);
    const double r2 = Generator::gaussByBoxMuller(r1, mSigmaTi.mSigmaMH);
    const double V2 = pow( (mTi.mX - (event->mTheta.mX - mDelta)), 2)/ r2;

    double rapport (0.);
    if (V2 > VMin ) {
        const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
        const double x2 = pow((event->mS02 + V1) / (event->mS02 + V2), event->mAShrinkage + 1.);
        
        rapport = x1 * sqrt(V1/V2) * x2 * pow(V2 / V1, 2.); // (V2 / V1) est le jacobien!

    }
#ifdef DEBUG
    else {
        qDebug()<<"TDate::updateSigmaReParam x1 x2 rapport rejet";
    }
#endif
    mSigmaTi.tryUpdate(sqrt(V2), rapport);
}

/*
 * void Date::updateWiggle()
{
    mWiggle.mX = mTi.mX + mDelta;
}
*/

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
        date.mTi.mSamplerProposal = plugin->getDataMethod();
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
                    if (deltaType == "fixed" && csvLocale.toDouble(delta1) != 0) {
                        date.mDeltaType = eDeltaFixed;
                        date.mDeltaFixed = csvLocale.toDouble(delta1);
                        
                    } else if (deltaType == "range" && csvLocale.toDouble(delta1) < csvLocale.toDouble(delta2)) {
                        date.mDeltaType = eDeltaRange;
                        date.mDeltaMin = csvLocale.toDouble(delta1);
                        date.mDeltaMax = csvLocale.toDouble(delta2);
                        
                    } else if (deltaType == "gaussian" && csvLocale.toDouble(delta2) > 0) {
                        date.mDeltaType = eDeltaGaussian;
                        date.mDeltaAverage = csvLocale.toDouble(delta1);
                        date.mDeltaError = csvLocale.toDouble(delta2);

                    } else {
                        date.mDeltaType = eDeltaNone;
                        date.mDeltaFixed = 0.;
                        date.mDeltaMin = 0.;
                        date.mDeltaMax = 0;
                        date.mDeltaAverage = 0.;
                        date.mDeltaError = 0.;

                    }
                } else {
                    date.mDeltaType = eDeltaNone;
                }
            }

        }
        date.mIsValid = plugin->isDateValid(date.mData,date.mSettings);
        date.mUUID = QString::fromStdString(Generator::UUID());
    }

    plugin = nullptr;
    return date;
}

QStringList Date::toCSV(const QLocale &csvLocale) const
{
    QStringList csv;

    csv << mPlugin->getName();
    csv << mName;
    csv << mPlugin->toCSV(mData, csvLocale);

    if (mDeltaType == eDeltaNone) {
        csv << "none";

    } else if (mDeltaType == eDeltaFixed) {
        csv << "fixed";
        csv << csvLocale.toString(mDeltaFixed);

    } else if (mDeltaType == eDeltaRange) {
        csv << "range";
        csv << csvLocale.toString(mDeltaMin);
        csv << csvLocale.toString(mDeltaMax);

    } else if (mDeltaType == eDeltaGaussian) {
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

    if (bSet && mPlugin!= 0 && mPlugin->withLikelihoodArg() && mOrigin==eSingleDate) {
         //   if (false) {
        switch (mTi.mSamplerProposal) {
            case MHVariable::eMHSymetric:
                updateti = &Date::fMHSymetricWithArg;
                break;
            
            case MHVariable::eInversion:
                updateti = &Date::fInversionWithArg;
                break;
            
                // only case with acceptation rate, because we use sigmaMH :
            case MHVariable::eMHSymGaussAdapt:
                updateti = &Date::fMHSymGaussAdaptWithArg;
                break;
            
            default:
                break;
            
        }
       // qDebug()<<"TDate::autoSetTiSampler()"<<this->mName<<"with getLikelyhoodArg";
    } else {
        switch (mTi.mSamplerProposal) {
            case MHVariable::eMHSymetric:
                updateti = &Date::fMHSymetric;
                break;
            
            case MHVariable::eInversion:
                updateti = &Date::fInversion;
                break;
            
                // only case with acceptation rate, because we use sigmaMH :
            case MHVariable::eMHSymGaussAdapt:
                updateti = &Date::fMHSymGaussAdapt;
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
void Date::fMHSymetric(Event* event)
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

        const double tiNew = Generator::gaussByBoxMuller(event->mTheta.mX - mDelta, mSigmaTi.mX);
        const double rapport = getLikelihood(tiNew) / getLikelihood(mTi.mX);

        mTi.tryUpdate(tiNew, rapport);


}
/**
 * @brief MH proposal = prior distribution
 * @brief identic as fMHSymetric but use getLikelyhoodArg, when plugin offer it
 *
 */
void Date::fMHSymetricWithArg(Event* event)
{

    const double tiNew = Generator::gaussByBoxMuller(event->mTheta.mX - mDelta, mSigmaTi.mX);

    QPair<long double, long double> argOld, argNew;

    argOld = getLikelihoodArg(mTi.mX);
    argNew = getLikelihoodArg(tiNew);

    const long double rapport=sqrt(argOld.first/argNew.first)*exp(argNew.second-argOld.second);

    mTi.tryUpdate(tiNew, (double)rapport);

}

/**
 *  @brief Calculation of proposal density for time value t
 *
 */
double Date::fProposalDensity(const double t, const double t0)
{
    (void) t0;
    const double tmin (mSettings.mTmin);
    const double tmax (mSettings.mTmax);
    const double level (mMixingLevel);
    double q1 (0.);

    const double tminCalib (mCalibration->mTmin);
    const double tmaxCalib (mCalibration->mTmax);

    // ----q1------Defined only on Calibration range-----
    if (t > tminCalib && t < tmaxCalib){
        //double prop = (t - tmin) / (tmax - tmin);
        const double prop = (t - tminCalib) / (tmaxCalib - tminCalib);
        const double idx = prop * (mCalibration->mRepartition.size() - 1);
        const int idxUnder = (int)floor(idx);

       const double step (mCalibration->mStep);

        q1 = (mCalibration->mRepartition[idxUnder+1] - mCalibration->mRepartition[idxUnder])/step;
        
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
void Date::fInversion(Event* event)
{
    const double u1 = Generator::randomUniform();
    const double level (mMixingLevel);
    double tiNew;
    const double tmin (mSettings.mTmin);
    const double tmax (mSettings.mTmax);

    const double tminCalib = mCalibration->mTmin;

    if (u1<level) { // tiNew always in the study period
        const double idx = vector_interpolate_idx_for_value(u1, mCalibration->mRepartition);
        tiNew = tminCalib + idx *mCalibration->mStep;
    } else {
        // -- gaussian
        const double t0 = mTi.mX;
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

    const double rapport1 = getLikelihood(tiNew) / getLikelihood(mTi.mX);

    const double rapport2 = exp((-0.5 / (mSigmaTi.mX * mSigmaTi.mX)) *
                          (pow(tiNew - (event->mTheta.mX - mDelta), 2) -
                           pow(mTi.mX - (event->mTheta.mX - mDelta), 2))
                          );

    const double rapport3 = fProposalDensity(mTi.mX, tiNew) /
                        fProposalDensity(tiNew, mTi.mX);


    mTi.tryUpdate(tiNew, rapport1 * rapport2 * rapport3);
}

void Date::fInversionWithArg(Event* event)
{
    const double u1 = Generator::randomUniform();
    const double level (mMixingLevel);
    double tiNew;
    const double tmin = mSettings.mTmin;
    const double tmax = mSettings.mTmax;

    const double tminCalib = mCalibration->mTmin;

    if (u1<level) { // tiNew always in the study period
        const double u2 = Generator::randomUniform();
        const double idx = vector_interpolate_idx_for_value(u2, mCalibration->mRepartition);
        tiNew = tminCalib + idx *mCalibration->mStep;


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

    argOld = getLikelihoodArg(mTi.mX);
    argNew = getLikelihoodArg(tiNew);

    const long double logGRapport = argNew.second-argOld.second;
    const long double logHRapport = (-0.5l/powl(mSigmaTi.mX, 2.)) * (  powl(tiNew - (event->mTheta.mX - mDelta), 2.) - powl(mTi.mX - (event->mTheta.mX - mDelta), 2.) ); // modif 2020-09-28

    const long double rapport = sqrt(argOld.first/argNew.first) * exp(logGRapport+logHRapport);

    const long double rapportPD = fProposalDensity(mTi.mX, tiNew) / fProposalDensity(tiNew, mTi.mX);

    mTi.tryUpdate(tiNew, (double)(rapport * rapportPD));

}


/**
 * @brief MH proposal = adaptatif Gaussian random walk, ti is defined on set R (real numbers)
 */
void Date::fMHSymGaussAdapt(Event* event)
{
    const double tiNew = Generator::gaussByBoxMuller(mTi.mX, mTi.mSigmaMH);
    double rapport = getLikelihood(tiNew) / getLikelihood(mTi.mX);
    rapport *= exp((-0.5/(mSigmaTi.mX * mSigmaTi.mX)) * (   pow(tiNew - (event->mTheta.mX - mDelta), 2)
                                                                  - pow(mTi.mX - (event->mTheta.mX - mDelta), 2)
                                                                 ));

    mTi.tryUpdate(tiNew, rapport);
}


/**
 * @brief MH proposal = adaptatif Gaussian random walk, ti is not constraint being on the study period
 *
 * @brief identic as fMHSymGaussAdapt but use getLikelyhoodArg, when plugin offer it
 */
void Date::fMHSymGaussAdaptWithArg(Event* event)
{
    const double tiNew = Generator::gaussByBoxMuller(mTi.mX, mTi.mSigmaMH);

    QPair<long double, long double> argOld, argNew;

    argOld = getLikelihoodArg(mTi.mX);
    argNew = getLikelihoodArg(tiNew);

    const long double logGRapport = argNew.second - argOld.second;
    const long double logHRapport = (-0.5 / (mSigmaTi.mX * mSigmaTi.mX)) * (  pow(tiNew - (event->mTheta.mX - mDelta), 2)
                                                                      - pow(mTi.mX - (event->mTheta.mX - mDelta), 2)
                                                                      );

    const long double rapport = sqrt(argOld.first / argNew.first) * exp(logGRapport+logHRapport);

    mTi.tryUpdate(tiNew, (double) rapport);

}
