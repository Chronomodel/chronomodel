/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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
    mProcessType(CURVE_PROCESS_TYPE_DEFAULT),
    mThreshold(0.),
    mUseErrMesure(CURVE_USE_ERR_MESURE_DEFAULT),
    mTimeType(CURVE_TIME_TYPE_DEFAULT),
    mVarianceType(CURVE_VARIANCE_TYPE_DEFAULT),
    mUseVarianceIndividual(CURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT),
    mVarianceFixed(CURVE_VARIANCE_FIXED_DEFAULT),
    mLambdaSplineType(CURVE_COEFF_LISSAGE_TYPE_DEFAULT),
    mLambdaSpline(CURVE_ALPHA_LISSAGE_DEFAULT)
{
    
}

CurveSettings::CurveSettings(const QJsonObject& json)
{
    mProcessType = json.contains(STATE_CURVE_PROCESS_TYPE) ? CurveSettings::ProcessType (json.value(STATE_CURVE_PROCESS_TYPE).toInt()) : CURVE_PROCESS_TYPE_DEFAULT;

    mThreshold  = json.contains(STATE_CURVE_THRESHOLD) ? json.value(STATE_CURVE_THRESHOLD).toDouble() : 0.;

    mUseErrMesure = json.contains(STATE_CURVE_USE_ERR_MESURE) ? json.value(STATE_CURVE_USE_ERR_MESURE).toBool() : CURVE_USE_ERR_MESURE_DEFAULT;

    mTimeType = json.contains(STATE_CURVE_TIME_TYPE) ? CurveSettings::ProcessMode (json.value(STATE_CURVE_TIME_TYPE).toInt()) : CURVE_TIME_TYPE_DEFAULT;

    mVarianceType = json.contains(STATE_CURVE_VARIANCE_TYPE) ? CurveSettings::ProcessMode (json.value(STATE_CURVE_VARIANCE_TYPE).toInt()) : CURVE_VARIANCE_TYPE_DEFAULT;

    mUseVarianceIndividual = json.contains(STATE_CURVE_USE_VARIANCE_INDIVIDUAL) ? json.value(STATE_CURVE_USE_VARIANCE_INDIVIDUAL).toBool() : CURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT;

    mVarianceFixed = json.contains(STATE_CURVE_VARIANCE_FIXED) ? json.value(STATE_CURVE_VARIANCE_FIXED).toDouble() : CURVE_VARIANCE_FIXED_DEFAULT;

    mLambdaSplineType = json.contains(STATE_CURVE_COEFF_LISSAGE_TYPE) ? CurveSettings::ProcessMode (json.value(STATE_CURVE_COEFF_LISSAGE_TYPE).toInt()) : CURVE_COEFF_LISSAGE_TYPE_DEFAULT;

    mLambdaSpline = json.contains(STATE_CURVE_ALPHA_LISSAGE) ? json.value(STATE_CURVE_ALPHA_LISSAGE).toDouble() : CURVE_ALPHA_LISSAGE_DEFAULT;

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
   if ( s.mProcessType != mProcessType ||
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
    mProcessType = s.mProcessType;
    mThreshold = s.mThreshold;
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
    
    settings.mProcessType = CURVE_PROCESS_TYPE_DEFAULT;
    settings.mThreshold = 0;
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
    mThreshold = 0;
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

    settings.mProcessType = json.contains(STATE_CURVE_PROCESS_TYPE) ? CurveSettings::ProcessType (json.value(STATE_CURVE_PROCESS_TYPE).toInt()) : CURVE_PROCESS_TYPE_DEFAULT;
    
    settings.mThreshold  = json.contains(STATE_CURVE_THRESHOLD) ? json.value(STATE_CURVE_THRESHOLD).toDouble() : 0.;
    
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
    QJsonObject cs;

    cs[STATE_CURVE_PROCESS_TYPE] = QJsonValue::fromVariant((int)mProcessType);
    cs[STATE_CURVE_THRESHOLD] = QJsonValue::fromVariant(mThreshold);
    cs[STATE_CURVE_USE_ERR_MESURE] = QJsonValue::fromVariant(mUseErrMesure);
    cs[STATE_CURVE_TIME_TYPE] = QJsonValue::fromVariant((int)mTimeType);
    cs[STATE_CURVE_VARIANCE_TYPE] = QJsonValue::fromVariant((int)mVarianceType);
    cs[STATE_CURVE_USE_VARIANCE_INDIVIDUAL] = QJsonValue::fromVariant(mUseVarianceIndividual);
    cs[STATE_CURVE_VARIANCE_FIXED] = QJsonValue::fromVariant(mVarianceFixed);
    cs[STATE_CURVE_COEFF_LISSAGE_TYPE] = QJsonValue::fromVariant((int)mLambdaSplineType);
    cs[STATE_CURVE_ALPHA_LISSAGE] = QJsonValue::fromVariant(mLambdaSpline);
    
    return cs;
}

QDataStream &operator<<( QDataStream &stream, const CurveSettings &data )
{
    (void) data;  
    return stream;
}

QDataStream &operator>>( QDataStream &stream, CurveSettings &data )
{
    (void) data;
    return stream;
}


bool CurveSettings::showX() const
{
    return (XLabel()!="");
}

QString CurveSettings::XLabel() const
{
    switch (mProcessType) {
    case eProcess_Univariate:
    case eProcess_2D:
    case eProcess_3D:
          return QObject::tr("X");
          break;

    case eProcess_Depth:
          return QObject::tr("Depth");
          break;

    case eProcess_Inclination:
    case eProcess_Spherical:
    case eProcess_Unknwon_Dec:
    case eProcess_Vector:
          return QObject::tr("Inclination");
          break;

    case eProcess_Declination:
          return QObject::tr("Declination");
          break;

    case eProcess_Field:
          return QObject::tr("Field");
          break;

    case eProcess_None:
    default:
          return QString();
          break;
    }

    return QString();
}

QString CurveSettings::X_short_name() const
{
    switch (mProcessType) {
    case eProcess_Univariate:
    case eProcess_2D:
    case eProcess_3D:
        return QObject::tr("X");
        break;

    case eProcess_Depth:
        return QObject::tr("Depth");
        break;

    case eProcess_Inclination:
    case eProcess_Spherical:
    case eProcess_Unknwon_Dec:
    case eProcess_Vector:
        return QObject::tr("Inc");
        break;

    case eProcess_Declination:
        return QObject::tr("Dec");
        break;

    case eProcess_Field:
        return QObject::tr("F");
        break;

    case eProcess_None:
    default:
        return QString();
        break;
    }

    return QString();
}

bool CurveSettings::showY() const
{
    return (YLabel()!="");
}

QString CurveSettings::YLabel() const
{
    switch (mProcessType) {
        case eProcess_2D:
        case eProcess_3D:
            return QObject::tr("Y");
            break;

        case eProcess_Spherical:
        case eProcess_Vector:
            return QObject::tr("Declination");
            break;

        case eProcess_Unknwon_Dec:
            return QObject::tr("Field");
            break;

        default:
            return QString();
            break;
    }

    return QString();
}

QString CurveSettings::Y_short_name() const
{
    switch (mProcessType) {
        case eProcess_2D:
        case eProcess_3D:
            return QObject::tr("Y");
            break;

        case eProcess_Spherical:
        case eProcess_Vector:
            return QObject::tr("Dec");
            break;

        case eProcess_Unknwon_Dec:
            return QObject::tr("F");
            break;

        default:
            return QString();
            break;
    }

    return QString();
}

bool CurveSettings::showYErr() const
{
    // Elle est toujours nécessaire en vectoriel, mais jamais en sphérique.
    // En univarié, elle n'est nécessaire que pour les variables d'étude autres que inclinaison et déclinaison.
    return ( mProcessType == CurveSettings::eProcess_2D
            || mProcessType == CurveSettings::eProcess_3D) ;
}

bool CurveSettings::showZ() const
{
    return (ZLabel()!="");
}

QString CurveSettings::ZLabel() const
{
    switch (mProcessType) {
        case eProcess_3D:
            return QObject::tr("Z");
            break;

        case eProcess_Vector:
            return QObject::tr("Field");
            break;

        default:
            return QString();
            break;
    }

    return QString();
}

QString CurveSettings::Z_short_name() const
{
    switch (mProcessType) {
        case eProcess_3D:
            return QObject::tr("Z");
            break;

        case eProcess_Vector:
            return QObject::tr("F");
            break;

        default:
            return QString();
            break;
    }

    return QString();
}

QString CurveSettings::processText() const
{
    switch (mProcessType) {
        case eProcess_None:
            return QObject::tr("None");
            break;

        case eProcess_Univariate:
            return QObject::tr("Univariate (1D)");
            break;

        case eProcess_2D:
            return QObject::tr("Bi-variate (2D)");
            break;
        case eProcess_3D:
            return QObject::tr("Tri-variate (3D)");
            break;

        case eProcess_Depth:
            return QObject::tr("Depth");
            break;
        case eProcess_Inclination:
            return QObject::tr("Inclination");
            break;
        case eProcess_Declination:
            return QObject::tr("Declination");
            break;
        case eProcess_Field:
            return QObject::tr("Field Intensity");
            break;

        case eProcess_Spherical:
            return QObject::tr("Spherical (I, D)");
            break;

        case eProcess_Unknwon_Dec:
            return QObject::tr("Unknown Dec (I, F)");
            break;

        case eProcess_Vector:
            return QObject::tr("Vector (I, D, F)");
            break;

        default:
            return QString();
            break;
    }

    return QString();

}
