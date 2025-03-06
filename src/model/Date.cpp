/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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
    mDelta(0.),
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
    _name("No Named Date")
{

    mTi.setName("Ti of Date : " + _name);
    mTi.mSupport = MetropolisVariable::eR;
    mTi.mFormat = DateUtils::eUnknown;
    mTi.mSamplerProposal = MHVariable::eMHPrior;

    mSigmaTi.setName("SigmaTi of Date : " + _name);
    mSigmaTi.mSupport = MetropolisVariable::eRp;
    mSigmaTi.mFormat = DateUtils::eNumeric;
    mSigmaTi.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mWiggle.setName("Wiggle of Date : " + _name);
    mWiggle.mSupport = MetropolisVariable::eR;
    mWiggle.mFormat = DateUtils::eUnknown;

    mId = -1;

    mTminRefCurve = -INFINITY;
    mTmaxRefCurve = INFINITY;

}

Date::Date(const QJsonObject& json):
    mTi(),
    mSigmaTi(),
    mWiggle(),
    mDelta(0.),
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
    _name("No Named Date")
{
    fromJson(json);
    autoSetTiSampler(true); // must be after fromJson()
}

Date::Date(PluginAbstract* plugin):
    mTi(),
    mSigmaTi(),
    mWiggle(),
    mDelta(0.),
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
    _name("No Named Date")
{
}

void Date::init()
{
    mColor = Qt::blue;
    mOrigin = eSingleDate;
    mPlugin = nullptr;

    mTi.setName("Ti of Date : " + _name);
    mTi.mSupport = MetropolisVariable::eR;
    mTi.mFormat = DateUtils::eUnknown;
    mTi.mSamplerProposal = MHVariable::eMHPrior;

    mSigmaTi.setName("SigmaTi of Date : " + _name);
    mSigmaTi.mSupport = MetropolisVariable::eRp;
    mSigmaTi.mFormat = DateUtils::eNumeric;
    mSigmaTi.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mWiggle.setName("Wiggle of Date : " + _name);
    mWiggle.mSupport = MetropolisVariable::eR;
    mWiggle.mFormat = DateUtils::eUnknown;

    mId = -1;
    mUUID = "";

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

    updateti = nullptr;

    mTminRefCurve = -INFINITY;
    mTmaxRefCurve = INFINITY;

    mCalibration = nullptr;
    mWiggleCalibration = nullptr;

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
    _name = other._name;

    // Réinitialiser l'autre objet

    other.mPlugin = nullptr;
    other.mCalibration = nullptr;
    other.mWiggleCalibration = nullptr;
    other.mCalibHPD.clear();
    other._name = "No Named Date";
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
    _name = date._name;

    mTminRefCurve = date.mTminRefCurve;
    mTmaxRefCurve = date.mTmaxRefCurve;
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

    mTi.setName("Theta of date : "+ _name);
    mTi.mFormat = DateUtils::eUnknown;
    mTi.mSamplerProposal = (MHVariable::SamplerProposal)json.value(STATE_DATE_SAMPLER).toInt();

    if ((MHVariable::SamplerProposal)json.value(STATE_DATE_SAMPLER).toInt() == MHVariable::eFixe)
        mSigmaTi.mSamplerProposal = MHVariable::eFixe;
    else
        mSigmaTi.mSamplerProposal = MHVariable::eMHAdaptGauss;
    mSigmaTi.setName("Sigma of date : "+ _name);

    mWiggle.setName("Wiggle of date : "+ _name);

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

    json[STATE_DATE_SAMPLER] = mTi.mSamplerProposal;
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
            
            return 0.l;
        }
    
    } else
         return 0.l;

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

    mCalibration->setName(_name);

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
                    int minIdx = floor(vector_interpolate_idx_for_value(th_min, repartitionTemp));
                    int maxIdx = ceil(vector_interpolate_idx_for_value(th_max, repartitionTemp));


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

                const int minIdx = int (floor(vector_interpolate_idx_for_value(double(threshold * lastRepVal), repartitionTemp)));
                const int maxIdx = int (ceil(vector_interpolate_idx_for_value(double ((1. - threshold) * lastRepVal), repartitionTemp)));

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
    mWiggleCalibration->setName(_name);

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
                
                
        case eDeltaRange:
        {
            /* ----- FFT -----
             * http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
             * https://jperalta.wordpress.com/2006/12/12/using-fftw3/
             * https://dsp.stackexchange.com/questions/22145/perform-convolution-in-frequency-domain-using-fftw
             */

            const int inputSize = (int)calibrationTemp.size();

            const double L = (mDeltaMax-mDeltaMin+1) / mWiggleCalibration->mStep;

            const int gateSize = std::max(inputSize, int(L)) ;
            const int paddingSize = 3*gateSize;

            const int N = inputSize + 4*paddingSize;
            const int NComplex = 2* (N/2)+1;

            double *inputReal;
            inputReal = new double [N];

            fftw_complex *inputComplex;
            inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);

            for (int i = 0; i< paddingSize; i++) {
                inputReal[i] = 0.;
            }
            for (int i = 0; i< inputSize; i++) {
                inputReal[i+paddingSize] = calibrationTemp[i];
            }
            for (int i = inputSize+paddingSize; i< N; i++) {
                inputReal[i] = 0.;
            }
            fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);
            fftw_execute(plan_input);


            // ---- gate
            double *gateReal;
            gateReal = new double [N];

            fftw_complex *gateComplex;
            gateComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);


            for (int i = 0; i< (N-L)/2; i++) {
                gateReal[i] = 0.;
            }
            for (int i = (N-L)/2; i< ((N+L)/2); i++) {
                gateReal[i] = 1.;
            }
            for (int i = (N+L)/2 ; i< N; i++) {
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


            for ( int i = 0; i < N ; i++) {
                curve.push_back(outputReal[i]);
            }

            mWiggleCalibration->mVector = equal_areas(curve, mWiggleCalibration->mStep, 1.);

            mWiggleCalibration->mTmin = minRefCurve - (double)(3*paddingSize +inputSize/2)* mWiggleCalibration->mStep + (mDeltaMin+mDeltaMax)/2.;
            mWiggleCalibration->mTmax = mWiggleCalibration->mTmin + curve.size()* mWiggleCalibration->mStep;

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

    mWiggleCalibration->mRepartition = stretch_vector(mWiggleCalibration->mRepartition, 0., 1.);

    /* ------------------------------------------------------------------
     * Restrict the calib and repartition vectors to where data are
     * ------------------------------------------------------------------ */
    if (*mWiggleCalibration->mRepartition.crbegin() > 0.) {

        const double threshold = threshold_limit;// 0.00001;
        const int minIdx = int (floor(vector_interpolate_idx_for_value(double(threshold * lastRep), mWiggleCalibration->mRepartition)));
        const int maxIdx = int (ceil(vector_interpolate_idx_for_value(double((1. - threshold) * lastRep), mWiggleCalibration->mRepartition)));

        const double tminCal = mWiggleCalibration->mTmin + minIdx * mWiggleCalibration->mStep;
        const double tmaxCal = mWiggleCalibration->mTmin + maxIdx * mWiggleCalibration->mStep;

        // Truncate both functions where data live
        //mWiggleCalibration->mVector = mWiggleCalibration->mVector.mid(minIdx, (maxIdx - minIdx) + 1);
        //mWiggleCalibration->mRepartition = mWiggleCalibration->mRepartition.mid(minIdx, (maxIdx - minIdx) + 1);
        mWiggleCalibration->mVector.assign(mWiggleCalibration->mVector.begin() + minIdx, mWiggleCalibration->mVector.begin()+(maxIdx + 1));
        mWiggleCalibration->mRepartition.assign(mWiggleCalibration->mRepartition.begin()+minIdx, mWiggleCalibration->mRepartition.begin()+(maxIdx + 1));


        // Stretch repartition curve so it goes from 0 to 1
        mWiggleCalibration->mRepartition = stretch_vector(mWiggleCalibration->mRepartition, 0., 1.);

        // Approximation : even if the calib has been truncated, we consider its area to be = 1
        mWiggleCalibration->mVector = equal_areas(mWiggleCalibration->mVector, mWiggleCalibration->mStep, 1.);

        mWiggleCalibration->mTmin = tminCal;
        mWiggleCalibration->mTmax = tmaxCal;

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

    if (*mCalibration->mRepartition.crbegin() > 0.) {
        double tminCal, tmaxCal;
        std::vector<double> curve;
        const double threshold  = 0.01 * (*std::max_element(mCalibration->mVector.begin(), mCalibration->mVector.end()));

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

        //curve = mCalibration->mVector.mid(minIdx, (maxIdx - minIdx) + 1);
        curve.assign(mCalibration->mVector.begin()+minIdx, mCalibration->mVector.begin()+(maxIdx + 1));

        curve = equal_areas(curve, mCalibration->mStep, 1.);
        calib = vector_to_map(curve, tminCal, tmaxCal, mCalibration->mStep );

    } else {
        calib = mCalibration->mMap;
    }

    calib[calib.cbegin()->first - mCalibration->mStep] = 0.;
    calib[calib.crbegin()->first + mCalibration->mStep] = 0.;

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

void Date::generateHistos(const std::vector<ChainSpecs>& chains, const int fftLen, const double bandwidth, const double tmin, const double tmax)
{
    mTi.generateHistos(chains, fftLen, bandwidth, tmin, tmax);
    mSigmaTi.generateHistos(chains, fftLen, bandwidth);

    if ( !( mDeltaType == Date::eDeltaNone ) )
        mWiggle.generateHistos(chains, fftLen, bandwidth);

}

QPixmap Date::generateUnifThumb(StudyPeriodSettings settings)
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

            graph.setXAxisMode(GraphView::eHidden);
            graph.setYAxisMode(GraphView::eHidden);

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
QPixmap Date::generateCalibThumb(StudyPeriodSettings settings)
{
    if (mIsValid) {
        //  No need to draw the graph on a large size
        //  These values are arbitary
        const QSize size(1000, 150);

        const double tmin = settings.mTmin;
        const double tmax = settings.mTmax;

        const std::map<double, double> &calib = normalize_map(getMapDataInRange(getRawCalibMap(), tmin, tmax));

        qDebug()<<"[Date::generateCalibThumb] mName "<< mCalibration->getQStringName() << mCalibration->mVector.size() << calib.size();
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

void Date::updateDate(const double theta_mX, const double S02Theta_mX, const double AShrinkage)
{
    updateDelta(theta_mX);
    updateTi(theta_mX);
    updateSigmaShrinkage(theta_mX, S02Theta_mX, AShrinkage);
    updateWiggle();
}

void Date::updateTi(const double theta_mX)
{
    (this->*updateti) (theta_mX);
}


/**
 * @brief TDate::initDelta Init the wiggle shift
 */
void Date::initDelta()
{
    switch (mDeltaType) {
        case eDeltaNone:
            mDelta = 0.;
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

void Date::updateDelta(const double theta_mX)
{
    const double lambdai = theta_mX - mTi.mX;
    
    switch (mDeltaType) {
        case eDeltaNone:
            mDelta = 0.;
            break;
        
        case eDeltaRange:
            mDelta = Generator::gaussByDoubleExp(lambdai, mSigmaTi.mX, mDeltaMin, mDeltaMax);
            break;
        
        case eDeltaGaussian: {

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

void Date::updateSigmaJeffreys(const double theta_mX)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double lambda = pow(mTi.mX - (theta_mX - mDelta), 2) / 2.0;

    const double a (0.0001); //precision
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

    mSigmaTi.try_update(sqrt(V2), rapport);
}

void Date::updateSigmaShrinkage(const double theta_mX, const double S02Theta_mX, const double AShrinkage)
{
    // ------------------------------------------------------------------------------------------
    //  Echantillonnage MH avec marcheur gaussien adaptatif sur le log de vi (vérifié)
    // ------------------------------------------------------------------------------------------
    const double mu = pow(mTi.mX - (theta_mX - mDelta), 2) / 2.0;

    constexpr int logVMin = -6;
    constexpr int logVMax = 100;

    const double V1 = mSigmaTi.mX * mSigmaTi.mX;
    const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigmaTi.mSigmaMH);
    const double V2 = pow(10, logV2);

    if (logV2 >= logVMin && logV2 <= logVMax) {
        const double x1 = exp(-mu * (V1 - V2) / (V1 * V2));
        const double x2 = pow((S02Theta_mX + V1) / (S02Theta_mX + V2), AShrinkage + 1.0);
        const double rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1 ; // (V2 / V1) est le jacobien!

        mSigmaTi.try_update(sqrt(V2), rapport);

    } else {
       mSigmaTi.reject_update();
 //       qDebug()<<"[TDate::updateSigma] x1 x2 rapport rejet";
    }




}

/*
void Date::updateSigmaShrinkage_K(const Event* event)
{
    const double lambda = pow(mTi.mX - (event->mTheta.mX - mDelta), 2.) / 2.;
    const double V1 = mSigmaTi.mX * mSigmaTi.mX;

    double rapport = -1, V2;
    if (mTi.mSamplerProposal != MHVariable::eMHAdaptGauss) {
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


        const double logV2 = Generator::gaussByBoxMuller(log10(V1), mSigmaTi.mSigmaMH);
        V2 = pow(10, logV2);

        if (logV2 >= logVMin && logV2 <= logVMax) {
            const double x1 = exp(-lambda * (V1 - V2) / (V1 * V2));
            const double x2 = pow((event->mS02Theta.mX + V1) / (event->mS02Theta.mX + V2), event->mAShrinkage + 1.);
            rapport = x1 * sqrt(V1/V2) * x2 * V2 / V1 ; // (V2 / V1) est le jacobien!

        }

    }

    mSigmaTi.tryUpdate(sqrt(V2), rapport);

}

void Date::updateSigmaReParam(const Event *event)
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
        const double x2 = pow((event->mS02Theta.mX + V1) / (event->mS02Theta.mX + V2), event->mAShrinkage + 1.);
        
        rapport = x1 * sqrt(V1/V2) * x2 * pow(V2 / V1, 2.); // (V2 / V1) est le jacobien!

    }
#ifdef DEBUG
    else {
        qDebug()<<"TDate::updateSigmaReParam x1 x2 rapport rejet";
    }
#endif
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
            case MHVariable::eMHPrior:
                updateti = &Date::PriorWithArg;
                break;
            
            case MHVariable::eInversion:
                updateti = &Date::InversionWithArg;
                break;
            
                // only case with acceptation rate, because we use sigmaMH :
            case MHVariable::eMHAdaptGauss: //old version is eMHSymGaussAdapt = 5
                updateti = &Date::MHAdaptGaussWithArg;
                break;

            default:
                break;
            
        }

    } else {
        switch (mTi.mSamplerProposal) {
            case MHVariable::eMHPrior:
            updateti = &Date::Prior;//old name fMHSymetric;
                break;
            
            case MHVariable::eInversion:
                updateti = &Date::Inversion;
                break;
            
                // only case with acceptation rate, because we use sigmaMH :
            case MHVariable::eMHAdaptGauss:
                updateti = &Date::MHAdaptGauss;
                break;
            
            default:
                break;
            }
    }
}


CalibrationCurve generate_mixingCalibration(const std::vector<Date> &dates, const std::string description)
{
    CalibrationCurve mixing_calib;
    if (dates.size() == 1) {
        //mixing_calib = dates.at(0).mWiggleCalibration != nullptr ? *dates.at(0).mWiggleCalibration  :  *dates.at(0).mCalibration;
        if (dates.at(0).mWiggleCalibration != nullptr ) {
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
            if (d.mWiggleCalibration != nullptr && !d.mWiggleCalibration->mVector.empty() ) {
                unionTmin = std::min(unionTmin, (long double)d.mWiggleCalibration->mTmin);
                unionTmax = std::max(unionTmax, (long double)d.mWiggleCalibration->mTmax);
                unionStep = std::min(unionStep,(long double) d.mWiggleCalibration->mStep);

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
            qDebug()<< "generate_mixing"<<d.getQStringName() << *d.mCalibration->mRepartition.crbegin();
        }
#endif
        // 2 - Creation of the cumulative distribution curves in the interval

        double t = unionTmin;
        long double sum = 0.;
        long double sum_old = 0.;
        const double n = dates.size();
        int i = 0;
        while (t < unionTmax) {
            t = unionTmin + i*unionStep;
            sum = 0.;
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
 *
 */
void Date::Prior(const double theta_mX)
{
     // Ici, le marcheur est forcément gaussien avec H(theta i) : double_exp (gaussien tronqué)
    /*   double tmin = date->mSettings.mTmin;
        double tmax = date->mSettings.mTmax;
         double theta = Generator::gaussByDoubleExp(event->mTheta.mX - date->mDelta, date->mSigma.mX, tmin, tmax);
         //rapport = G(theta_new) / G(theta_old)
         double rapport = date->getLikelyhoodFromCalib(theta) / date->getLikelyhoodFromCalib(date->mTheta.mX);
         date->mTheta.tryUpdate(theta, rapport);
    */

    const double tiNew = Generator::gaussByBoxMuller(theta_mX - mDelta, mSigmaTi.mX);
    const double rapport = getLikelihood(tiNew) / getLikelihood(mTi.mX);

    mTi.try_update(tiNew, rapport);
}

/**
 * @brief MH proposal = prior distribution
 * @brief identic as Prior but use getLikelyhoodArg, when plugin offer it
 *
 */
void Date::PriorWithArg(const double theta_mX)
{
    const double tiNew = Generator::gaussByBoxMuller(theta_mX - mDelta, mSigmaTi.mX);

    QPair<long double, long double> argOld, argNew;

    argOld = getLikelihoodArg(mTi.mX);
    argNew = getLikelihoodArg(tiNew);

    const long double rapport = sqrt(argOld.first/argNew.first) * exp(argNew.second-argOld.second);

    mTi.try_update(tiNew, (double)rapport);

}

/**
 *  @brief Calculation of proposal density for time value t
 *
 */
double Date::fProposalDensity(const double t, const double t0)
{
    const double tmin (mSettings.mTmin);
    const double tmax (mSettings.mTmax);
    double q1 (0.);

    const double tminCalib (mCalibration->mTmin);
    const double tmaxCalib (mCalibration->mTmax);

    // ----q1------Defined only on Calibration range-----
    if (t > tminCalib && t < tmaxCalib){

        const double prop = (t - tminCalib) / (tmaxCalib - tminCalib);
        const double idx = prop * (mCalibration->mRepartition.size() - 1);
        const int idxUnder = (int)floor(idx);

       const double step (mCalibration->mStep);

        q1 = (mCalibration->mRepartition[idxUnder+1] - mCalibration->mRepartition[idxUnder])/step;
        
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
    const double sigma = std::max(tmax - tmin, tmaxCalib - tminCalib) / 2;
    const double q2 = exp(-0.5* pow((t-t0)/ sigma, 2)) / (sigma*sqrt(2*M_PI));

    return (mMixingLevel * q1 + (1 - mMixingLevel) * q2);
}

/**
 *  @brief MH proposal = Distribution of Calibrated date, ti is defined on set R (real numbers)
 *  @brief simulation according to uniform shrinkage with s parameter
 */
void Date::Inversion(const double theta_mX)
{
    const double u1 = Generator::randomUniform();
    double tiNew;

    const double tminCalib = mCalibration->mTmin;

    if (u1 < mMixingLevel) { // tiNew always in the study period
        const double idx = vector_interpolate_idx_for_value(u1, mCalibration->mRepartition);
        tiNew = tminCalib + idx * mCalibration->mStep;
    } else {
        // -- gaussian
        const double t0 = mTi.mX;
        const double s = (mSettings.mTmax-mSettings.mTmin)/2.;

        tiNew = Generator::gaussByBoxMuller(t0, s);
    }

    const double rapport1 = getLikelihood(tiNew) / getLikelihood(mTi.mX);

    const double rapport2 = exp((-0.5 / (mSigmaTi.mX * mSigmaTi.mX)) *
                          (pow(tiNew - (theta_mX - mDelta), 2) -
                           pow(mTi.mX - (theta_mX - mDelta), 2))
                          );

    const double rapport3 = fProposalDensity(mTi.mX, tiNew) /
                            fProposalDensity(tiNew, mTi.mX);

    mTi.try_update(tiNew, rapport1 * rapport2 * rapport3);
}

void Date::InversionWithArg(const double theta_mX)
{
    const double u1 = Generator::randomUniform();
    double tiNew;

    const double tminCalib = mCalibration->mTmin;

    if (u1 < mMixingLevel) { // tiNew always in the study period
        const double u2 = Generator::randomUniform();
        const double idx = vector_interpolate_idx_for_value(u2, mCalibration->mRepartition);
        tiNew = tminCalib + idx *mCalibration->mStep;


    } else {
        // -- gaussian
        const double t0 =(mSettings.mTmax + mSettings.mTmin)/2.;
        const double s = (mSettings.mTmax - mSettings.mTmin)/2.;

        tiNew = Generator::gaussByBoxMuller(t0, s);

    }

    QPair<long double, long double> argOld, argNew;

    argOld = getLikelihoodArg(mTi.mX);
    argNew = getLikelihoodArg(tiNew);

    const long double logGRapport = argNew.second - argOld.second;
    const long double logHRapport = (-0.5l/powl(mSigmaTi.mX, 2.)) * (  powl(tiNew - (theta_mX - mDelta), 2.) - powl(mTi.mX - (theta_mX - mDelta), 2.) ); // modif 2020-09-28

    const long double rapport = sqrt(argOld.first/argNew.first) * exp(logGRapport+logHRapport);

    const long double rapportPD = fProposalDensity(mTi.mX, tiNew) / fProposalDensity(tiNew, mTi.mX);

    mTi.try_update(tiNew, (double)(rapport * rapportPD));

}


/**
 * @brief MH proposal = Adaptatif Gaussian random walk, ti is defined on set R (real numbers)
 */
void Date::MHAdaptGauss(const double theta_mX)
{
    const double tiNew = Generator::gaussByBoxMuller(mTi.mX, mTi.mSigmaMH);
    double rapport = getLikelihood(tiNew) / getLikelihood(mTi.mX);
    rapport *= exp((-0.5/(mSigmaTi.mX * mSigmaTi.mX)) * (   pow(tiNew - (theta_mX - mDelta), 2)
                                                           - pow(mTi.mX - (theta_mX - mDelta), 2)
                                                                 ));

    mTi.try_update(tiNew, rapport);
}


/**
 * @brief MH proposal = adaptatif Gaussian random walk, ti is not constraint being on the study period
 *
 * @brief identic as fMHSymGaussAdapt but use getLikelyhoodArg, when plugin offer it
 */
void Date::MHAdaptGaussWithArg(const double theta_mX)//fMHSymGaussAdaptWithArg(Event *event)
{
    const double tiNew = Generator::gaussByBoxMuller(mTi.mX, mTi.mSigmaMH);

    QPair<long double, long double> argOld, argNew;

    argOld = getLikelihoodArg(mTi.mX);
    argNew = getLikelihoodArg(tiNew);

    const long double logGRapport = argNew.second - argOld.second;
    const long double logHRapport = (-0.5 / (mSigmaTi.mX * mSigmaTi.mX)) * (  pow(tiNew - (theta_mX - mDelta), 2)
                                                                      - pow(mTi.mX - (theta_mX - mDelta), 2)
                                                                      );

    const long double rapport = sqrt(argOld.first / argNew.first) * exp(logGRapport+logHRapport);

    mTi.try_update(tiNew, (double) rapport);

}
