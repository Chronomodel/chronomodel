/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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

#include "CalibrationCurve.h"
#include "Generator.h"
#include "PluginAbstract.h"
#include "StateKeys.h"
#include "StdUtilities.h"
#include "PluginManager.h"
#include "PluginUniform.h"

#include "QtUtilities.h"
#include "StudyPeriodSettings.h"

#include "Project.h"
#include "GraphCurve.h"
#include "GraphView.h"

#include "fftw3.h"

#include <QDebug>

Date::Date():
    mTi(),
    mSigmaTi(),
    mWiggle(),
    mDelta(0.0),
    mXi(0.0),
    mId(-1),
    mUUID(""),
    mColor(Qt::blue),
    mData(),
    mOrigin(eSingleDate),

    mPlugin(nullptr),
    mIsValid(false),
    mDeltaType(eDeltaNone),
    mDeltaFixed(0.),
    mDeltaMin(-INFINITY),
    mDeltaMax(+INFINITY),
    mDeltaAverage(0.0),
    mDeltaError(0.0),

    mIsCurrent(false),
    mIsSelected(false),
    mCalibration(nullptr),
    mWiggleCalibration(nullptr),
    mCalibHPD(),
    mSubDates(),
    mMixingLevel(0.99),
    updateti(nullptr),
    mName("No Named Date")
{

    mTi.setName("Ti of Date : " + mName);
    mTi.mSupport = Support::eR;
    mTi.mFormat = DateUtils::eUnknown;
    mTi.mSamplerProposal = SamplerProposal::eDatePrior;

    mSigmaTi.setName("SigmaTi of Date : " + mName);
    mSigmaTi.mSupport = Support::eRp;
    mSigmaTi.mFormat = DateUtils::eNumeric;
    mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

    mWiggle.setName("Wiggle of Date : " + mName);
    mWiggle.mSupport = Support::eR;
    mWiggle.mFormat = DateUtils::eNumeric;

    mId = -1;

    mTminRefCurve = -INFINITY;
    mTmaxRefCurve = INFINITY;

}

Date::Date(const QJsonObject& json):
    mTi(),
    mSigmaTi(),
    mWiggle(),
    mDelta(0.0),
    mXi(0.0),
    mUUID(""),
    mColor(Qt::blue),
    mData(),
    mOrigin(eSingleDate),
    mPlugin(nullptr),
    mIsValid(false),
    mDeltaType(eDeltaNone),
    mDeltaFixed(0.),
    mDeltaMin(-INFINITY),
    mDeltaMax(+INFINITY),
    mDeltaAverage(0.),
    mDeltaError(0.),
    mIsCurrent(false),
    mIsSelected(false),
    mCalibration(nullptr),
    mWiggleCalibration(nullptr),
    mCalibHPD(),
    mSubDates(),
    mMixingLevel(0.99),
    updateti(nullptr),
    mName("No Named Date")
{
    fromJson(json);
    autoSetTiSampler(true); // must be after fromJson()
}

Date::Date(PluginAbstract* plugin):
    mTi(),
    mSigmaTi(),
    mWiggle(),
    mDelta(0.0),
    mXi(0.0),
    mUUID(""),
    mColor(Qt::blue),
    mData(),
    mOrigin(eSingleDate),

    mPlugin(plugin),
    mIsValid(false),
    mDeltaType(eDeltaNone),
    mDeltaFixed(0.),
    mDeltaMin(-INFINITY),
    mDeltaMax(+INFINITY),
    mDeltaAverage(0.),
    mDeltaError(0.),
    mIsCurrent(false),
    mIsSelected(false),
    mCalibration(nullptr),
    mWiggleCalibration(nullptr),
    mCalibHPD(),
    mSubDates(),
    mMixingLevel(0.99),
    updateti(nullptr),
    mName("No Named Date")
{
    mSettings =  getModel_ptr()->mSettings;
}

void Date::init()
{
    mColor = Qt::blue;
    mOrigin = eSingleDate;
    mPlugin = nullptr;

    mTi.setName("Ti of Date : " + mName);
    mTi.mSupport = Support::eR;
    mTi.mFormat = DateUtils::eUnknown;
    mTi.mSamplerProposal = SamplerProposal::eDatePrior;

    mSigmaTi.setName("SigmaTi of Date : " + mName);
    mSigmaTi.mSupport = Support::eRp;
    mSigmaTi.mFormat = DateUtils::eNumeric;
    mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;

    mWiggle.setName("Wiggle of Date : " + mName);
    mWiggle.mSupport = Support::eR;
    mWiggle.mFormat = DateUtils::eUnknown;

    mId = -1;
    mUUID = "";

    mIsValid = true;
    mDelta = 0.0;
    mDeltaType = eDeltaNone;
    mDeltaFixed = 0.0;
    mDeltaMin = 0.0;
    mDeltaMax = 0.0;
    mDeltaAverage = 0.0;
    mDeltaError = 0.0;
    mIsCurrent = false;
    mIsSelected = false;
   // mSubTDates.clear();

    updateti = nullptr;

    mTminRefCurve = -INFINITY;
    mTmaxRefCurve = INFINITY;

    mCalibration = nullptr;
    mWiggleCalibration = nullptr;

    mSettings =  getModel_ptr()->mSettings;

}

/** copy constructor */
Date::Date(const Date& date)
{
    copyFrom(date);
}

/** move constructor */
Date::Date(Date&& other) noexcept
{
    moveFrom(std::move(other));
}

/** Copy move operator */
Date& Date::operator=(Date&& other) noexcept
{
    if (this != &other) { // Vérification d'auto-assignation
        // Transférer les ressources de l'autre objet
        moveFrom(std::move(other));
    }
    return *this;
}

void Date::moveFrom(Date&& other) noexcept
{
    // Transférer les membres de l'autre objet
    mTi = other.mTi;
    mSigmaTi = std::move(other.mSigmaTi);
    mWiggle = std::move(other.mWiggle);
    mDelta = other.mDelta;
    mXi = other.mXi;

    mId = other.mId;
    mUUID = other.mUUID;
    mColor = other.mColor;
    mData = other.mData;
    mOrigin = other.mOrigin;

    mPlugin = other.mPlugin;
    mIsValid = other.mIsValid;
    mDeltaType = other.mDeltaType;
    mDeltaFixed = other.mDeltaFixed;
    mDeltaMin = other.mDeltaMin;
    mDeltaMax = other.mDeltaMax;
    mDeltaAverage = other.mDeltaAverage;
    mDeltaError = other.mDeltaError;
    mIsCurrent = other.mIsCurrent;
    mIsSelected = other.mIsSelected;

    mCalibration = other.mCalibration;
    mWiggleCalibration = other.mWiggleCalibration;
    mCalibHPD = other.mCalibHPD;
    mSubDates = other.mSubDates;
    mMixingLevel = other.mMixingLevel;
    updateti = other.updateti;
    mName = other.mName;

    mSettings = other.mSettings;

    // Réinitialiser l'autre objet

    other.mPlugin = nullptr;
    other.mCalibration = nullptr;
    other.mWiggleCalibration = nullptr;
    other.mCalibHPD.clear();
    other.mName = "No Named Date";
    other.updateti = nullptr;
    other.mId = -1;

}

/** Copy assignment operator */
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
    mXi = date.mXi;

    mId = date.mId;
    mUUID = date.mUUID;
    mColor = date.mColor;
    mData = date.mData;
    mOrigin = date.mOrigin;

    mPlugin = date.mPlugin;
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
    mName = date.mName;

    mTminRefCurve = date.mTminRefCurve;
    mTmaxRefCurve = date.mTmaxRefCurve;

    mSettings = date.mSettings;
}

Date::~Date()
{
    mPlugin = nullptr;
    mCalibration = nullptr;
    mWiggleCalibration = nullptr;

    mTi.clear();
    mSigmaTi.clear();
    updateti = nullptr;
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
    mId = json.value(STATE_ID).toInt();
    setName(json.value(STATE_NAME).toString());
    mColor = QColor(json.value(STATE_COLOR_RED).toInt(),
                    json.value(STATE_COLOR_GREEN).toInt(),
                    json.value(STATE_COLOR_BLUE).toInt());

    mUUID = json.value(STATE_DATE_UUID).toString().toStdString();

    if (mUUID.empty())
        mUUID = Generator::UUID();

    // Copy plugin specific values for this data :
    mData = json.value(STATE_DATE_DATA).toObject();
    mOrigin = (OriginType)json.value(STATE_DATE_ORIGIN).toInt();

    QString pluginId = json.value(STATE_DATE_PLUGIN_ID).toString();
    mPlugin = PluginManager::getPluginFromId(pluginId);

    mIsValid = json.value(STATE_DATE_VALID).toBool();

    mDeltaType = (DeltaType)json.value(STATE_DATE_DELTA_TYPE).toInt();
    mDeltaFixed = json.value(STATE_DATE_DELTA_FIXED).toDouble();
    mDeltaMin = json.value(STATE_DATE_DELTA_MIN).toDouble();
    mDeltaMax = json.value(STATE_DATE_DELTA_MAX).toDouble();
    mDeltaAverage = json.value(STATE_DATE_DELTA_AVERAGE).toDouble();
    mDeltaError = json.value(STATE_DATE_DELTA_ERROR).toDouble();

    mIsCurrent = false;
    mIsSelected = false;

    auto project = getProject_ptr();// MainWindow::getInstance()->getProject().get();
    mSettings = StudyPeriodSettings::fromJson(project->mState.value(STATE_SETTINGS).toObject()); // StudyPeriodSettings::fromJson is static
    mSubDates = json.value(STATE_DATE_SUB_DATES).toArray();

    mMixingLevel = project->mState.value(STATE_MCMC).toObject().value(STATE_MCMC_MIXING).toDouble();

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

            for (auto&& d : mSubDates ) {

                const bool hasWiggle (d.toObject().value(STATE_DATE_DELTA_TYPE).toInt() != eDeltaNone);
                std::string toFind;
                if (hasWiggle) {
                    toFind = "WID::" + d.toObject().value(STATE_DATE_UUID).toString().toStdString();

                } else {
                     toFind = d.toObject().value(STATE_DATE_UUID).toString().toStdString();
                }

                auto it = project->mCalibCurves.find(toFind);

                if ( it != project->mCalibCurves.end()) {
                    CalibrationCurve* d_mCalibration = & it->second;
                    tmin = std::min(d_mCalibration->mTmin, tmin);
                    tmax = std::max(d_mCalibration->mTmax, tmax);

                } else {
                    /* When reading the .chr file without the presence of the .cal file, there is no calibration for the subdates
                     *  and the display of the curves in CalibrationView crashes.
                     */
                    Date sd;
                    sd.fromJson(d.toObject());
                    sd.calibrate(project);

                    if (sd.mCalibration) {
                        tmin = std::min(sd.mCalibration->mTmin, tmin);
                        tmax = std::max(sd.mCalibration->mTmax, tmax);

                    }
                }

            }
            mTminRefCurve = tmin;
            mTmaxRefCurve = tmax;

        }
    }

    mTi.setName("Ti of Date : "+ mName);
    mTi.mSupport = Support::eR;
    mTi.mFormat = DateUtils::eUnknown;
    mTi.mSamplerProposal = (SamplerProposal)json.value(STATE_DATE_SAMPLER).toInt();

    if ((SamplerProposal)json.value(STATE_DATE_SAMPLER).toInt() == SamplerProposal::eFixe)
        mSigmaTi.mSamplerProposal = SamplerProposal::eFixe;
    else
        mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
    mSigmaTi.setName("Sigma of Date : "+ mName);
    mSigmaTi.mSupport = Support::eRp;
    mSigmaTi.mFormat = DateUtils::eNumeric;

    mWiggle.setName("Wiggle of Date : "+ mName);
    mWiggle.mSupport = Support::eR;
    mWiggle.mFormat = DateUtils::eUnknown;


    std::map<std::string, CalibrationCurve>::iterator it = project->mCalibCurves.find (mUUID);
    if ( it != project->mCalibCurves.end()) {
        mCalibration = & it->second;

     } else {
        mCalibration = nullptr;
     }

    std::string toFind = "WID::" + mUUID;
    it = project->mCalibCurves.find (toFind);
    if ( it != project->mCalibCurves.end()) {
        mWiggleCalibration = & it->second;

    } else {
        mWiggleCalibration = nullptr;
    }


}

QJsonObject Date::toJson() const
{
    QJsonObject json;
    json[STATE_ID] = mId;
    json[STATE_DATE_UUID] = QString::fromStdString(mUUID);
    json[STATE_NAME] = getQStringName();
    json[STATE_DATE_DATA] = mData;
    json[STATE_DATE_ORIGIN] = mOrigin;
    if (mPlugin)
        json[STATE_DATE_PLUGIN_ID] = mPlugin->getId();
    else
        json[STATE_DATE_PLUGIN_ID] = -1;

    json[STATE_DATE_SAMPLER] = static_cast<int>(mTi.mSamplerProposal);
    json[STATE_DATE_VALID] = mIsValid;

    json[STATE_DATE_DELTA_TYPE] = mDeltaType;
    json[STATE_DATE_DELTA_FIXED] = mDeltaFixed;
    json[STATE_DATE_DELTA_MIN] = mDeltaMin;
    json[STATE_DATE_DELTA_MAX] = mDeltaMax;
    json[STATE_DATE_DELTA_AVERAGE] = mDeltaAverage;
    json[STATE_DATE_DELTA_ERROR] = mDeltaError;

    json[STATE_COLOR_RED] = mColor.red();
    json[STATE_COLOR_GREEN] = mColor.green();
    json[STATE_COLOR_BLUE] = mColor.blue();

    json[STATE_DATE_SUB_DATES] = mSubDates;
    return json;
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
            if (mCalibration == nullptr || mCalibration->mVector.empty()) {
                return mPlugin->getLikelihoodCombine(t, mSubDates, mSettings.mStep);

            } else {
                return getLikelihoodFromCalib(t);
            }
            
        } else {
            
            return 0.0L;
        }
    
    } else
         return 0.0L;

}

QPair<long double, long double> Date::getLikelihoodArg(const double t) const
{
    if (mPlugin)
        if (mOrigin == eSingleDate) {
            return mPlugin->getLikelihoodArg(t, mData);
            
        }  else if (mOrigin == eCombination) {
                     if (mCalibration->mVector.empty()) {
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


QString Date::getWiggleDesc() const
{
    QString res;
      
    switch (mDeltaType) {
        case Date::eDeltaFixed:
            res = "Fixed wiggle: " + QString::number(mDeltaFixed);
        break;
        case Date::eDeltaRange:
            res = QStringLiteral("Wiggle ~ U(%1, %2)")
                      .arg(mDeltaMin)
                      .arg(mDeltaMax);

        break;
        case Date::eDeltaGaussian:
            res = "Wiggle ~ N(μ=" + QString::number(mDeltaAverage) +
                  ", σ=" + QString::number(mDeltaError) + ")";


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




void Date::clear()
{
    mTi.clear();
    mSigmaTi.clear();
    mWiggle.clear();
}

void Date::shrink_to_fit() noexcept
{
    mTi.shrink_to_fit();
    mSigmaTi.shrink_to_fit();
    mWiggle.shrink_to_fit();
}

/**
 * @brief TDate::calibrate
 * Function that calculates the calibrated density and updates the wiggle density if necessary
 * @param settings
 * @param project
 * @param truncate Restrict the calib and repartition vectors to where data are
 */

void Date::calibrate(const StudyPeriodSettings &period_settings, std::shared_ptr<Project> project, bool truncate)
{
    // add the calibration
    std::map<std::string, CalibrationCurve>::iterator it = project->mCalibCurves.find (mUUID);

    if ( it == project->mCalibCurves.end()) {
        qDebug()<<"[Date::calibrate] Curve to create mUUID: "<< mUUID ;
        project->mCalibCurves.insert_or_assign(mUUID, CalibrationCurve());

    } else {
        const CalibrationCurve& d_mCalibration = it->second;
        if ( d_mCalibration.mDescription == getDesc().toStdString() ) {
            // Controls whether the curve has already been calculated using the description
            calibrateWiggle(period_settings, project);

            return;
        }
    }

  // Check if the ref curve is in the plugin list

    double refMinStep = INFINITY;
    if (mOrigin == eSingleDate) {
        const QStringList refsNames = mPlugin->getRefsNames();
        const QString dateRefName = mPlugin->getDateRefCurveName(this);

        if (!dateRefName.isEmpty() && !refsNames.contains(dateRefName) )
            return;
        refMinStep = mPlugin->getMinStepRefsCurve(mData);

    } else if (mOrigin == eCombination) {
        if ( it != project->mCalibCurves.end()) {
            const CalibrationCurve& d_mCalibration = it->second;
            refMinStep = std::min(refMinStep, d_mCalibration.mStep);

        } else {
            for (auto&& sd : mSubDates) {
                const std::string toFind = sd.toObject().value(STATE_DATE_UUID).toString().toStdString();
                auto it = project->mCalibCurves.find(toFind);

                if ( it != project->mCalibCurves.end()) {
                    const CalibrationCurve& d_mCalibration = it->second;
                    refMinStep = std::min(refMinStep, d_mCalibration.mStep);
                }

            }
        }

    }


    // Update of the new calibration curve

    mCalibration = &project->mCalibCurves[mUUID];
    mCalibration -> mDescription = getDesc().toStdString();
    if(period_settings.mStepForced)
        refMinStep = period_settings.mStep;

    mCalibration->mStep = refMinStep;
    mCalibration->mPluginId = mPlugin->getId().toStdString();

    mCalibration->setName(mName);

    mCalibHPD.clear();
    mCalibration->mMap.clear();
    mCalibration->mVector.clear();

    // Impossible to calibrate because the plugin could not return any calib curve definition period.
    // This may be due to invalid ref curve files or to polynomial equations with only imaginary solutions (See Gauss Plugin...)
    if (mTmaxRefCurve <= mTminRefCurve)
        return;

    double tminCal;
    double tmaxCal;

    /* --------------------------------------------------
     *  Calibrate on the whole calibration period (= ref curve definition domain)
     * -------------------------------------------------- */

    if (!period_settings.mStepForced) {
        int nb_step_frac = 0;

        while ( mCalibration->mVector.empty() && std::count_if (mCalibration->mVector.begin(), mCalibration->mVector.end(), [](double v){return std::isnormal(v);}) < 22 && nb_step_frac < 50) {
            ++nb_step_frac;
            mCalibration->mStep = refMinStep / (double)nb_step_frac;
#ifdef DEBUG
            long int ld = static_cast<long int>( (mTmaxRefCurve - mTminRefCurve) / mCalibration->mStep);
            if (ld < std::numeric_limits<long int>::min() || ld > std::numeric_limits<long int>::max()) {
                qDebug()<< "La valeur est hors de portée pour un long int.";
            }
#endif
            long int nbStep = std::lround((mTmaxRefCurve - mTminRefCurve) / mCalibration->mStep);

            long double long_step = (long double)(mTmaxRefCurve - mTminRefCurve) / nbStep; // Very usefull for loop precision

            // Correction de mStep apres arrondi
            mCalibration->mStep = long_step;

            std::vector<double> calibrationTemp;
            std::vector<double> repartitionTemp;

            /* We use long double type because
             * after several sums, the repartition can be in the double type range
             */

            long double lastV = 0;
            long double rep = 0.;

            for (auto i = 0; i <= nbStep; ++i) {
                long double t = mTminRefCurve + i * long_step;
                long double v =  getLikelihood(t);

                calibrationTemp.push_back(v);

                if (v != 0.l && lastV != 0.l)
                    rep += (lastV + v); //step is constant

                repartitionTemp.push_back(rep);

                lastV = v;
            }

            /*
             *  Restrict the calibration and distribution vectors to the locations of the data.
             */
            if (repartitionTemp.empty()) // if error in CSV importation
                return;

            if (*repartitionTemp.crbegin() > 0.) {
                if (truncate && repartitionTemp.size() > 10) {
                    long double threshold = threshold_limit;
                    double th_min= (threshold * rep);
                    double th_max = ((1. - threshold) * rep);
                    /*int minIdx = floor(vector_interpolate_idx_for_value(th_min, repartitionTemp));
                    int maxIdx = ceil(vector_interpolate_idx_for_value(th_max, repartitionTemp));*/

                    int minIdx = floor(interpolate_index(th_min, repartitionTemp));
                    int maxIdx = ceil(interpolate_index(th_max, repartitionTemp));


                    tminCal = mTminRefCurve + minIdx * long_step;
                    tmaxCal = mTminRefCurve + maxIdx * long_step;

                    // Truncate both functions where data live
                    mCalibration->mVector.assign(calibrationTemp.begin()+minIdx, calibrationTemp.begin()+(maxIdx + 1));
                    mCalibration->mRepartition.assign(repartitionTemp.begin()+minIdx, repartitionTemp.begin()+(maxIdx + 1));

                    /* NOTE ABOUT THIS APPROXIMATION :
                     * By truncating the calib and repartition, the calib density's area is not 1 anymore!
                     * It is now 1 - 2*threshold = 0,99998... We consider it to be 1 anyway!
                     * By doing this, calib and repartition are stored on a restricted number of data
                     * instead of storing them on the whole reference curve's period (as done for calibrationTemp & repartitionTemp above).
                     */
                } else {
                    tminCal = mTminRefCurve;
                    tmaxCal = mTmaxRefCurve;
                    mCalibration->mVector = calibrationTemp;

                    mCalibration->mRepartition = repartitionTemp;
                }
                // Stretch repartition curve so it goes from 0 to 1
                mCalibration->mRepartition = stretch_vector(mCalibration->mRepartition, (double)0., (double)1.);

                // Approximation : even if the calib has been truncated, we consider its area to be = 1
                mCalibration->mVector = equal_areas(mCalibration->mVector, mCalibration->mStep, 1.);

                mCalibration->mTmin = tminCal;
                mCalibration->mTmax = tmaxCal;
                mCalibration->mMap = vector_to_map(mCalibration->mVector, mCalibration->mTmin, mCalibration->mTmax, mCalibration->mStep);

            }
            /*
             *  Measurement is very far from Ref curve on the whole ref curve preriod!
             *  => Calib values are very small, considered as being 0 even using "double" !
             *  => lastRepVal = 0, and impossible to truncate using it....
             *  => So,
             */

            else  {
                mCalibration->mTmin = mTminRefCurve;
                mCalibration->mTmax = mTmaxRefCurve;
            }

        }
    } else {
        std::vector<double> calibrationTemp;
        std::vector<double> repartitionTemp;

        long double v0 = getLikelihood(mTminRefCurve);
        calibrationTemp.push_back(v0);
        repartitionTemp.push_back(v0);
        long double lastRepVal = v0;

        /* We use long double type because
         * after several sums, the repartition can be in the double type range
         */

        long double lastV = v0;
        long double rep;
        long int nbStep = floor((long double)(mTmaxRefCurve - mTminRefCurve) / mCalibration->mStep);

        const long double long_step = (long double)(mTmaxRefCurve - mTminRefCurve) / nbStep; // Very usefull for loop precision

        for (auto i = 1; i <= nbStep; ++i) {
            const long double t = mTminRefCurve + i * long_step;
            const long double v = getLikelihood(t);

            calibrationTemp.push_back(double(v));
            rep = lastRepVal;
            if (v != 0.l && lastV != 0.l)
                rep = lastRepVal + long_step * (lastV + v) / 2.l;

            repartitionTemp.push_back(double (rep));
            lastRepVal = rep;
            lastV = v;
        }
        mCalibration->mStep = long_step;
        tmaxCal = mTminRefCurve + nbStep * long_step;
        /*
         *  Restrict the calibration and distribution vectors to the locations of the data.
         */

        if (*repartitionTemp.crbegin() > 0.) {
            if (truncate && repartitionTemp.size() > 10) {
                const double threshold = threshold_limit;

                /*const int minIdx = int (floor(vector_interpolate_idx_for_value(double(threshold * lastRepVal), repartitionTemp)));
                const int maxIdx = int (ceil(vector_interpolate_idx_for_value(double ((1.0 - threshold) * lastRepVal), repartitionTemp)));*/

                const int minIdx = int (floor(interpolate_index(double(threshold * lastRepVal), repartitionTemp)));
                const int maxIdx = int (ceil(interpolate_index(double ((1.0 - threshold) * lastRepVal), repartitionTemp)));


                tminCal = mTminRefCurve + minIdx * long_step;
                tmaxCal = mTminRefCurve + maxIdx * long_step;

                // Truncate both functions where data live
                mCalibration->mVector.assign(calibrationTemp.begin()+minIdx, calibrationTemp.begin()+(maxIdx + 1));
                mCalibration->mRepartition.assign(repartitionTemp.begin()+ minIdx, repartitionTemp.begin()+(maxIdx + 1));

                /* NOTE ABOUT THIS APPROXIMATION :
                 * By truncating the calib and repartition, the calib density's area is not 1 anymore!
                 * It is now 1 - 2*threshold = 0,99998... We consider it to be 1 anyway!
                 * By doing this, calib and repartition are stored on a restricted number of data
                 * instead of storing them on the whole reference curve's period (as done for calibrationTemp & repartitionTemp above).
                 */
            } else {
                tminCal = mTminRefCurve;
                tmaxCal = mTminRefCurve + nbStep * mCalibration->mStep;
                mCalibration->mVector = calibrationTemp;
                mCalibration->mRepartition = repartitionTemp;
            }
            // Stretch repartition curve so it goes from 0 to 1
            mCalibration->mRepartition = stretch_vector(mCalibration->mRepartition, (double)0., (double)1.);

            // Approximation : even if the calib has been truncated, we consider its area to be = 1
            mCalibration->mVector = equal_areas(mCalibration->mVector, mCalibration->mStep, 1.);

            mCalibration->mTmin = tminCal;
            mCalibration->mTmax = tmaxCal;
            mCalibration->mMap = vector_to_map(mCalibration->mVector, mCalibration->mTmin, mCalibration->mTmax, mCalibration->mStep);

        }
        /* ------------------------------------------------------------------
         *  Measurement is very far from Ref curve on the whole ref curve preriod!
         *  => Calib values are very small, considered as being 0 even using "double" !
         *  => lastRepVal = 0, and impossible to truncate using it....
         *  => So,
         * ------------------------------------------------------------------ */

        else  {
            mCalibration->mTmin = mTminRefCurve;
            mCalibration->mTmax = tmaxCal;
        }
    }
    // If the calibration curve changes, the wiggle curve must be recalculated.
    if (mWiggleCalibration != nullptr)  {
        const std::string toFind ("WID::" + mUUID);
        std::map<std::string, CalibrationCurve>::const_iterator it = project->mCalibCurves.find(toFind);
        project->mCalibCurves.erase(it);
    }

    /* WIGGLE CALIBRATION CURVE */
    if (mDeltaType != eDeltaNone) {
        calibrateWiggle(project);
    }

}


/**
 * @brief TDate::calibrateWiggle Function that calculates the wiggle density according to the defined wiggle type
 * @param settings
 * @param project
 */
void Date::calibrateWiggle(const StudyPeriodSettings &settings, std::shared_ptr<Project> project)
{
    (void) settings;
    // Check if the ref curve is in the plugin list
    if (mDeltaType == Date::eDeltaNone) {
        mWiggleCalibration = nullptr;
        return;
    }
    // add the calibration

    // We need to keep the calibration curve and then the whole wiggle on the whole support,
    // to allow a more accurate combination when the densities are far away.
    const std::string toFind ("WID::" + mUUID);
    auto it = project->mCalibCurves.find(toFind);

    if ( it == project->mCalibCurves.end()) {
        qDebug() << "[Date::calibrateWiggle] Curve to create Wiggle: "<< toFind ;
        qDebug() << "[Date::calibrateWiggle]create Wiggle descript: " << getWiggleDesc() ;
        project->mCalibCurves.insert_or_assign(toFind, CalibrationCurve());
        
        
    } else if ( it->second.mDescription == getWiggleDesc().toStdString() ) {
        // Controls whether the curve has already been calculated using the description
        qDebug() << "[Date::calibrateWiggle] The curve already exists Wiggle:" << toFind ;
        qDebug() << "[Date::calibrateWiggle] Wiggle descript: " << getWiggleDesc() ;
        return;
        
    }

    mWiggleCalibration = & (project->mCalibCurves[toFind]);

    mWiggleCalibration->mDescription = getWiggleDesc().toStdString();
    mWiggleCalibration->mPluginId = mPlugin->getId().toStdString();
   // mWiggleCalibration->mPlugin = mPlugin;
    mWiggleCalibration->setName(mName);

    if (mDeltaType == eDeltaFixed) {
        mWiggleCalibration->mVector = mCalibration->mVector; // mVector and repartition are the same as mCalibration
        mWiggleCalibration->mRepartition = mCalibration->mRepartition;
        mWiggleCalibration->mTmin = mCalibration->mTmin + mDeltaFixed;
        mWiggleCalibration->mTmax = mCalibration->mTmax + mDeltaFixed;
        mWiggleCalibration->mStep = mCalibration->mStep;
        mWiggleCalibration->mMap = vector_to_map(mWiggleCalibration->mVector, mWiggleCalibration->mTmin, mWiggleCalibration->mTmax, mWiggleCalibration->mStep);

        return;
    }

    std::vector<double> calibrationTemp;
    const QPair<double, double> tminTmax = mPlugin->getTminTmaxRefsCurve(mData);
    const double minRefCurve = tminTmax.first;
    const double maxRefCurve = tminTmax.second;

     // Update of the new calibration curve, on the whole reference curve


    const double nbRefPts = 1. + round((maxRefCurve - minRefCurve) / double(mCalibration->mStep));
    calibrationTemp.push_back(getLikelihood(minRefCurve));

    /* We use long double type because
     * after several sums, the repartition can be in the double type range
     */
    for (int i = 1; i <= nbRefPts; ++i) {
        const double t = minRefCurve + double (i) * mCalibration->mStep;
        calibrationTemp.push_back(double(getLikelihood(t)));
    }

    mWiggleCalibration->mStep = mCalibration->mStep;



    /* --------------------------------------------------
     *  Calibrate on the whole calibration period (= ref curve definition domain)
     * -------------------------------------------------- */
    std::vector<double> curve;
    switch (mDeltaType) {
        case eDeltaFixed: //obsolete
            //mWiggleCalibration = mCalibration;
            mWiggleCalibration->mVector = mCalibration->mVector; // mVector and repartition are the same as mCalibration
            mWiggleCalibration->mRepartition = mCalibration->mRepartition;
            mWiggleCalibration->mTmin = mCalibration->mTmin + mDeltaFixed;
            mWiggleCalibration->mTmax = mCalibration->mTmax + mDeltaFixed;
            mWiggleCalibration->mStep = mCalibration->mStep;
            mWiggleCalibration->mMap = vector_to_map(mWiggleCalibration->mVector, mWiggleCalibration->mTmin, mWiggleCalibration->mTmax, mWiggleCalibration->mStep);

            return;
            break;
                
#pragma mark wiggle Gate
        case eDeltaRange:
        {
            const int inputSizeOld = (int)calibrationTemp.size();
            const double origStep = mWiggleCalibration->mStep;
            const double deltaSpan = mDeltaMax - mDeltaMin;// + 1.0;

            // 1. Recherche du rapport irréductible p/q = deltaSpan / origStep
            const double R = deltaSpan / origStep;
            int q = 1;
            while (std::abs(std::round(R * q) - R * q) > 1e-5 && q < 1000) {
                q++;
            }
            int p = static_cast<int>(std::round(R * q));

            // 2. Facteur de sur-échantillonnage S pour avoir une bonne résolution
            // (Par exemple : au moins 5 sous-points par origStep)
            const int targetSubdivisions = 5;
            const int S = std::max(1, static_cast<int>(std::ceil((double)targetSubdivisions / q)));

            // M = sous-pas par origStep, K = sous-pas dans la porte
            const int M = S * q;
            const int K = S * p;

            // Le pas cible est exactement sous-multiple des deux grandeurs
            const double targetStep = origStep / (double)M;

            // 3. Rééchantillonnage sans erreur de phase
            const double origDuration = (inputSizeOld - 1) * origStep;
            const int N_in = static_cast<int>(std::round(origDuration / targetStep)) + 1;

            std::vector<double> f_resampled(N_in, 0.0);

            for (int j = 0; j < N_in; ++j) {
                // Division entière exacte pour retrouver l'indice d'origine
                int idxLow = j / M;

                if (idxLow >= inputSizeOld - 1) {
                    f_resampled[j] = calibrationTemp.back();
                } else {
                    f_resampled[j] = calibrationTemp[idxLow];
                }
            }

            // 4. Convolution exacte en O(N) avec porte de taille K
            const int N_out = N_in + K - 1;
            std::vector<double> outputReal(N_out, 0.0);

            double runningSum = 0.0;

            for (int j = 0; j < N_out; ++j) {
                if (j < N_in) {
                    runningSum += f_resampled[j];
                }
                if (j >= K) {
                    runningSum -= f_resampled[j - K];
                }
                outputReal[j] = std::max(0.0, runningSum) * targetStep;
            }

            // 5. Mise à jour des bornes et sauvegarde
            curve.clear();
            curve.insert(curve.end(), outputReal.begin(), outputReal.end());

            mWiggleCalibration->mStep = targetStep;
            mWiggleCalibration->mVector = equal_areas(curve, targetStep, 1.0);

            const double T_start = minRefCurve + mDeltaMin;
            mWiggleCalibration->mTmin = T_start;
            mWiggleCalibration->mTmax = T_start + (curve.size() - 1) * targetStep;
        }
        break;
#pragma mark wiggle Gaussian
        case eDeltaGaussian:
        {
            /* ----- FFT -----
             http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
            https://jperalta.wordpress.com/2006/12/12/using-fftw3/
             */
            qDebug() <<"[Date::calibrateWiggle] wiggle eDeltaGaussian";
            //  data
            const int inputSize = (int)calibrationTemp.size();

            const double sigma = mDeltaError / mWiggleCalibration->mStep;
            const int gaussSize (std::max(inputSize, int(3*sigma)) );
            const int paddingSize (2*gaussSize);

            const int N ( gaussSize + 2*paddingSize);
            const int NComplex (2* (N/2)+1);

            double *inputReal;
            inputReal = new double [N];

            fftw_complex *inputComplex;
            inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);
            

            // we could use std::copy
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


            for ( int i = 0; i < N ; i++) {
                curve.push_back(outputReal[i]);
            }



            mWiggleCalibration->mVector = equal_areas(curve, mWiggleCalibration->mStep, 1.);

            mWiggleCalibration->mTmin = minRefCurve - paddingSize* mWiggleCalibration->mStep + mDeltaAverage;
            mWiggleCalibration->mTmax = minRefCurve + curve.size()* mWiggleCalibration->mStep + mDeltaAverage;

            fftw_destroy_plan(plan_input);
            fftw_destroy_plan(plan_output);
            fftw_free(inputComplex);
            delete [] inputReal;
            delete [] outputReal;
            fftw_cleanup();
        }
            break;
            
        default:
            //mWiggleCalibration = mCalibration;
            mWiggleCalibration->mVector = mCalibration->mVector; // mVector and repartition are the same as mCalibration
            mWiggleCalibration->mRepartition = mCalibration->mRepartition;
            mWiggleCalibration->mTmin = mCalibration->mTmin + mDeltaFixed;
            mWiggleCalibration->mTmax = mCalibration->mTmax + mDeltaFixed;
            mWiggleCalibration->mStep = mCalibration->mStep;
            mWiggleCalibration->mMap = vector_to_map(mWiggleCalibration->mVector, mWiggleCalibration->mTmin, mWiggleCalibration->mTmax, mWiggleCalibration->mStep);

            return;
            break;
    }

    mWiggleCalibration->mRepartition = mWiggleCalibration->mVector;
    /* We use long double type because
       after several sums, the repartition can be in the double type range
     */
    long double v(0.l);
    long double lastV (0.l);
    long double rep;
    long double lastRep (0.l);
    std::vector<double>::iterator itR = mWiggleCalibration->mRepartition.begin();

    for (std::vector<double>::iterator itt (itR); itt != mWiggleCalibration->mRepartition.end(); ++itt) {
        lastV = v;
        v = (long double) (*itt );

        rep = lastRep;

        if (v != 0.l && lastV != 0.l)
            rep = lastRep + (long double) ( mWiggleCalibration->mStep) * (lastV + v) / 2.l;

        *itt = rep;

        lastRep = rep;
    }


    /* ------------------------------------------------------------------
     * Restrict the calib and repartition vectors to where data are
     * ------------------------------------------------------------------ */
    if (*mWiggleCalibration->mRepartition.crbegin() > 0.0) {

        const double threshold = threshold_limit;// 0.00001;
        const int n = static_cast<int>(mWiggleCalibration->mRepartition.size());
        const int minIdx = std::clamp(int (floor(interpolate_index(double(threshold * lastRep), mWiggleCalibration->mRepartition))) - 1, 0, n - 1);
        const int maxIdx = std::clamp(int (ceil(interpolate_index(double((1.0 - threshold) * lastRep), mWiggleCalibration->mRepartition))) + 1, minIdx, n - 1);;

        if (maxIdx > minIdx) {
            const double tminCal = mWiggleCalibration->mTmin + minIdx * mWiggleCalibration->mStep;
            const double tmaxCal = mWiggleCalibration->mTmin + maxIdx * mWiggleCalibration->mStep;

            // Truncate both functions where data live

            mWiggleCalibration->mVector.assign(mWiggleCalibration->mVector.begin() + minIdx, mWiggleCalibration->mVector.begin()+ maxIdx);
            mWiggleCalibration->mRepartition.assign(mWiggleCalibration->mRepartition.begin()+minIdx, mWiggleCalibration->mRepartition.begin()+ maxIdx);

            // Stretch repartition curve so it goes from 0 to 1
            mWiggleCalibration->mRepartition = stretch_vector(mWiggleCalibration->mRepartition, 0.0, 1.0);

            // Approximation : even if the calib has been truncated, we consider its area to be = 1
            mWiggleCalibration->mVector = equal_areas(mWiggleCalibration->mVector, mWiggleCalibration->mStep, 1.0);

            mWiggleCalibration->mTmin = tminCal;
            mWiggleCalibration->mTmax = tmaxCal;

        } else {
            std::cout << "[" << __func__ << "] ‼️ Error in restricting the calibration and repartition vectors" << std::endl;

        }

    } else {
        mWiggleCalibration->mRepartition = stretch_vector(mWiggleCalibration->mRepartition, 0.0, 1.0);
    }

    mWiggleCalibration->mMap = vector_to_map(mWiggleCalibration->mVector, mWiggleCalibration->mTmin, mWiggleCalibration->mTmax, mWiggleCalibration->mStep);

}


const std::map<double, double> &Date::getRawCalibMap() const
{
    return mCalibration->mMap;
}

const std::map<double, double> Date::getFormatedCalibMap() const
{
    if (mCalibration->mVector.empty())
        return std::map<double, double>();

    return DateUtils::convertMapToAppSettingsFormat(mCalibration->mMap);

}

/**
 * @fn Date::getFormatedCalibToShow
 * @brief The goal is to have a light curve in memory for a faster drawing
 * @return const QMap<double, double>
 */

const std::map<double, double> Date::getFormatedCalibToShow() const
{
    if (mCalibration->mVector.empty())
        return std::map<double, double>();

    std::map<double, double> calib = getRawCalibMap();

    if (*mCalibration->mRepartition.crbegin() > 0.0) {
        double tminCal, tmaxCal;
        std::vector<double> curve;
        const double threshold  = 0.01 * (*std::max_element(mCalibration->mVector.begin(), mCalibration->mVector.end())); // ici

        int minIdx = 0;
        for (auto& v : mCalibration->mVector) {
            if (v >=threshold) break;
            minIdx++;
        }

        if (minIdx == (int) mCalibration->mVector.size())
            return std::map<double, double>();

        auto maxIdx = mCalibration->mVector.size()-1;
        for (auto itv = mCalibration->mVector.rbegin(); itv!= mCalibration->mVector.rend(); itv++) {
            if (*itv >=threshold) break;
            maxIdx--;
        }

        tminCal = mCalibration->mTmin + minIdx * mCalibration->mStep;
        tmaxCal = mCalibration->mTmin + maxIdx * mCalibration->mStep;

        curve.assign(mCalibration->mVector.begin()+minIdx, mCalibration->mVector.begin()+(maxIdx + 1));

        curve = equal_areas(curve, mCalibration->mStep, 1.0);
        calib = vector_to_map(curve, tminCal, tmaxCal, mCalibration->mStep );

    } else {
        calib = mCalibration->mMap;
    }

    calib[calib.cbegin()->first - mCalibration->mStep] = 0.0;
    calib[calib.crbegin()->first + mCalibration->mStep] = 0.0;

    return DateUtils::convertMapToAppSettingsFormat(calib);
}

inline const std::map<double, double> &Date::getRawWiggleCalibMap() const
{
    return mWiggleCalibration->mMap;
}

const std::map<double, double> Date::getFormatedWiggleCalibMap() const
{
    if (mWiggleCalibration == nullptr || mWiggleCalibration->mVector.empty())
        return std::map<double, double>();
    return DateUtils::convertMapToAppSettingsFormat(mWiggleCalibration->mMap);
}



const std::map<double, double> Date::getFormatedWiggleCalibToShow() const
{
    if (mWiggleCalibration == nullptr || mWiggleCalibration->mVector.empty())
        return std::map<double, double>();

    std::map<double, double> calib = getRawWiggleCalibMap();


    double tminCal, tmaxCal;
    std::vector<double> curve;
    const double threshold  = 0.01 * (*std::max_element(mWiggleCalibration->mVector.begin(), mWiggleCalibration->mVector.end()));

    int minIdx = 0;
    for (auto& v : mWiggleCalibration->mVector) {
        if (v >threshold) break;
        minIdx++;
    }

    auto maxIdx = mWiggleCalibration->mVector.size()-1;
    for (auto itv = mWiggleCalibration->mVector.rbegin(); itv!= mWiggleCalibration->mVector.rend(); itv++) {
        if (*itv >threshold) break;
        maxIdx--;
    }

    tminCal = mWiggleCalibration->mTmin + minIdx * mWiggleCalibration->mStep;
    tmaxCal = mWiggleCalibration->mTmin + maxIdx * mWiggleCalibration->mStep;

    //curve = mWiggleCalibration->mVector.mid(minIdx, (maxIdx - minIdx) + 1);
    curve.assign(mWiggleCalibration->mVector.begin()+minIdx, mWiggleCalibration->mVector.begin()+(maxIdx + 1));
    curve = equal_areas(curve, mWiggleCalibration->mStep, 1.);
    calib = vector_to_map(curve, tminCal, tmaxCal, mWiggleCalibration->mStep );

    calib[calib.cbegin()->first] = 0.;
    calib[calib.crbegin()->first] = 0.;

    return DateUtils::convertMapToAppSettingsFormat(std::move(calib));
}


std::vector<double> Date::getFormatedRepartition() const
{
    if (DateUtils::convertToAppSettingsFormat(mCalibration->mTmin)>DateUtils::convertToAppSettingsFormat(mCalibration->mTmax)) {
       // reverse the QVector and complement, we suppose it's the same step
        std::vector<double> repart;
        double lastValue = *mCalibration->mRepartition.crbegin();
        std::vector<double>::const_iterator iter = mCalibration->mRepartition.cend()-1;
        while (iter != mCalibration->mRepartition.cbegin()-1) {
             repart.push_back(lastValue-(*iter));
             --iter;
        }
        return repart;

    } else
        return mCalibration->mRepartition;

}


double Date::getFormatedTminRefCurve() const
{
    return std::min(DateUtils::convertToAppSettingsFormat(getTminRefCurve()), DateUtils::convertToAppSettingsFormat(getTmaxRefCurve()));
}

double Date::getFormatedTmaxRefCurve() const
{
    return std::max(DateUtils::convertToAppSettingsFormat(getTminRefCurve()), DateUtils::convertToAppSettingsFormat(getTmaxRefCurve()));
}

double Date::getFormatedTminCalib() const
{
    return std::min(DateUtils::convertToAppSettingsFormat(mCalibration->mTmin), DateUtils::convertToAppSettingsFormat(mCalibration->mTmax));
}

double Date::getFormatedTmaxCalib()const
{
    return std::max(DateUtils::convertToAppSettingsFormat(mCalibration->mTmin), DateUtils::convertToAppSettingsFormat(mCalibration->mTmax));
}

void Date::setBandwidth(BandwidthType bwt, double bandwidth)
{
    mTi.setBandwidth(bwt, bandwidth);
    mSigmaTi.setBandwidth(bwt, bandwidth);

    if ( !( mDeltaType == Date::eDeltaNone ) )
        mWiggle.setBandwidth(bwt, bandwidth);

}

void Date::generateFormatedKDE(const std::vector<ChainSpecs>& chains, const int fftLen, const double tmin, const double tmax)
{

    mTi.generateFormatedKDE(chains, fftLen, tmin, tmax);
    mSigmaTi.generateFormatedKDE(chains, fftLen);

    if ( !( mDeltaType == Date::eDeltaNone ) )
        mWiggle.generateFormatedKDE(chains, fftLen);

}

QPixmap Date::generateUnifThumb(const StudyPeriodSettings &settings)
{
    if (mIsValid){
        //  No need to draw the graph on a large size
        //  These values are arbitary
        const QSize size(1000, 150);
        QPixmap thumb(size);

        const double tLower = mData.value(DATE_UNIFORM_MIN_STR).toDouble();
        const double tUpper = mData.value(DATE_UNIFORM_MAX_STR).toDouble();

        const double tmin = settings.mTmin;
        const double tmax = settings.mTmax;

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

            graph.setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
            graph.setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

            graph.setXAxisMode(GraphView::AxisMode::eHidden);
            graph.setYAxisMode(GraphView::AxisMode::eHidden);

            const QColor color = mPlugin->getColor();

            const double tminDisplay = std::clamp(tLower, tmin, tmax);
            const double tmaxDisplay = std::clamp(tUpper, tmin, tmax);

            GraphCurve curve = horizontalSection(qMakePair(tminDisplay, tmaxDisplay), "Calibration", color, QBrush(color));
            curve.mVisible = true;
            graph.add_curve(curve);

            // Drawing the wiggle
            if (mDeltaType != eDeltaNone) {

                std::map<double, double> calibWiggle = normalize_map(getMapDataInRange(getRawWiggleCalibMap(), tmin, tmax));
                GraphCurve curveWiggle = densityCurve(calibWiggle, "Wiggle", Qt::red);
                curveWiggle.mVisible = true;
                graph.add_curve(curveWiggle);
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
QPixmap Date::generateCalibThumb(const StudyPeriodSettings& settings)
{
    if (mIsValid) {
        //  No need to draw the graph on a large size
        //  These values are arbitary
        const QSize size(1000, 150);

        const double tmin = settings.mTmin;
        const double tmax = settings.mTmax;

        const std::map<double, double> &calib = normalize_map(getMapDataInRange(getRawCalibMap(), tmin, tmax));

        if (calib.empty())
            return QPixmap();

        const QColor color = mPlugin->getColor();
        GraphCurve curve = densityCurve(calib, "Calibration", mPlugin->getColor(), Qt::SolidLine, color);
        curve.mVisible = true;
        curve.mIsRectFromZero = true; // When then ref curve is shorter than the study period

        GraphView graph;

        graph.add_curve(curve);
        
        // Drawing the wiggle
        if (mDeltaType != eDeltaNone) {

            const std::map<double, double> calibWiggle = normalize_map(getMapDataInRange(getRawWiggleCalibMap(), tmin, tmax));

            GraphCurve curveWiggle = densityCurve(calibWiggle, "Wiggle", Qt::blue, Qt::SolidLine, QBrush(Qt::NoBrush));
            curveWiggle.mVisible = true;
            graph.add_curve(curveWiggle);
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

        graph.setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
        graph.setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);
        graph.setXAxisMode(GraphView::AxisMode::eHidden);
        graph.setYAxisMode(GraphView::AxisMode::eHidden);
        graph.showYAxisLine(false);


        QPixmap thumb(size);
        // Vérification de la création réussie de QPixmap
        if (thumb.isNull()) {
            qDebug() << "[Date::generateCalibThumb] Failed to create QPixmap!";
            return QPixmap(); // Gestion d'erreur
        }

        // Remplissage du QPixmap avec une couleur transparente
        thumb.fill(Qt::transparent);

        QPainter p;
        if (!p.begin(&thumb)) {
            qDebug() << "[Date::generateCalibThumb] Failed to begin QPainter!";
            return QPixmap(); // Gestion d'erreur
        }

        graph.render(&p);
        p.end();

        graph.showInfos(false);

        return thumb;

    } else {
        // If date is invalid, return a null pixmap!
        return QPixmap();
    }

}

double Date::getLikelihoodFromCalib(const double &t) const
{
    return mCalibration->interpolate(t);
}

double Date::getLikelihoodFromWiggleCalib(const double &t) const
{
    // test si mWiggleCalibration existe, sinon calcul de la valeur
    if (mWiggleCalibration == nullptr || mWiggleCalibration->mVector.empty()) {

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
        return mWiggleCalibration->interpolate(t);
    }

}



void Date::updateDate_v3(const double theta, const double S02Theta)
{
    updateTi_v3(theta);

    updateDelta_v3(theta, S02Theta);
   // updateSigmaShrinkage_K(theta, S02Theta);
    mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
    updateSigma_Log10(theta, S02Theta);

    updateWiggle();
}

void Date::applyTi_v3(const double theta)
{
    switch (mTi.mSamplerProposal) {
    case SamplerProposal::eDatePrior:
        applyPrior(theta);
        break;
    case SamplerProposal::eRWAdaptGauss:
        applyMHAdaptGauss(theta);
        break;
    case SamplerProposal::eLikelihood:
    default:
        applyInversion(theta);
        break;
    }
}
// Utilise Xi
//Obsolete

/*
void Date::updateTi_v4(const double theta, const double S02Theta)
{

    const double try_ti = Generator::normalDistribution(mTi.value(), mTi.mSigmaMH);
    const double rate_q = 1.0;

    const double rate_L = getLikelihood(try_ti) / getLikelihood(mTi.value());

    const double h_p_x = h_prior_EDM2(theta, mTi.value(), mDelta, mXi, S02Theta);
    const double h_p_y = h_prior_EDM2(theta, try_ti,      mDelta, mXi, S02Theta);
    const double rate_p2 = h_p_y  / h_p_x;

    // La loi instrumentale avec mélange de distribution Calibré-Gaussienne, n'est pas symétrique
    // il faut faire le rapport q_xy/q_yx


    mTi.try_update(try_ti, rate_L * rate_p2 * rate_q);

    // mise à jour de x_i

    // Attention  dans wiki beta -> 1/beta dans la fonction gammaDistribution
    // ---------------------------------------------------------------------
    // 0.  Choix du mode d’échantillonnage
    // ---------------------------------------------------------------------
    //   A  → Gamma direct
    //   B  → Random‑Walk gaussienne tronquée sur le log‑échelle
    //   C  → Random‑Walk gaussienne tronquée sur l’échelle naturelle
    // ---------------------------------------------------------------------

    #define SAMPLER_MODE 'A'          // <-- choisissez A, B ou C
    // ---------------------------------------------------------------------
    // 1.  Paramètres du prior Gamma (déclarés une fois, visibles partout)
    // ---------------------------------------------------------------------
    constexpr double alpha_gamma = 0.5;   // shape
    constexpr double beta_gamma  = 0.5;   // scale  (β > 0)
    constexpr double teta_gamma  = 1.0 / beta_gamma;
    // ---------------------------------------------------------------------
    // 2.  Variables d’adaptation (static → persiste entre appels)
    // ---------------------------------------------------------------------
    static double sigmaMH   = 1.0;                // écart‑type de la proposition
    static std::size_t iter = 0;                 // compteur d’itérations
    // ---------------------------------------------------------------------
    const double log_Xi_curr = std::log(mXi);   // état courant en log(e)

#if defined(SAMPLER_MODE) && (SAMPLER_MODE == 'A')
    // -----------------------------------------------------------------
    // A – Proposition Gamma directe (tirage du prior)
    // -----------------------------------------------------------------
    // 5.1  Tirage direct d’une loi Gamma(α=0.5 , β=0.5)
    //      (dans la documentation de la fonction, β = 1/θ)
    double try_x_i = Generator::gammaDistribution(alpha_gamma, teta_gamma);

    // 5.2  Log‑densités du prior EDM2 (déjà en log, pas de Jacobien)
    const double log_h_p_Xx = log_h_prior_EDM2(theta, mTi.value(), mDelta, mXi,     S02Theta);
    const double log_h_p_Xy = log_h_prior_EDM2(theta, mTi.value(), mDelta, try_x_i, S02Theta);

    // 5.3  Log‑ratio d’acceptation (seulement le facteur EDM2,
    //     le prior Gamma s’annule car on a tiré *exactement* dans ce prior)
    double log_alpha = log_h_p_Xy - log_h_p_Xx;

#endif

    auto logGammaPrior = [&](double y) -> double
    {
        // log p_Gamma(e^y) = (α‑1)·y – e^y / θ   (+ constantes ignorées), notation wiki
        return (alpha_gamma - 1.0) * y - std::exp(y) / teta_gamma;
    };

#if defined(SAMPLER_MODE) && (SAMPLER_MODE == 'B')
    // ---------------------------------------------------------------------
    // B.  Proposition (Random‑Walk gaussienne tronquée) sur le log‑échelle
    // ---------------------------------------------------------------------
    double log_Xi_prop = Generator::truncatedNormal(
        log_Xi_curr,   // μ
        sigmaMH,           // σ
        -6.0,          // lower bound
        100.0);        // upper bound

    // Retour à l’échelle naturelle pour le prior EDM2
    const double try_x_i = std::exp(log_Xi_prop);
    // -------------------------------------------------------------
    //   f)  Log‑ratio d’acceptation (la proposition est symétrique,
    //       donc le facteur q(x→y)/q(y→x) = 1 et disparaît)
    // -------------------------------------------------------------


    const double log_h_gamma_x = logGammaPrior(log_Xi_curr);
    const double log_h_gamma_y = logGammaPrior(log_Xi_prop);

    const double log_h_p_Xx = log_h_prior_EDM2(theta, mTi.value(), mDelta, mXi,     S02Theta);
    const double log_h_p_Xy = log_h_prior_EDM2(theta, mTi.value(), mDelta, try_x_i, S02Theta);

    // ---------------------------------------------------------------------
    // 8.  Log‑ratio d’acceptation (inclut le Jacobien y) si echantillonneur echelle log
    // ---------------------------------------------------------------------
    const double log_alpha = (log_h_gamma_y + log_h_p_Xy + log_Xi_prop)
                             - (log_h_gamma_x + log_h_p_Xx + log_Xi_curr);
#endif

#if defined(SAMPLER_MODE) && (SAMPLER_MODE == 'C')
    // -----------------------------------------------------------------
    // C – Random‑Walk gaussienne tronquée sur l’échelle naturelle
    //-----------------------------------------------------------------
    // 5.1  Proposition asymétrique : N(mXi , sigmaMH²) tronquée à x>0
    const double try_x_i = Generator::truncatedNormal(
        mXi,   // μ
        sigmaMH,           // σ
        0,          // lower bound
        5);        // upper bound

    const double log_Xi_prop = log(try_x_i);

    const double log_h_gamma_x = logGammaPrior(log_Xi_curr);
    const double log_h_gamma_y = logGammaPrior(log_Xi_prop);

    const double log_h_p_Xx = log_h_prior_EDM2(theta, mTi.value(), mDelta, mXi,     S02Theta);
    const double log_h_p_Xy = log_h_prior_EDM2(theta, mTi.value(), mDelta, try_x_i, S02Theta);

    // ---------------------------------------------------------------------
    // 8.  Log‑ratio d’acceptation
    // ---------------------------------------------------------------------
    const double log_alpha = (log_h_gamma_y + log_h_p_Xy )
                           - (log_h_gamma_x + log_h_p_Xx );
#endif



    // ---------------------------------------------------------------------
    // 7.  Test d’acceptation (Metropolis)
    // ---------------------------------------------------------------------
    bool accepted = MHAcceptanceTest_log(log_alpha);
    if (accepted) {
        mXi = try_x_i;          // on accepte la proposition
    }

    // ---------------------------------------------------------------------
    // 8.  Adaptation de sigmaMH (Robbins‑Monro)
    // ---------------------------------------------------------------------
    ++iter;                                 // incrément du compteur
    constexpr double target = 0.44;        // taux d’acceptation cible (1‑D)
    constexpr double gamma0 = 0.05;        // amplitude du pas d’adaptation
    constexpr double eta    = 0.6;         // décroissance du pas (0.5 < η ≤ 1)
    double gamma_k = gamma0 / std::pow(static_cast<double>(iter), eta);

    sigmaMH = std::exp( std::log(sigmaMH)
                       + gamma_k * ( (accepted ? 1.0 : 0.0) - target ) );
    // (optionnel) on borne sigmaMH pour éviter des valeurs extrêmes
    //sigmaMH = std::clamp(sigmaMH, 0.01, 5.0);

#ifdef DEBUG_no
    // ---------------------------------------------------------------------
    // 8.  DEBUG – statistiques d’acceptation et suivi de sigmaMH
    // ---------------------------------------------------------------------
    static std::size_t naccept = 0;
    static std::size_t ntot    = 0;
    static double mean = 0.0;

    if (accepted) ++naccept;
    ++ntot;
    mean += (mXi - mean) / static_cast<double>(ntot);

    std::cout << " mXi  accept% = "
              << 100.0 * static_cast<double>(naccept) / static_cast<double>(ntot)
              << "%   mean = " << mean
              << "   sigmaMH = " << sigmaMH << '\n';
#endif

}

// Utilise Xi
void Date::applyTi_v4(const double theta, const double S02Theta)
{
    // mise à jour de x_i
    // ---------------------------------------------------------------------
    // 1.  Paramètres du prior Gamma (déclarés une fois, visibles partout)
    // ---------------------------------------------------------------------
    constexpr double alpha_gamma = 0.5;   // shape
    constexpr double beta_gamma  = 0.5;   // scale  (β > 0)
    constexpr double teta_gamma  = 1.0 / beta_gamma;

#if defined(SAMPLER_MODE) && (SAMPLER_MODE == 'A')
    // -----------------------------------------------------------------
    // A – Proposition Gamma directe (tirage du prior)
    // -----------------------------------------------------------------
    // 5.1  Tirage direct d’une loi Gamma(α=0.5 , β=0.5)
    //      (dans la documentation de la fonction, β = 1/θ)
    double try_x_i = Generator::gammaDistribution(alpha_gamma, teta_gamma);

    // 5.2  Log‑densités du prior EDM2 (déjà en log, pas de Jacobien)
    const double log_h_p_Xx = log_h_prior_EDM2(theta, mTi.value(), mDelta, mXi,     S02Theta);
    const double log_h_p_Xy = log_h_prior_EDM2(theta, mTi.value(), mDelta, try_x_i, S02Theta);

    // 5.3  Log‑ratio d’acceptation (seulement le facteur EDM2,
    //     le prior Gamma s’annule car on a tiré *exactement* dans ce prior)
    double log_alpha = log_h_p_Xy - log_h_p_Xx;

#endif

    if (MHAcceptanceTest_log(log_alpha)) {
        mXi = try_x_i;
    }

    // mise à jour de ti
    double try_ti;

    if (Generator::randomUniform() < mMixingLevel) { // try_ti always in the study period
        const double tminCalib = mCalibration->mTmin;
        const double u = Generator::randomUniform();
        const double idx = interpolate_index(u, mCalibration->mRepartition);
        try_ti = tminCalib + idx * mCalibration->mStep;

    } else {
        // -- gaussian -- try_ti can be outside the study period
        const double t0 = mTi.value();
        const double s = (mSettings.mTmax - mSettings.mTmin) / 2.0;
        try_ti = Generator::normalDistribution(t0, s);
    }

    const double rate_p1 = getLikelihood(try_ti) / getLikelihood(mTi.value());

    const double h_p_x = h_prior_EDM2(theta, mTi.value(), mDelta, mXi, S02Theta);
    const double h_p_y = h_prior_EDM2(theta, try_ti,      mDelta, mXi, S02Theta);
    const double rate_p2 = h_p_y  / h_p_x;

    // La loi instrumentale avec mélange de distribution Calibré et une Gaussienne, n'est pas symétrique
    // il faut faire le rapport q_xy/q_yx

    const double rate_q = fProposalDensity(mTi.value(), try_ti) / fProposalDensity(try_ti, mTi.value());

    if (MHAcceptanceTest(rate_p1 * rate_p2 * rate_q)) {
        mTi.setValue(try_ti);
    }




}
*/
// identique à updateDate, mais sans mémorisation des valeurs

void Date::applyDateProposal_v3(const double theta, const double S02Theta)
{
    switch (mTi.mSamplerProposal) {
    case SamplerProposal::eDatePrior:
        applyPrior(theta);
        break;

     // only case with acceptation rate, because we use sigmaMH :
    case SamplerProposal::eRWAdaptGauss:
        applyMHAdaptGauss(theta);
        break;

    case SamplerProposal::eLikelihood:
    default:
        applyInversion(theta);
        break;
    }


    updateDelta(theta, S02Theta); // pas de memo
    applySigmaShrinkage_K_tempering(theta, S02Theta); // Par defaut T=1, càd pas de recuit

    updateWiggle(); // mise à jour déterministe, pas de tirage
}


// obsolete, il n'y a pas un proposol MH
void Date::applyTi_MH_Tempering(const double theta_mX, const double T)
{
    const double u1 = Generator::randomUniform();

    const double tminCalib = mCalibration->mTmin;
    const double idx = interpolate_index(u1, mCalibration->mRepartition);
    double ti_try = tminCalib + idx * mCalibration->mStep;

    const double log_alpha =
        (log_dnorm(ti_try, theta_mX, mSigmaTi.value())
         - log_dnorm(mTi.value(), theta_mX, mSigmaTi.value())) / T;


    if (MHAcceptanceTest_log(log_alpha))
        mTi.setValue(ti_try);


    updateDelta(theta_mX, 1.0); // pas de memo

    //updateSigmaShrinkage_K(theta_mX, S02Theta_mX, AShrinkage); // ici, peut être garder

    updateWiggle(); // mise à jour déterministe, pas de tirage
}




/**
 * @brief TDate::initDelta Init the wiggle shift
 */
void Date::initDelta()
{
    switch (mDeltaType) {
        case eDeltaNone:
            mDelta = 0.0;
            break;
        
        case eDeltaRange:
            mDelta = Generator::randomUniform(mDeltaMin, mDeltaMax);
            break;
        
        case eDeltaGaussian: {
            // change init of Delta in case of gaussian function since 2015/06 with PhL
            //mDelta = event->mTheta.mX - mTheta.mX;
            const double tmin = mDeltaAverage - 5 * mDeltaError;
            const double tmax = mDeltaAverage + 5 * mDeltaError;
            mDelta = Generator::truncatedNormal(mDeltaAverage, mDeltaError, tmin, tmax);
            }
            break;
        
        case eDeltaFixed:
            mDelta = mDeltaFixed;
            break;
        
    }

    mWiggle.mLastMHAccepts.clear();
}


// C'est un tirage pour un MCMC Gibbs en utilisant le Prior Event
void Date::updateDelta_v3(const double theta, const double )
{
    const double lambda_i = theta - mTi.value();
    
    switch (mDeltaType) {
        case eDeltaNone:
            mDelta = 0.0;
            break;
        
        case eDeltaRange:
            mDelta = Generator::gaussByDoubleExp(lambda_i, mSigmaTi.value(), mDeltaMin, mDeltaMax);
            break;
        
        case eDeltaGaussian: {
            const double w_i = ( 1.0/(mSigmaTi.value() * mSigmaTi.value()) ) + ( 1.0/(mDeltaError * mDeltaError) );
            const double deltaAvg = (lambda_i / (mSigmaTi.value() * mSigmaTi.value()) + mDeltaAverage / (mDeltaError * mDeltaError)) / w_i;
            const double x = Generator::normalDistribution(0, 1);
            mDelta = deltaAvg + x / sqrt(w_i);

        }
            break;
        
        case eDeltaFixed:
            mDelta = mDeltaFixed;
            break;
        
    }
}

/*
void Date::updateDelta_v4(const double theta, const double S02Theta)
{
    double try_delta;

    switch (mDeltaType) {
    case eDeltaNone:
        mDelta = 0.0;
        return;
        break;

    case eDeltaFixed:
        mDelta = mDeltaFixed;
        return;
        break;

    case eDeltaRange:
        try_delta = Generator::randomUniform( mDeltaMin, mDeltaMax);
        break;

    case eDeltaGaussian:
       try_delta = Generator::normalDistribution(mDeltaAverage, mDeltaError);
       break;
    }

    const double h_p_x = h_prior_EDM2(theta, mTi.value(), mDelta,    mXi, S02Theta);
    const double h_p_y = h_prior_EDM2(theta, mTi.value(), try_delta, mXi, S02Theta);
    const double rate_p_xy = h_p_y  / h_p_x;

    if (MHAcceptanceTest(rate_p_xy)) {
        mDelta = try_delta;
    }

}

// fonction identique à updateDelta_v4 car il n'y a pas de memo
void Date::applyDelta_v4(const double theta, const double S02Theta)
{
    double try_delta;

    switch (mDeltaType) {
    case eDeltaNone:
        mDelta = 0.0;
        return;
        break;

    case eDeltaFixed:
        mDelta = mDeltaFixed;
        return;
        break;

    case eDeltaRange:
        try_delta = Generator::randomUniform( mDeltaMin, mDeltaMax);
        break;

    case eDeltaGaussian:
        try_delta = Generator::normalDistribution(mDeltaAverage, mDeltaError);
        break;
    }

    const double h_p_x = h_prior_EDM2(theta, mTi.value(), mDelta,    mXi, S02Theta);
    const double h_p_y = h_prior_EDM2(theta, mTi.value(), try_delta, mXi, S02Theta);
    const double rate_p_xy = h_p_y  / h_p_x;

    if (MHAcceptanceTest(rate_p_xy)) {
        mDelta = try_delta;
    }

}
*/

// obsolete
void Date::updateSigmaJeffreys(const double theta_mX)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double lambda = pow(mTi.value() - (theta_mX - mDelta), 2) / 2.0;

    const double a = 0.0001; //precision
    const double b = pow(mSettings.mTmax - mSettings.mTmin, 2.);

    const double V1 = mSigmaTi.value() * mSigmaTi.value();

    double V2 (0.);
    do {
        const double logV2 = Generator::normalDistribution(log10(V1), mSigmaTi.mSigmaMH);
        V2 = pow(10, logV2);

    } while ((V2<a) || (V2>b));
    
    
    const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
    const double x2 = V1/V2;
    const double rate = x1 * sqrt(V1/V2) * x2 * V2 / V1; // (V2 / V1) est le jacobien!

    mSigmaTi.try_update(sqrt(V2), rate);
}

void Date::updateSigmaShrinkage(const double theta_mX, const double S02Theta_mX)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double mu = pow(mTi.value() - (theta_mX - mDelta), 2) / 2.0;

    constexpr int logVMin = -6;
    constexpr int logVMax = 100;

    const double V1 = mSigmaTi.value() * mSigmaTi.value();
    const double logV2 = Generator::normalDistribution(log10(V1), mSigmaTi.mSigmaMH);
    const double V2 = pow(10, logV2);

    if (logV2 >= logVMin && logV2 <= logVMax) {
        const double x1 = exp(-mu * (V1 - V2) / (V1 * V2));
        const double x2 = pow((S02Theta_mX + V1) / (S02Theta_mX + V2), 1.0 + 1.0);
        const double rate = x1 * sqrt(V1/V2) * x2 * V2 / V1 ; // (V2 / V1) est le jacobien!

        mSigmaTi.try_update(sqrt(V2), rate);

    } else {
       mSigmaTi.reject_update();
 //       qDebug()<<"[TDate::updateSigma] x1 x2 rapport rejet";
    }
}

void Date::updateSigmaShrinkage0(const double theta_mX,
                                const double S02Theta_mX)
{
    // ---------------------------------------------------------
    // Target terms
    // ---------------------------------------------------------
    const double mu = pow(mTi.value() - (theta_mX - mDelta), 2) / 2.0;

    constexpr double logVMin = -6.0;
    constexpr double logVMax = 100.0;
    //constexpr double LN10    = 2.302585092994046;

    const double V1 = mSigmaTi.value() * mSigmaTi.value();
    const double logV1 = log10(V1);

    double logV2 = 0;          // log10(V) proposé
    double V2;             // sigma² proposé (déduit de logV2)

    double log_proposal_ratio = 0.0; // log q(old|new) – log q(new|old)
    double log_jacobian = 0.0;
    double log_rate = 0.0;
    // ---------------------------------------------------------
    // Mixture weights (fixes ici, adaptables si voulu)
    // ---------------------------------------------------------
    constexpr double w1 = 0.3;   // RW local
    constexpr double w2 = 0.3;   // RW large
    //constexpr double w3 = 0.1;   // Independence heavy-tail

    const double u = Generator::randomUniform();

    // =========================================================
    // q1 : Random Walk local (symétrique)
    // =========================================================
   /* if (u < w1) {

        logV2 = Generator::normalDistribution(logV1, mSigmaTi.mSigmaMH);

        log_proposal_ratio = 0.0; // symétrique

        log_jacobian = logV2 - log(V1);
    }*/
    // kernel shrinkage
    if (u < w1) {

        V2 = Generator::shrinkageUniforme(S02Theta_mX);

       // log_proposal_ratio = 0.0; // symétrique

        log_jacobian = 0;
        // Densité proposal q(V) ∝ (S0² + V)^(-(A+1))
        // donc log q(V) = -(A+1) * log(S0² + V) + C
        const double log_q1 = -(1.0 + 1.0) * log(S02Theta_mX + V1);

        const double log_q2 = -(1.0 + 1.0) * log(S02Theta_mX + V2);
        log_proposal_ratio = log_q1 - log_q2;

        log_rate = + 0.5*(log(V1)-log(V2)) + log_proposal_ratio;
    }
    // =========================================================
    // q2 : Random Walk large multiplicatif (symétrique)
    // =========================================================
    else if (u < w1 + w2) {

        logV2 = Generator::normalDistribution(logV1, 5.0);// * mSigmaTi.mSigmaMH);

        log_proposal_ratio = 0.0; // symétrique
        log_jacobian = logV2 - log(V1);


    }

    // =========================================================
    // q3 : Independence heavy-tail shrinkage
    // π(V) ∝ (S0² + V)^(-(A+1))
    // On approxime via Pareto-like :
    // V = S0² * ( (1-u)^(-1/A) - 1 )
    // =========================================================
    else {

        const double u2 = Generator::randomUniform();

        V2 = S02Theta_mX * (pow(1.0 - u2, -1.0 / 1.0) - 1.0);

        logV2 = log10(V2); // on repasse en log10

        // Densité proposal q(V) ∝ (S0² + V)^(-(A+1))
        // donc log q(V) = -(A+1) * log(S0² + V) + C
        const double log_q1 = -(1.0 + 1.0) * log(S02Theta_mX + V1);

        const double log_q2 = -(1.0 + 1.0) * log(S02Theta_mX + V2);

        // ratio log q(old|new) – log q(new|old)
        log_proposal_ratio = log_q1 - log_q2;
        log_jacobian = 0;
    }

    // ---------------------------------------------------------
    // Support check
    // ---------------------------------------------------------
    if (logV2 < logVMin || logV2 > logVMax) {
        mSigmaTi.reject_update();
        return;
    }

    V2 = pow(10.0, logV2);

    // ---------------------------------------------------------
    // Log target ratio
    // ---------------------------------------------------------
    // Likelihood terme avec theta dans mu
    const double log_x1 =  -mu * (V1 - V2) / (V1 * V2);

    // Prior shrinkage term
    const double log_x2 = (1.0 + 1.0) *  (log(S02Theta_mX + V1) - log(S02Theta_mX + V2));

    // Jacobian du changement de variable V = sigma²  (log10 → V)
    // d log10(V) / dV = 1 / (V * ln(10))  →  log10(V) = log10(V) + 1
    // En travaillant directement en log10, le facteur de conversion
    // est simplement +1 (car log10(10) = 1)
   // const double log_jacobian = 0;//logV2 - logV1 + 1.0;   // = log10(V2) - log10(V1) + log10(10)
    // const double log_jacobian = log(V2) - log(V1);// + log(LN10);

    const double log_target_ratio = log_x1 + log_x2 + log_jacobian + 0.5*(log(V1)-log(V2));

    // ---------------------------------------------------------
    // Final MH
    // ---------------------------------------------------------
    log_rate = log_target_ratio + log_proposal_ratio;

    mSigmaTi.try_update_log(sqrt(V2), log_rate);
}


#pragma mark update SigmaTi
/*
void Date::updateSigmaShrinkage_K(const double theta_mX,
                                  const double S02Theta_mX)
{
    const double mu = pow(mTi.value() - (theta_mX - mDelta), 2.) * 0.5;
    const double V1 = mSigmaTi.value() * mSigmaTi.value();

    double rapport = -1, V2;

    // ---------------------------------------------------------
    // Mixture weights (fixes ici, adaptables si voulu)
    // ---------------------------------------------------------
    constexpr double w1 = 0.;   // RW local
    //constexpr double w2 = 0.5;   // RW large

    const double u = Generator::randomUniform();

    if (u < w1) {
        const double VMin = 0.;
        const double VMax = 1.E+10;

        V2 = Generator::shrinkageUniforme(S02Theta_mX);

        if (VMin<V2 && V2<VMax) {
            //const double dexp = exp(-mu * (V1 -V2)/(V1*V2));
            // Likelihood term
            const double log_x1 =  -mu * (V1 - V2) / (V1 * V2);
            //rapport = dexp * sqrt(V1/V2);
            const double log_rate = log_x1 + 0.5*(log(V1)-log(V2));
            mSigmaTi.try_update_log(sqrt(V2), log_rate);
            return;
        }
    }
        // =========================================================
        // q1 : Random Walk local (symétrique)
        // =========================================================
    else {
        const int logVMin = -100;
        const int logVMax = 100;

        const double logV2 = Generator::normalDistribution(log10(V1), mSigmaTi.mSigmaMH);

        V2 = pow(10, logV2);

        if (logV2 >= logVMin && logV2 <= logVMax) {
            const double x1 = exp(-mu * (V1 - V2) / (V1 * V2));
            // Likelihood term
            //const double log_x1 =  -mu * (V1 - V2) / (V1 * V2);

            const double x2 = pow((S02Theta_mX + V1) / (S02Theta_mX + V2), 1.0 + 1.0);

            rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1 ; // (V2 / V1) est le jacobien!
            mSigmaTi.try_update(sqrt(V2), rapport);
            return;
        }

    }
    mSigmaTi.reject_update();

}
*/

//----- test
// Sigma tiré par adaptation directement en variance
// Ca marche

void Date::updateSigma_Variance(const double theta,
                                const double S02Theta)
{
    const double diff = mTi.value() - (theta - mDelta);
    const double mu = 0.5 * diff * diff;

    const double V1 = mSigmaTi.value() * mSigmaTi.value();
    const double tau = mSigmaTi.mSigmaMH; // Écart-type du saut

    const double VMin = 1e-20;
    const double VMax = 1e20; // Borne sup raisonnable pour la variance

    // 1. Proposition tronquée sur [VMin, VMax] centrée en V1
    const double V2 = Generator::truncatedNormal(V1, tau, VMin, VMax);

    // 2. Terme de vraisemblance : -mu * (1/V2 - 1/V1)
    const double log_vraisemblance = -mu * (V1 - V2) / (V1 * V2);

    // 3. Prior 1/sigma_t (soit V^(-1/2))
    const double log_prior_sigma = 0.5 * (std::log(V1) - std::log(V2));

    // 4. Prior Shrinkage : ((s02 + V1) / (s02 + V2))^2
    const double log_prior_shrinkage = 2.0 * (std::log(S02Theta + V1) - std::log(S02Theta + V2));

    // 5. Correction de Hastings pour la proposition tronquée : log( q(V1|V2) / q(V2|V1) )
    // Z(center, tau) = CDF((VMax - center)/tau) - CDF((VMin - center)/tau)
    const double log_q_ratio = std::log(normalCDF((VMax - V1) / tau) - normalCDF((VMin - V1) / tau))
                               - std::log(normalCDF((VMax - V2) / tau) - normalCDF((VMin - V2) / tau));

    // Log-rate global
    const double log_rate = log_vraisemblance + log_prior_sigma + log_prior_shrinkage + log_q_ratio;

    mSigmaTi.try_update_log(std::sqrt(V2), log_rate);
}



// Echantillonnage Indépendante via shrinkageUniforme
void Date::updateSigma_Shrinkage(const double theta,
                                 const double S02Theta)
{
    const double diff = mTi.value() - (theta - mDelta);
    const double mu = 0.5 * diff * diff;
    const double V1 = mSigmaTi.value() * mSigmaTi.value();

    //  Indépendante via shrinkageUniforme
    const double VMin = 1e-120;
    const double VMax = 1e100;

    const double V2 = Generator::shrinkageUniforme(S02Theta);

    if (V2 > VMin && V2 < VMax) {
        const double log_likelihood_diff = -mu * (V1 - V2) / (V1 * V2);
        const double log_prior_diff = 0.5 * (std::log(V1) - std::log(V2));

        const double log_rate = log_likelihood_diff + log_prior_diff;
        mSigmaTi.try_update_log(std::sqrt(V2), log_rate);
        return;
    }

    mSigmaTi.reject_update();
}

//Marche adaptatif  en log10(V) pour Gibbs
void Date::updateSigma_Log10(const double theta, const double S02Theta)
{
    mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss;
    const double diff = mTi.value() + mDelta - theta;
    const double mu = 0.5 * diff * diff;
    const double V1 = mSigmaTi.value() * mSigmaTi.value();

    const double logVMin = -20.0;
    const double logVMax = 20.0;

    const double log10_V1 = std::log10(V1);
    const double log10_V2 = Generator::normalDistribution(log10_V1, mSigmaTi.mSigmaMH);

    if (log10_V2 >= logVMin && log10_V2 <= logVMax) {
        const double V2 = std::pow(10.0, log10_V2);

        // 1. Terme de vraisemblance : -mu * (1/V2 - 1/V1)
        const double log_vraisemblance = -mu * (V1 - V2) / (V1 * V2);

        // 2. Terme du prior 1/sigma_t (soit V^(-1/2))
        const double log_prior_sigma = 0.5 * (std::log(V1) - std::log(V2));

        // 3. Terme de prior Shrinkage : ((s02 + V1) / (s02 + V2))^2
        const double log_prior_shrinkage = 2.0 * (std::log(S02Theta + V1) - std::log(S02Theta + V2));

        // 4. Jacobien de la proposition
        const double log_jacobian = std::log(V2) - std::log(V1);

        // Log-rate global de MH
        const double log_rate = log_vraisemblance + log_prior_sigma + log_prior_shrinkage + log_jacobian;

        mSigmaTi.try_update_log(std::sqrt(V2), log_rate);
        return;
    }

    mSigmaTi.reject_update();
}

void Date::updateSigmaShrinkage_K(const double theta_mX,
                                  const double S02Theta_mX)
{
    const double diff = mTi.value() - (theta_mX - mDelta);
    const double mu = 0.5 * diff * diff;
    double V1 = mSigmaTi.value() * mSigmaTi.value();

    constexpr double w1 = 1.; // Poids pour mélange (désactivé ici)
    const double u = Generator::randomUniform();

    if (u < w1) {
        // Branche 1 : Indépendante via shrinkageUniforme
        const double V2 = Generator::shrinkageUniforme(S02Theta_mX, 1e-100, 1e100);

        const double log_likelihood_diff = -mu * (V1 - V2) / (V1 * V2);
        const double log_prior_diff = 0.5 * (std::log(V1) - std::log(V2));

        const double log_rate = log_likelihood_diff + log_prior_diff;
        mSigmaTi.try_update_log(std::sqrt(V2), log_rate);
        return;

    }
    else {
        mSigmaTi.mSamplerProposal = SamplerProposal::eRWAdaptGauss; // test ici
        // Branche 2 : Marche aléatoire en log10(V)
        const double logVMin = -100.0;
        const double logVMax = 100.0;

        const double log10_V1 = std::log10(V1);
        const double log10_V2 = Generator::normalDistribution(log10_V1, mSigmaTi.mSigmaMH);

        if (log10_V2 >= logVMin && log10_V2 <= logVMax) {
            const double V2 = std::pow(10.0, log10_V2);

            // 1. Terme de vraisemblance : -mu * (1/V2 - 1/V1)
            const double log_vraisemblance = -mu * (V1 - V2) / (V1 * V2);

            // 2. Terme du prior 1/sigma_t (soit V^(-1/2))
            const double log_prior_sigma = 0.5 * (std::log(V1) - std::log(V2));

            // 3. Terme de prior Shrinkage : ((s02 + V1) / (s02 + V2))^2
            const double log_prior_shrinkage = 2.0 * (std::log(S02Theta_mX + V1) - std::log(S02Theta_mX + V2));

            // 4. Jacobien : dV/dy = V (y = ln V), donc facteur V2/V1 dans le ratio
            const double log_jacobian = std::log(V2) - std::log(V1);

            // Log-rate global de MH
            const double log_rate = log_vraisemblance + log_prior_sigma + log_prior_shrinkage + log_jacobian;

            mSigmaTi.try_update_log(std::sqrt(V2), log_rate);
            return;
        }
    }

    mSigmaTi.reject_update();
}

void Date::updateSigmaWithX(const double theta, const double S02Theta)
{
    // --- Marcheur sur x_i (variable auxiliaire), diff fixé pour cette itération ---
    const double diff = mTi.value() - (theta - mDelta);

    // x1 = état courant de la chaîne pour x_i (PAS dérivé de diff — même piège qu'avant à éviter)
    const double x1 = mXi;

    // Proposition indépendante : Gamma(shape=1/2, rate=1/2) => scale = 1/rate = 2.0
    const double x2 = Generator::gammaDistribution(0.5, 2.0);

    if (x2 > 0.0) {
        const double denom1 = diff * diff + S02Theta * x1;
        const double denom2 = diff * diff + S02Theta * x2;

        // Le morceau x^{-1/2} e^{-x/2} s'annule avec la proposition : il ne reste que ceci
        const double log_rate = (std::log(x2) - 2.0 * std::log(denom2))
                                - (std::log(x1) - 2.0 * std::log(denom1));

        if (MHAcceptanceTest_log(log_rate)) {
            mXi = x2;
        }
    }

    // Valeur déterministe de σ²_ti à partir de l'état accepté de x_i
    //const double sigmaTi2 = (diff * diff) / (2.0 * mXi);
    double sigmaTiFloor2 = 0.1 * S02Theta; /* ex: fraction du s_i de calibration ou de S02Theta */;
    const double sigmaTi2 = std::max((diff * diff) / (2.0 * mXi), sigmaTiFloor2);
    mSigmaTi.accept_update(std::sqrt(sigmaTi2)); // pas de MH ici — pure transformation
}

// https://en.wikipedia.org/wiki/Lomax_distribution with alpha=1
void Date::applySigmaShrinkage_K_tempering(const double theta_mX,
                                           const double S02Theta_mX,
                                           const double T)
{
    const double mu = pow(mTi.value() - (theta_mX - mDelta), 2.) * 0.5;
    const double V1 = mSigmaTi.value() * mSigmaTi.value();

    // ---------------------------------------------------------
    // Mixture weights (fixes ici, adaptables si voulu)
    // ---------------------------------------------------------
    constexpr double w1 = 1.;   // RW local

    const double u = Generator::randomUniform();

    if (u < w1) {
        const double VMin = 1.E-6;
        const double VMax = 1.E+10;

        double V2 = Generator::shrinkageUniforme(S02Theta_mX);

        if (VMin<V2 && V2<VMax) {
            // Likelihood term
            const double log_x1 =  -mu * (V1 - V2) / (V1 * V2);
            const double log_rate = log_x1 + 0.5*(log(V1)-log(V2));

            if (MHAcceptanceTest_log(log_rate/T)) {
                mSigmaTi.setValue(sqrt(V2));
            }
            return;
        }
    }
    // =========================================================
    // q1 : Random Walk local (symétrique)
    // =========================================================
    else {

        const int logVMin = -6;
        const int logVMax = 100;

        const double logV2 = Generator::truncatedNormal(log10(V1), mSigmaTi.mSigmaMH, logVMin, logVMax);
        //const double mu_centre = 2. * mu;
        //const double logV2 = log10(mu_centre) + Generator::normalDistribution(0, 5); // test
        double V2 = pow(10, logV2);


        const double x1 = exp(-mu * (V1 - V2) / (V1 * V2));
        // Likelihood term
        //const double log_x1 =  -mu * (V1 - V2) / (V1 * V2);

        const double x2 = pow((S02Theta_mX + V1) / (S02Theta_mX + V2), 1.0 + 1.0);// a priori shrinkage

        double rate = x1 * sqrt(V1/V2) * x2 * V2 / V1 ; // (V2 / V1) est le jacobien!

        const double log_rate = log(rate) / T;

        if (MHAcceptanceTest(exp(log_rate) )) {
            mSigmaTi.setValue(sqrt(V2));
        }

        return;


    }


}

/*
void Date::updateSigmaShrinkage_K(const Event* event)
{
    const double lambda = pow(mTi.mX - (event->mTheta.mX - mDelta), 2.) / 2.;
    const double V1 = mSigmaTi.mX * mSigmaTi.mX;

    double rapport = -1, V2;
    if (mTi.mSamplerProposal != SamplerProposal::eRWAdaptGauss) {
        const double VMin = 0.;
        const double VMax = 1.E+10;

        V2 = Generator::shrinkageUniforme(event->mS02Theta.mX);

        if (VMin<V2 && V2<VMax) {
            const double dexp =exp(-lambda * (V1 -V2)/(V1*V2));
            rapport = dexp * sqrt(V1/V2);
        }

    } else {
        const int logVMin = -6;
        const int logVMax = 100;


        const double logV2 = Generator::normalDistribution(log10(V1), mSigmaTi.mSigmaMH);
        V2 = pow(10, logV2);

        if (logV2 >= logVMin && logV2 <= logVMax) {
            const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
            const double x2 = pow((event->mS02Theta.mX + V1) / (event->mS02Theta.mX + V2), event->mAShrinkage + 1.);
            rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1 ; // (V2 / V1) est le jacobien!

        }

    }

    mSigmaTi.tryUpdate(sqrt(V2), rapport);

}


*/
/*
void Date::updateSigma_v4(Event* event)
{

    const double VMin = 0.;
    const double VMax = 1.E+10;
    double rapport = -1, V2;
    const double V1 = mSigmaTi.mX * mSigmaTi.mX;

    V2 = Generator::shrinkageUniforme(event->mS02Theta.mX);

    if (VMin<V2 && V2<VMax) {
          const double ti_revalued = xi_current*mSigmaTi.mX  + event->mTheta.mX - mDelta;
        const double r1 = mCalibration->interpolate(xi_current*sqrt(V2)  + event->mTheta.mX - mDelta) / mCalibration->interpolate(ti_revalued);
        rapport =  sqrt(V1/V2) * r1;

    }


    //V2 = Generator::shrinkageUniforme(event->mS02Theta.mX);

    //if (VMin<V2 && V2<VMax) {
    //    rapport = dnorm(xi_current, 0., sqrt(V2)) / dnorm(xi_current, 0., mSigmaTi.mX);
    //}

    mSigmaTi.tryUpdate(sqrt(V2), rapport);

}
*/

// CSV dates
Date Date::fromCSV(const QStringList &dataStr, const QLocale &csvLocale, const StudyPeriodSettings settings)
{
    Date date;
    const QString pluginName = dataStr.first();

    PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
    if (plugin) {
        QStringList dataTmp = dataStr.mid(1,dataStr.size()-1);
        date.setName(dataTmp.at(0));
        date.mPlugin = plugin;
        date.mTi.mSamplerProposal = plugin->getDataMethod();
        date.mData = plugin->fromCSV(dataTmp, csvLocale);

        if (plugin->wiggleAllowed()) {
            qsizetype firstColNum = plugin->csvMinColumns() + plugin->csvOptionalColumns();
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
                        date.mDeltaMin = 0.0;
                        date.mDeltaMax = 0.0;
                        date.mDeltaAverage = 0.0;
                        date.mDeltaError = 0.0;

                    }
                } else {
                    date.mDeltaType = eDeltaNone;
                }
            }

        }
        date.mSettings = settings;
        date.mIsValid = plugin->isDateValid(date.mData, settings);
        date.mUUID = Generator::UUID();
    }

    plugin = nullptr;
    return date;
}

QStringList Date::toCSV(const QLocale &csvLocale) const
{
    QStringList csv;

    csv << mPlugin->getName();
    csv << getQStringName();
    csv << mPlugin->toCSV(mData, csvLocale);

    switch (mDeltaType) {
    case eDeltaNone:
        csv << "none";
        break;
    case eDeltaFixed:
        csv << "fixed";
        csv << csvLocale.toString(mDeltaFixed);
        break;
    case eDeltaRange:
        csv << "range";
        csv << csvLocale.toString(mDeltaMin);
        csv << csvLocale.toString(mDeltaMax);
        break;
    case eDeltaGaussian:
        csv << "gaussian";
        csv << csvLocale.toString(mDeltaAverage);
        csv << csvLocale.toString(mDeltaError);
        break;
    default:

        break;
    }

    return csv;
}

void Date::autoSetTiSampler(const bool bSet)
{
    // define sampling function
    // select if using getLikelyhooArg is possible, it's a faster way

    if (bSet && mPlugin!= nullptr && mPlugin->withLikelihoodArg() && mOrigin == eSingleDate) {
         //   if (false) {
        switch (mTi.mSamplerProposal) {
            case SamplerProposal::eDatePrior:
                updateti = &Date::PriorWithArg;
                break;
            
            case SamplerProposal::eLikelihood:
                updateti = &Date::InversionWithArg;
                break;
            
                // only case with acceptation rate, because we use sigmaMH :
            case SamplerProposal::eRWAdaptGauss: //old version is eMHSymGaussAdapt = 5
                updateti = &Date::MHAdaptGaussWithArg;
                break;

            default:
                updateti = &Date::InversionWithArg;
                break;
            
        }

    } else {
        switch (mTi.mSamplerProposal) {
            case SamplerProposal::eDatePrior:
            updateti = &Date::Prior;//old name fMHSymetric;
                break;
            
            case SamplerProposal::eLikelihood:
                updateti = &Date::Inversion;
                break;
            
                // only case with acceptation rate, because we use sigmaMH :
            case SamplerProposal::eRWAdaptGauss:
                updateti = &Date::MHAdaptGauss;
                break;
            
            default:
                updateti = &Date::Inversion;
                break;
        }
    }
}


CalibrationCurve generate_mixingCalibration(const std::vector<Date> &dates, const std::string description)
{
    CalibrationCurve mixing_calib;
    if (dates.size() == 1) {
        //mixing_calib = dates.at(0).mWiggleCalibration != nullptr ? *dates.at(0).mWiggleCalibration  :  *dates.at(0).mCalibration;
        if (dates.at(0).mDeltaType != Date::eDeltaNone ) {
            mixing_calib = *dates.at(0).mWiggleCalibration;

        } else {
            mixing_calib = *dates.at(0).mCalibration;
        }

        mixing_calib.mDescription = description;
        mixing_calib.mPluginId = "";

    } else {

        mixing_calib.setName(description);
        mixing_calib.mDescription = description;
        mixing_calib.mPluginId = "";

        // 1 - Search for tmin and tmax, distribution curves, identical to the calibration.
        long double unionTmin = +INFINITY;
        long double unionTmax = -INFINITY;
        long double unionStep = INFINITY;

        for (auto&& d : dates) {
            if (d.mDeltaType != Date::eDeltaNone && d.mWiggleCalibration != nullptr && !d.mWiggleCalibration->mVector.empty() ) {
                unionTmin = std::min(unionTmin, (long double)d.mWiggleCalibration->mTmin);
                unionTmax = std::max(unionTmax, (long double)d.mWiggleCalibration->mTmax);
                unionStep = std::min(unionStep, (long double) d.mWiggleCalibration->mStep);

            } else if (d.mCalibration != nullptr && !d.mCalibration->mVector.empty() ) {
                unionTmin = std::min(unionTmin, (long double)d.mCalibration->mTmin);
                unionTmax = std::max(unionTmax, (long double)d.mCalibration->mTmax);
                unionStep = std::min(unionStep, (long double)d.mCalibration->mStep);
            }
        }

        mixing_calib.mTmin = unionTmin;
        mixing_calib.mTmax = unionTmax;
        // Adjust Step
        // We take the smallest step, but it does not necessarily correspond to the same curve with unionTmin and unionTmax.
        int union_N = std::ceil((unionTmax-unionTmin)/unionStep);
        unionStep = (unionTmax-unionTmin)/union_N;
        mixing_calib.mStep = unionStep;

#ifdef DEBUG
        for (auto&& d : dates) {
            qDebug()<< "[Date generate_mixingCalibration] "<< d.getQStringName() << *d.mCalibration->mRepartition.crbegin();
        }
#endif
        // 2 - Creation of the cumulative distribution curves in the interval

        double t = unionTmin;
        long double sum = 0.0;
        long double sum_old = 0.0;
        const double n = dates.size();
        int i = 0;
        while (t < unionTmax) {
            t = unionTmin + i*unionStep;
            sum = 0.0;
            for (auto&& d : dates) {
                if (d.mWiggleCalibration != nullptr)
                    sum += d.mWiggleCalibration->repartition_interpolate(t);
                else
                    sum += d.mCalibration->repartition_interpolate(t);
            }
            mixing_calib.mVector.push_back((sum - sum_old)/(unionStep*n));
            mixing_calib.mRepartition.push_back(sum/n);
            sum_old = sum;
            i++;
        }

        mixing_calib.mMap = vector_to_map(mixing_calib.mVector, unionTmin, unionTmax, unionStep);
    }
    return mixing_calib;
}

#pragma mark SamplingFunction
/**
 * @brief MH proposal = prior distribution
 * @remark done for EDM1
 */
void Date::Prior(const double theta_mX)
{
    const double tiNew = Generator::normalDistribution(theta_mX - mDelta, mSigmaTi.value());
    const double rate = getLikelihood(tiNew) / getLikelihood(mTi.value());

    mTi.try_update(tiNew, rate);

}

void Date::applyPrior(const double theta_mX)
{
    const double tiNew = Generator::normalDistribution(theta_mX - mDelta, mSigmaTi.value());
    const double rate = getLikelihood(tiNew) / getLikelihood(mTi.value());

    if(MHAcceptanceTest(rate))
        mTi.setValue(tiNew);

}
/**
 * @brief MH proposal = prior distribution
 * @brief identic as Prior but use getLikelyhoodArg, when plugin offer it
 * @remark done for EDM1
 */
void Date::PriorWithArg(const double theta_mX)
{
    const double mean  = theta_mX - mDelta;
    const double sigma = mSigmaTi.value();
    assert(sigma > 0.0 && "Sigma must be > 0");

    const double tiNew = Generator::normalDistribution(mean, sigma);
    const auto [oldVariance, oldExponentiel] = getLikelihoodArg(mTi.value());
    const auto [newVariance, newExponentiel] = getLikelihoodArg(tiNew);

    // schoolbook
    //     const long double rate = sqrt(oldVariance / newVariance) * exp(newExponentiel - oldExponentiel);

    const long double logRate =
        0.5L * (std::log(oldVariance) - std::log(newVariance))
        + (newExponentiel - oldExponentiel);

    mTi.try_update_log(tiNew, static_cast<double>(logRate));

}

/**
 *  @brief Calculation of proposal density for time value t
 *  loi mélange de densités calibré + gaussienne
 *
 */
double Date::fProposalDensity(const double t, const double t0)
{
    const double tminCalib = mCalibration->mTmin;
    const double tmaxCalib = mCalibration->mTmax;

    const double s = std::max((mSettings.mTmax - mSettings.mTmin), tmaxCalib - tminCalib) / 2;
    const double q_gaussian = dnorm(t, t0, s);

    // q_calibrate , defined only on Calibration range-----
    // outside study period q_calibrate = 0
    if (t >= tminCalib && t <= tmaxCalib) {
        const double q_calibrate = mCalibration->interpolate(t);
        //const double q_calibrate = getLikelihood(t);

        return (mMixingLevel * q_calibrate + (1 - mMixingLevel) * q_gaussian);
    } 

    return (1 - mMixingLevel) * q_gaussian;
}

/**
 *  @brief MH proposal = Distribution of Calibrated date, t_i is defined on set R (real numbers)
 *  @brief simulation according to uniform shrinkage with s parameter
 */
void Date::Inversion(const double theta)
{
    double ti_prop;
    const double ti_old = mTi.value();

    const double tminCalib = mCalibration->mTmin;
    const double tmaxCalib = mCalibration->mTmax;

   if (Generator::randomUniform() < mMixingLevel) { // tiNew always in the study period

        const double u = Generator::randomUniform();
        const double idx = interpolate_index(u, mCalibration->mRepartition);
        ti_prop = tminCalib + idx * mCalibration->mStep;

    } else {
        // -- gaussian -- tiNew can be outside the study period

        const double s = (std::max((mSettings.mTmax - mSettings.mTmin), tmaxCalib-tminCalib)/2) ;

        ti_prop = Generator::normalDistribution(ti_old, s);
    }

    const double rate_p1 = getLikelihood(ti_prop) / getLikelihood(ti_old);

    const double rate_p2 = exp((-0.5 / (mSigmaTi.value() * mSigmaTi.value())) *
                          (pow(ti_prop - (theta - mDelta), 2) -
                           pow(ti_old - (theta - mDelta), 2))
                          );

    // La loi instrumentale avec mélange de distribution Calibré-Gaussienne, n'est pas symétrique
    // il faut faire le rapport q_xy/q_yx

    const double rate_q = fProposalDensity(ti_old, ti_prop) / fProposalDensity(ti_prop, ti_old);

    mTi.try_update(ti_prop, rate_p1 * rate_p2 * rate_q);

}

void Date::applyInversion(const double theta_mX)
{
    double ti_prop;

    const double tminCalib = mCalibration->mTmin;

    if (Generator::randomUniform() < mMixingLevel) { // tiNew always in the study period
        const double u = Generator::randomUniform();
        const double idx = interpolate_index(u, mCalibration->mRepartition);
        ti_prop = tminCalib + idx * mCalibration->mStep;

    } else {
        // -- gaussian
        const double t0 = mTi.value();
        const double s = (mSettings.mTmax - mSettings.mTmin) / 2.0;

        ti_prop = Generator::normalDistribution(t0, s);
    }

    const double rate_p1 = getLikelihood(ti_prop) / getLikelihood(mTi.value());

    const double rate_p2 = exp((-0.5 / (mSigmaTi.value() * mSigmaTi.value())) *
                              (pow(ti_prop - (theta_mX - mDelta), 2) -
                               pow(mTi.value() - (theta_mX - mDelta), 2))
                              );

    const double rate_q = fProposalDensity(mTi.value(), ti_prop) / fProposalDensity(ti_prop, mTi.value());

    if(MHAcceptanceTest(rate_p1 * rate_p2 * rate_q))
        mTi.setValue(ti_prop);
}

void Date::InversionWithArg(const double theta_mX)
{
    double tiNew;

    const double tminCalib = mCalibration->mTmin;
    const double tmaxCalib = mCalibration->mTmax;

    if (Generator::randomUniform() < mMixingLevel) { // tiNew always in the study period
        const double u = Generator::randomUniform();
        const double idx = vector_interpolate_idx_for_value(u, mCalibration->mRepartition);
        tiNew = tminCalib + idx *mCalibration->mStep;

    } else {
        // -- gaussian
        const double t0 = mTi.value(); //(mSettings.mTmax + mSettings.mTmin) / 2.0;
        const double s = (std::max((mSettings.mTmax - mSettings.mTmin), tmaxCalib-tminCalib)/2) ;//(mSettings.mTmax - mSettings.mTmin) / 2.0;

        tiNew = Generator::normalDistribution(t0, s);

    }

    QPair<long double, long double> argOld, argNew;

    argOld = getLikelihoodArg(mTi.value());
    argNew = getLikelihoodArg(tiNew);

    const long double logG_Rate = argNew.second - argOld.second;
    const long double logH_Rate = (-0.5l/powl(mSigmaTi.value(), 2.)) * (  powl(tiNew - (theta_mX - mDelta), 2.) - powl(mTi.value() - (theta_mX - mDelta), 2.) ); // modif 2020-09-28

    const long double rate_p = sqrt(argOld.first/argNew.first) * exp(logG_Rate + logH_Rate);

    const long double rate_q = fProposalDensity(mTi.value(), tiNew) / fProposalDensity(tiNew, mTi.value());

    mTi.try_update(tiNew, static_cast<double>(rate_p * rate_q));

}


/**
 * @brief Metropolis‑Hastings adaptation with a Gaussian proposal.
 * MH proposal = Adaptatif Gaussian random walk, ti is defined on set R (real numbers)
 * La fonction travaille en log‑probabilité afin d’éviter les under‑/overflow
 * lorsqu’on manipule des rapports très petits ou très grands.
 *
 * @param theta_mX  moyenne du prior (θ) utilisée pour centrer la proposition.
 */
void Date::MHAdaptGauss(const double theta_mX)
{
    /* ------------------------------------------------------------------
     * 1️⃣  Proposition gaussienne autour de la valeur courante
     * ------------------------------------------------------------------ */
    const double tiNew = Generator::normalDistribution(mTi.value(), mTi.mSigmaMH);
    /* ------------------------------------------------------------------
     * 2️⃣  Calcul du log‑rate (log‑acceptance ratio)
     * ------------------------------------------------------------------ */

    // 2.1 – log‑likelihood ratio
    const double likOld = getLikelihood(mTi.value());   // peut être 0 ou négatif ?
    const double likNew = getLikelihood(tiNew);
    // Si l’une des vraisemblances est ≤ 0, le log n’est pas défini → rejet
    double logLikelihoodRatio;
    if (likOld <= 0.0 || likNew <= 0.0) {
        // log(0) = -inf  → rejet systématique
        logLikelihoodRatio = -std::numeric_limits<double>::infinity();
    } else {
        logLikelihoodRatio = std::log(likNew) - std::log(likOld);
    }

    // 2.2 – log‑proposal ratio (gaussian random‑walk)
    const double sigma2 = mSigmaTi.value() * mSigmaTi.value();          // σ²
    const double mu     = theta_mX - mDelta;                 // centre de la loi a priori
    const double diffNew = tiNew - mu;
    const double diffOld = mTi.value() - mu;
    const double logProposalRatio = (-0.5 / sigma2) *
                                    (diffNew * diffNew - diffOld * diffOld);

    // 2.3 – log‑acceptance ratio (log‑rate)
    const double log_rate = logLikelihoodRatio + logProposalRatio;
    /* ------------------------------------------------------------------
     * 3️⃣  Mise à jour de la variable via la fonction log‑rate
     * ------------------------------------------------------------------ */
    // La méthode `try_update_log` accepte déjà le log‑rate.
    // Si vous ne disposez pas encore de cette surcharge, ajoutez‑la
    // (voir la réponse précédente) ou utilisez le wrapper suivant :
    //   mTi.try_update(tiNew, std::exp(log_rate));
    // Ici on utilise directement la version log‑rate.
    mTi.try_update_log(tiNew, log_rate);


}
void Date::applyMHAdaptGauss(const double theta_mX, const double T)
{
    /* ------------------------------------------------------------------
     * 1️⃣  Proposition gaussienne autour de la valeur courante
     * ------------------------------------------------------------------ */
    const double tiNew = Generator::normalDistribution(mTi.value(), mTi.mSigmaMH);
    /* ------------------------------------------------------------------
     * 2️⃣  Calcul du log‑rate (log‑acceptance ratio)
     * ------------------------------------------------------------------ */
    // 2.1 – log‑likelihood ratio
    const double likOld = getLikelihood(mTi.value());   // peut être 0?
    const double likNew = getLikelihood(tiNew);
    // Si l’une des vraisemblances est ≤ 0, le log n’est pas défini → rejet
    double logLikelihoodRatio;
    if (likOld <= 0.0 || likNew <= 0.0) {
        // log(0) = -inf  → rejet systématique
        logLikelihoodRatio = -std::numeric_limits<double>::infinity();
    } else {
        logLikelihoodRatio = std::log(likNew) - std::log(likOld);
    }
    // 2.2 – log‑proposal ratio (gaussian Event)
    const double sigma2 = mSigmaTi.value() * mSigmaTi.value();          // σ²
    const double mu     = theta_mX - mDelta;                 // centre de la loi a priori
    const double diffNew = tiNew - mu;
    const double diffOld = mTi.value() - mu;
    const double logEventRatio = (-0.5 / sigma2) *
                                    (diffNew * diffNew - diffOld * diffOld);
    // 2.3 – log‑acceptance ratio (log‑rate)
    const double log_rate = logLikelihoodRatio + logEventRatio;
    /* ------------------------------------------------------------------
     * 3️⃣  Mise à jour de la variable via la fonction log‑rate
     * ------------------------------------------------------------------ */

    if(MHAcceptanceTest_log(log_rate / T))
        mTi.setValue(tiNew);
}

/**
 * @brief MH proposal = adaptatif Gaussian random walk, ti is not constraint being on the study period
 *
 * @brief identic as fMHSymGaussAdapt but use getLikelyhoodArg, when plugin offer it
 */

void Date::MHAdaptGaussWithArg(const double theta_mX)
{
    /* --------------------------------------------------------------
     * 1️⃣  Proposition gaussienne autour de la valeur courante
     * -------------------------------------------------------------- */
    const double tiNew = Generator::normalDistribution(mTi.value(), mTi.mSigmaMH);
    /* --------------------------------------------------------------
     * 2️⃣  Récupération des arguments de vraisemblance
     *     getLikelihoodArg renvoie (det, logLikelihood)
     * -------------------------------------------------------------- */
    const std::pair<long double,long double> argOld = getLikelihoodArg(mTi.value());
    const std::pair<long double,long double> argNew = getLikelihoodArg(tiNew);
    /* --------------------------------------------------------------
     * 3️⃣  Construction du log‑rate
     * -------------------------------------------------------------- */
    long double log_rate = 0.0L;
    // 3.1 – log‑determinant ratio (0.5*log(old/new))
    if (argOld.first <= 0.0L || argNew.first <= 0.0L) {
        // det ≤ 0 → log indéfini → rejet immédiat (log_rate = -inf)
        log_rate = -std::numeric_limits<long double>::infinity();

    } else {
        log_rate += 0.5L * ( std::log(argOld.first) - std::log(argNew.first) );

        // 3.2 – log‑likelihood ratio
        const long double logG_Rate = argNew.second - argOld.second;
        log_rate += logG_Rate;

        // 3.3 – log‑proposal ratio (gaussian random‑walk)
        const long double sigma2 = mSigmaTi.value() * mSigmaTi.value();   // σ²
        const long double mu     = theta_mX - mDelta;          // centre de la loi a priori
        const long double diffNew = tiNew - mu;
        const long double diffOld = mTi.value() - mu;
        const long double logH_Rate = (-0.5L / sigma2) *  (diffNew*diffNew - diffOld*diffOld);

        // 3.4 – somme de tous les termes
        log_rate += logH_Rate;
    }
    /* --------------------------------------------------------------
     * 4️⃣  Mise à jour de la variable via la fonction log‑rate
     * -------------------------------------------------------------- */
    // `try_update_log` attend le log‑rate (pas le taux lui‑même)
    mTi.try_update_log(tiNew, static_cast<double>(log_rate));

}
