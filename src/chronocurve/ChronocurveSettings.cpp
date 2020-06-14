/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "ChronocurveSettings.h"
#include "StateKeys.h"
#include <QDataStream>
#include <QVariant>


ChronocurveSettings::ChronocurveSettings():
mEnabled(false),
mProcessType(CHRONOCURVE_PROCESS_TYPE_DEFAULT),
mVariableType(CHRONOCURVE_VARIABLE_TYPE_DEFAULT),
mSelectOuv(CHRONOCURVE_SELECT_OUV_DEFAULT),
mOuvMax(CHRONOCURVE_OUV_MAX_DEFAULT),
mUseCorrLat(CHRONOCURVE_USE_CORR_LAT_DEFAULT),
mUseErrMesure(CHRONOCURVE_USE_ERR_MESURE_DEFAULT),
mLat(CHRONOCURVE_LAT_DEFAULT),
mLng(CHRONOCURVE_LNG_DEFAULT),
mTimeType(CHRONOCURVE_TIME_TYPE_DEFAULT),
mVarianceType(CHRONOCURVE_VARIANCE_TYPE_DEFAULT),
mUseVarianceIndividual(CHRONOCURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT),
mVarianceFixed(CHRONOCURVE_VARIANCE_FIXED_DEFAULT),
mCoeffLissageType(CHRONOCURVE_COEFF_LISSAGE_TYPE_DEFAULT),
mAlpha(CHRONOCURVE_ALPHA_DEFAULT)
{
    
}

ChronocurveSettings::ChronocurveSettings(const ChronocurveSettings& s)
{
    copyFrom(s);
}

ChronocurveSettings& ChronocurveSettings::operator=(const ChronocurveSettings& s)
{
    copyFrom(s);
    return *this;
}

void ChronocurveSettings::copyFrom(const ChronocurveSettings& s)
{
    mEnabled = s.mEnabled;
    mProcessType = s.mProcessType;
    mVariableType = s.mVariableType;
    mSelectOuv = s.mSelectOuv;
    mOuvMax = s.mOuvMax;
    mUseCorrLat = s.mUseCorrLat;
    mUseErrMesure = s.mUseErrMesure;
    mLat = s.mLat;
    mLng = s.mLng;
    mTimeType = s.mTimeType;
    mVarianceType = s.mVarianceType;
    mUseVarianceIndividual = s.mUseVarianceIndividual;
    mVarianceFixed = s.mVarianceFixed;
    mCoeffLissageType = s.mCoeffLissageType;
    mAlpha = s.mAlpha;
}

ChronocurveSettings::~ChronocurveSettings()
{

}

ChronocurveSettings ChronocurveSettings::getDefault()
{
    ChronocurveSettings settings;
    
    settings.mEnabled = CHRONOCURVE_ENABLED_DEFAULT;
    settings.mProcessType = CHRONOCURVE_PROCESS_TYPE_DEFAULT;
    settings.mVariableType = CHRONOCURVE_VARIABLE_TYPE_DEFAULT;
    settings.mSelectOuv = CHRONOCURVE_SELECT_OUV_DEFAULT;
    settings.mOuvMax = CHRONOCURVE_OUV_MAX_DEFAULT;
    settings.mUseCorrLat = CHRONOCURVE_USE_CORR_LAT_DEFAULT;
    settings.mUseErrMesure = CHRONOCURVE_USE_ERR_MESURE_DEFAULT;
    settings.mLat = CHRONOCURVE_LAT_DEFAULT;
    settings.mLng = CHRONOCURVE_LNG_DEFAULT;
    settings.mTimeType = CHRONOCURVE_TIME_TYPE_DEFAULT;
    settings.mVarianceType = CHRONOCURVE_VARIANCE_TYPE_DEFAULT;
    settings.mUseVarianceIndividual = CHRONOCURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT;
    settings.mVarianceFixed = CHRONOCURVE_VARIANCE_FIXED_DEFAULT;
    settings.mCoeffLissageType = CHRONOCURVE_COEFF_LISSAGE_TYPE_DEFAULT;
    settings.mAlpha = CHRONOCURVE_ALPHA_DEFAULT;
    
    return settings;
}

void ChronocurveSettings::restoreDefault()
{
    mEnabled = CHRONOCURVE_ENABLED_DEFAULT;
    mProcessType = CHRONOCURVE_PROCESS_TYPE_DEFAULT;
    mVariableType = CHRONOCURVE_VARIABLE_TYPE_DEFAULT;
    mSelectOuv = CHRONOCURVE_SELECT_OUV_DEFAULT;
    mOuvMax = CHRONOCURVE_OUV_MAX_DEFAULT;
    mUseCorrLat = CHRONOCURVE_USE_CORR_LAT_DEFAULT;
    mUseErrMesure = CHRONOCURVE_USE_ERR_MESURE_DEFAULT;
    mLat = CHRONOCURVE_LAT_DEFAULT;
    mLng = CHRONOCURVE_LNG_DEFAULT;
    mTimeType = CHRONOCURVE_TIME_TYPE_DEFAULT;
    mVarianceType = CHRONOCURVE_VARIANCE_TYPE_DEFAULT;
    mUseVarianceIndividual = CHRONOCURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT;
    mVarianceFixed = CHRONOCURVE_VARIANCE_FIXED_DEFAULT;
    mCoeffLissageType = CHRONOCURVE_COEFF_LISSAGE_TYPE_DEFAULT;
    mAlpha = CHRONOCURVE_ALPHA_DEFAULT;
}

ChronocurveSettings ChronocurveSettings::fromJson(const QJsonObject& json)
{
    ChronocurveSettings settings;
    
    settings.mEnabled = json.contains(STATE_CHRONOCURVE_ENABLED) ?  json.value(STATE_CHRONOCURVE_ENABLED).toBool() : CHRONOCURVE_ENABLED_DEFAULT;
    
    settings.mProcessType = json.contains(STATE_CHRONOCURVE_PROCESS_TYPE) ? ChronocurveSettings::ProcessType (json.value(STATE_CHRONOCURVE_PROCESS_TYPE).toInt()) : CHRONOCURVE_PROCESS_TYPE_DEFAULT;
    
    settings.mVariableType = json.contains(STATE_CHRONOCURVE_VARIABLE_TYPE) ? ChronocurveSettings::VariableType (json.value(STATE_CHRONOCURVE_VARIABLE_TYPE).toInt()) : CHRONOCURVE_VARIABLE_TYPE_DEFAULT;
    
    settings.mSelectOuv = json.contains(STATE_CHRONOCURVE_SELECT_OUV) ? json.value(STATE_CHRONOCURVE_SELECT_OUV).toBool() : CHRONOCURVE_SELECT_OUV_DEFAULT;
    
    settings.mOuvMax = json.contains(STATE_CHRONOCURVE_OUV_MAX) ? json.value(STATE_CHRONOCURVE_OUV_MAX).toDouble() : CHRONOCURVE_OUV_MAX_DEFAULT;
    
    settings.mUseCorrLat = json.contains(STATE_CHRONOCURVE_USE_CORR_LAT) ? json.value(STATE_CHRONOCURVE_USE_CORR_LAT).toBool() : CHRONOCURVE_USE_CORR_LAT_DEFAULT;
    
    settings.mLat = json.contains(STATE_CHRONOCURVE_LAT) ? json.value(STATE_CHRONOCURVE_LAT).toDouble() : CHRONOCURVE_LAT_DEFAULT;
    
    settings.mLng = json.contains(STATE_CHRONOCURVE_LNG) ? json.value(STATE_CHRONOCURVE_LNG).toDouble() : CHRONOCURVE_LNG_DEFAULT;
    
    settings.mUseErrMesure = json.contains(STATE_CHRONOCURVE_USE_ERR_MESURE) ? json.value(STATE_CHRONOCURVE_USE_ERR_MESURE).toBool() : CHRONOCURVE_USE_ERR_MESURE_DEFAULT;
    
    settings.mTimeType = json.contains(STATE_CHRONOCURVE_TIME_TYPE) ? ChronocurveSettings::ProcessMode (json.value(STATE_CHRONOCURVE_TIME_TYPE).toInt()) : CHRONOCURVE_TIME_TYPE_DEFAULT;
    
    settings.mVarianceType = json.contains(STATE_CHRONOCURVE_VARIANCE_TYPE) ? ChronocurveSettings::ProcessMode (json.value(STATE_CHRONOCURVE_VARIANCE_TYPE).toInt()) : CHRONOCURVE_VARIANCE_TYPE_DEFAULT;
    
    settings.mUseVarianceIndividual = json.contains(STATE_CHRONOCURVE_USE_VARIANCE_INDIVIDUAL) ? json.value(STATE_CHRONOCURVE_USE_VARIANCE_INDIVIDUAL).toBool() : CHRONOCURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT;
    
    settings.mVarianceFixed = json.contains(STATE_CHRONOCURVE_VARIANCE_FIXED) ? json.value(STATE_CHRONOCURVE_VARIANCE_FIXED).toDouble() : CHRONOCURVE_VARIANCE_FIXED_DEFAULT;
    
    settings.mCoeffLissageType = json.contains(STATE_CHRONOCURVE_COEFF_LISSAGE_TYPE) ? ChronocurveSettings::ProcessMode (json.value(STATE_CHRONOCURVE_COEFF_LISSAGE_TYPE).toInt()) : CHRONOCURVE_COEFF_LISSAGE_TYPE_DEFAULT;
    
    settings.mAlpha = json.contains(STATE_CHRONOCURVE_ALPHA) ? json.value(STATE_CHRONOCURVE_ALPHA).toDouble() : CHRONOCURVE_ALPHA_DEFAULT;
    
    return settings;
}

QJsonObject ChronocurveSettings::toJson() const
{
    QJsonObject mcmc;
    
    mcmc[STATE_CHRONOCURVE_ENABLED] = QJsonValue::fromVariant((int)mEnabled);
    mcmc[STATE_CHRONOCURVE_PROCESS_TYPE] = QJsonValue::fromVariant((int)mProcessType);
    mcmc[STATE_CHRONOCURVE_VARIABLE_TYPE] = QJsonValue::fromVariant((int)mVariableType);
    mcmc[STATE_CHRONOCURVE_SELECT_OUV] = QJsonValue::fromVariant(mSelectOuv);
    mcmc[STATE_CHRONOCURVE_OUV_MAX] = QJsonValue::fromVariant(mOuvMax);
    mcmc[STATE_CHRONOCURVE_USE_CORR_LAT] = QJsonValue::fromVariant(mUseCorrLat);
    mcmc[STATE_CHRONOCURVE_USE_ERR_MESURE] = QJsonValue::fromVariant(mUseErrMesure);
    mcmc[STATE_CHRONOCURVE_LAT] = QJsonValue::fromVariant(mLat);
    mcmc[STATE_CHRONOCURVE_LNG] = QJsonValue::fromVariant(mLng);
    mcmc[STATE_CHRONOCURVE_TIME_TYPE] = QJsonValue::fromVariant((int)mTimeType);
    mcmc[STATE_CHRONOCURVE_VARIANCE_TYPE] = QJsonValue::fromVariant((int)mVarianceType);
    mcmc[STATE_CHRONOCURVE_USE_VARIANCE_INDIVIDUAL] = QJsonValue::fromVariant(mUseVarianceIndividual);
    mcmc[STATE_CHRONOCURVE_VARIANCE_FIXED] = QJsonValue::fromVariant(mVarianceFixed);
    mcmc[STATE_CHRONOCURVE_COEFF_LISSAGE_TYPE] = QJsonValue::fromVariant((int)mCoeffLissageType);
    mcmc[STATE_CHRONOCURVE_ALPHA] = QJsonValue::fromVariant(mAlpha);
    
    return mcmc;
}

QDataStream &operator<<( QDataStream &stream, const ChronocurveSettings &data )
{
    //stream << quint8 (data.mNumChains);
    
    return stream;
}

QDataStream &operator>>( QDataStream &stream, ChronocurveSettings &data )
{
    //quint8 tmp8;
    //stream >> tmp8;
    //data.mNumChains = tmp8;

    return stream;
}
