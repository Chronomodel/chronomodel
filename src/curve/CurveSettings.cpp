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

#include "CurveSettings.h"
#include "StateKeys.h"
#include <QDataStream>
#include <QVariant>
#include <QDebug>


CurveSettings::CurveSettings():
//mEnabled(Curve_ENABLED_DEFAULT),
mProcessType(CURVE_PROCESS_TYPE_DEFAULT),
mVariableType(CURVE_VARIABLE_TYPE_DEFAULT),
mUseErrMesure(CURVE_USE_ERR_MESURE_DEFAULT),
mTimeType(CURVE_TIME_TYPE_DEFAULT),
mVarianceType(CURVE_VARIANCE_TYPE_DEFAULT),
mUseVarianceIndividual(CURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT),
mVarianceFixed(CURVE_VARIANCE_FIXED_DEFAULT),
mLambdaSplineType(CURVE_COEFF_LISSAGE_TYPE_DEFAULT),
mLambdaSpline(CURVE_ALPHA_LISSAGE_DEFAULT)
{
    
}

CurveSettings::CurveSettings(const CurveSettings& s)
{
    copyFrom(s);
}

CurveSettings& CurveSettings::operator=(const CurveSettings& s)
{
    copyFrom(s);
    return *this;
}

bool CurveSettings::operator!=(CurveSettings const & s) const
{
    return !isEqual(s);
}

bool CurveSettings::operator==(CurveSettings const & s) const
{
    return isEqual(s);
}

bool CurveSettings::isEqual(const CurveSettings& s) const
{
  //  if (s.mEnabled != mEnabled ||
    if (s.mProcessType != mProcessType ||
        s.mVariableType != mVariableType ||
        s.mUseErrMesure != mUseErrMesure ||
        s.mTimeType != mTimeType ||
        s.mVarianceType != mVarianceType ||
        s.mUseVarianceIndividual != mUseVarianceIndividual ||
        s.mVarianceFixed != mVarianceFixed ||
        s.mLambdaSplineType != mLambdaSplineType ||
        s.mLambdaSpline != mLambdaSpline) {
        return false;
    }
    return true;
}


void CurveSettings::copyFrom(const CurveSettings& s)
{
  //  mEnabled = s.mEnabled;
    mProcessType = s.mProcessType;
    mVariableType = s.mVariableType;
    mUseErrMesure = s.mUseErrMesure;
    mTimeType = s.mTimeType;
    mVarianceType = s.mVarianceType;
    mUseVarianceIndividual = s.mUseVarianceIndividual;
    mVarianceFixed = s.mVarianceFixed;
    mLambdaSplineType = s.mLambdaSplineType;
    mLambdaSpline = s.mLambdaSpline;
}

CurveSettings::~CurveSettings()
{

}

CurveSettings CurveSettings::getDefault()
{
    CurveSettings settings;
    
   // settings.mEnabled = Curve_ENABLED_DEFAULT;
    settings.mProcessType = CURVE_PROCESS_TYPE_DEFAULT;
    settings.mVariableType = CURVE_VARIABLE_TYPE_DEFAULT;
    settings.mUseErrMesure = CURVE_USE_ERR_MESURE_DEFAULT;
    settings.mTimeType = CURVE_TIME_TYPE_DEFAULT;
    settings.mVarianceType = CURVE_VARIANCE_TYPE_DEFAULT;
    settings.mUseVarianceIndividual = CURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT;
    settings.mVarianceFixed = CURVE_VARIANCE_FIXED_DEFAULT;
    settings.mLambdaSplineType = CURVE_COEFF_LISSAGE_TYPE_DEFAULT;
    settings.mLambdaSpline = CURVE_ALPHA_LISSAGE_DEFAULT;
    
    return settings;
}

void CurveSettings::restoreDefault()
{
  //  mEnabled = Curve_ENABLED_DEFAULT;
    mProcessType = CURVE_PROCESS_TYPE_DEFAULT;
    mVariableType = CURVE_VARIABLE_TYPE_DEFAULT;
    mUseErrMesure = CURVE_USE_ERR_MESURE_DEFAULT;
    mTimeType = CURVE_TIME_TYPE_DEFAULT;
    mVarianceType = CURVE_VARIANCE_TYPE_DEFAULT;
    mUseVarianceIndividual = CURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT;
    mVarianceFixed = CURVE_VARIANCE_FIXED_DEFAULT;
    mLambdaSplineType = CURVE_COEFF_LISSAGE_TYPE_DEFAULT;
    mLambdaSpline = CURVE_ALPHA_LISSAGE_DEFAULT;
}

CurveSettings CurveSettings::fromJson(const QJsonObject& json)
{
    CurveSettings settings;
    
  //  settings.mEnabled = json.contains(STATE_Curve_ENABLED) ?  json.value(STATE_Curve_ENABLED).toBool() : Curve_ENABLED_DEFAULT;
    
    settings.mProcessType = json.contains(STATE_CURVE_PROCESS_TYPE) ? CurveSettings::ProcessType (json.value(STATE_CURVE_PROCESS_TYPE).toInt()) : CURVE_PROCESS_TYPE_DEFAULT;
    
    settings.mVariableType = json.contains(STATE_CURVE_VARIABLE_TYPE) ? CurveSettings::VariableType (json.value(STATE_CURVE_VARIABLE_TYPE).toInt()) : CURVE_VARIABLE_TYPE_DEFAULT;
    
    settings.mUseErrMesure = json.contains(STATE_CURVE_USE_ERR_MESURE) ? json.value(STATE_CURVE_USE_ERR_MESURE).toBool() : CURVE_USE_ERR_MESURE_DEFAULT;
    
    settings.mTimeType = json.contains(STATE_CURVE_TIME_TYPE) ? CurveSettings::ProcessMode (json.value(STATE_CURVE_TIME_TYPE).toInt()) : CURVE_TIME_TYPE_DEFAULT;
    
    settings.mVarianceType = json.contains(STATE_CURVE_VARIANCE_TYPE) ? CurveSettings::ProcessMode (json.value(STATE_CURVE_VARIANCE_TYPE).toInt()) : CURVE_VARIANCE_TYPE_DEFAULT;
    
    settings.mUseVarianceIndividual = json.contains(STATE_CURVE_USE_VARIANCE_INDIVIDUAL) ? json.value(STATE_CURVE_USE_VARIANCE_INDIVIDUAL).toBool() : CURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT;
    
    settings.mVarianceFixed = json.contains(STATE_CURVE_VARIANCE_FIXED) ? json.value(STATE_CURVE_VARIANCE_FIXED).toDouble() : CURVE_VARIANCE_FIXED_DEFAULT;
    
    settings.mLambdaSplineType = json.contains(STATE_CURVE_COEFF_LISSAGE_TYPE) ? CurveSettings::ProcessMode (json.value(STATE_CURVE_COEFF_LISSAGE_TYPE).toInt()) : CURVE_COEFF_LISSAGE_TYPE_DEFAULT;
    
    settings.mLambdaSpline = json.contains(STATE_CURVE_ALPHA_LISSAGE) ? json.value(STATE_CURVE_ALPHA_LISSAGE).toDouble() : CURVE_ALPHA_LISSAGE_DEFAULT;
    
    return settings;
}

QJsonObject CurveSettings::toJson() const
{
    QJsonObject mcmc;
    
  //  mcmc[STATE_Curve_ENABLED] = QJsonValue::fromVariant(mEnabled);
    mcmc[STATE_CURVE_PROCESS_TYPE] = QJsonValue::fromVariant((int)mProcessType);
    mcmc[STATE_CURVE_VARIABLE_TYPE] = QJsonValue::fromVariant((int)mVariableType);
    mcmc[STATE_CURVE_USE_ERR_MESURE] = QJsonValue::fromVariant(mUseErrMesure);
    mcmc[STATE_CURVE_TIME_TYPE] = QJsonValue::fromVariant((int)mTimeType);
    mcmc[STATE_CURVE_VARIANCE_TYPE] = QJsonValue::fromVariant((int)mVarianceType);
    mcmc[STATE_CURVE_USE_VARIANCE_INDIVIDUAL] = QJsonValue::fromVariant(mUseVarianceIndividual);
    mcmc[STATE_CURVE_VARIANCE_FIXED] = QJsonValue::fromVariant(mVarianceFixed);
    mcmc[STATE_CURVE_COEFF_LISSAGE_TYPE] = QJsonValue::fromVariant((int)mLambdaSplineType);
    mcmc[STATE_CURVE_ALPHA_LISSAGE] = QJsonValue::fromVariant(mLambdaSpline);
    
    return mcmc;
}

QDataStream &operator<<( QDataStream &stream, const CurveSettings &data )
{
    (void) data;
    //stream << quint8 (data.mNumChains);
    
    return stream;
}

QDataStream &operator>>( QDataStream &stream, CurveSettings &data )
{
    (void) data;
    //quint8 tmp8;
    //stream >> tmp8;
    //data.mNumChains = tmp8;

    return stream;
}

bool CurveSettings::showInclination() const
{
    // Elle est toujours nécessaire en sphérique et vectoriel.
    // En univarié, elle n'est nécessaire que pour les variables d'étude : inclinaison ou déclinaison.
    return ( mProcessType == CurveSettings::eProcessTypeSpherical) ||
             mProcessType == CurveSettings::eProcessType3D ||
            mProcessType == CurveSettings::eProcessType2D ||
            ( mProcessType == CurveSettings::eProcessTypeVector) ||
            ((mProcessType == CurveSettings::eProcessTypeUnivarie) &&
             ( mVariableType == CurveSettings::eVariableTypeInclination ||
               mVariableType == CurveSettings::eVariableTypeDeclination));
}

QString CurveSettings::inclinationLabel() const
{
    QString label;
    if ( (mProcessType == CurveSettings::eProcessTypeUnivarie &&
           ( mVariableType == CurveSettings::eVariableTypeInclination || mVariableType == CurveSettings::eVariableTypeDeclination)) ||
         mProcessType == CurveSettings::eProcessTypeVector ||
         mProcessType == CurveSettings::eProcessTypeSpherical) {
          label = QObject::tr("Inclination");

    } else if (mProcessType == CurveSettings::eProcessType3D || mProcessType == CurveSettings::eProcessType2D) {
        label = QObject::tr("X");

    }
    return label;
}

bool CurveSettings::showDeclination() const
{
    // Elle est toujours nécessaire en sphérique et vectoriel.
    // En univarié, elle n'est nécessaire que pour la variable d'étude : déclinaison.
    return ( mProcessType == CurveSettings::eProcessTypeSpherical ||
             mProcessType == CurveSettings::eProcessType3D ||
             mProcessType == CurveSettings::eProcessType2D ||
             mProcessType == CurveSettings::eProcessTypeVector ||
             ( mProcessType == CurveSettings::eProcessTypeUnivarie &&
               mVariableType == CurveSettings::eVariableTypeDeclination));
}

QString CurveSettings::declinationLabel() const
{
    QString label;
    if ( mProcessType == CurveSettings::eProcessTypeVector ||
         mProcessType == CurveSettings::eProcessTypeSpherical) {
          label = QObject::tr("Declination");

    } else if (mProcessType == CurveSettings::eProcessType2D || mProcessType == CurveSettings::eProcessType3D) {
        label = QObject::tr("Y");

    }
    return label;
}

bool CurveSettings::showIntensity() const
{
    // Elle est toujours nécessaire en vectoriel, mais jamais en sphérique.
    // En univarié, elle n'est nécessaire que pour les variables d'étude autres que inclinaison et déclinaison.
    return ((mProcessType == CurveSettings::eProcessTypeVector) ||
            (mProcessType == CurveSettings::eProcessType3D) ||
            ((mProcessType == CurveSettings::eProcessTypeUnivarie) &&
             (mVariableType != CurveSettings::eVariableTypeInclination) &&
             (mVariableType != CurveSettings::eVariableTypeDeclination)));
}

bool CurveSettings::showYErr() const
{
    // Elle est toujours nécessaire en vectoriel, mais jamais en sphérique.
    // En univarié, elle n'est nécessaire que pour les variables d'étude autres que inclinaison et déclinaison.
    return (mProcessType == CurveSettings::eProcessType2D ||mProcessType == CurveSettings::eProcessType3D) ;
}

QString CurveSettings::intensityLabel() const
{
    QString label;
    if (mProcessType == CurveSettings::eProcessTypeVector) {
        label = QObject::tr("Field");

    } else if (mProcessType == CurveSettings::eProcessType3D) {
        label = QObject::tr("Z");

    } else if (mProcessType == CurveSettings::eProcessTypeUnivarie) {
        if (mVariableType == CurveSettings::eVariableTypeField) {
            label = QObject::tr("Field");

        } else if (mVariableType == CurveSettings::eVariableTypeDepth) {
            label = QObject::tr("Depth");

        } else if (mVariableType == CurveSettings::eVariableTypeOther) {
            label = QObject::tr("Measure");
        }
    }
    return label;
}

QString CurveSettings::processText() const
{
    switch (mProcessType) {
    case eProcessTypeNone:
        return QString("None");
        break;
    case eProcessTypeUnivarie:
        return QString("Univariate");
        break;
    case eProcessType2D:
        return QString("2D");
        break;
    case eProcessTypeSpherical:
        return QString("Spherical");
        break;
    case eProcessTypeVector:
        return QString("Vector");
        break;
    case eProcessType3D:
        return QString("3D");
        break;
    default:
        return QString("Error");
    }
}
