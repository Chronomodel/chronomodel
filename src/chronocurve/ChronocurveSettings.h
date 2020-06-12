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

#ifndef CHRONOCURVESETTINGS_H
#define CHRONOCURVESETTINGS_H

#include "StateKeys.h"
#include <QJsonObject>

#define CHRONOCURVE_PROCESS_TYPE_DEFAULT ChronocurveSettings::eProcessTypeUnivarie
#define CHRONOCURVE_VARIABLE_TYPE_DEFAULT ChronocurveSettings::eVariableTypeInclinaison
#define CHRONOCURVE_SELECT_OUV_DEFAULT false
#define CHRONOCURVE_OUV_MAX_DEFAULT 0
#define CHRONOCURVE_USE_CORR_LAT_DEFAULT true
#define CHRONOCURVE_LAT_DEFAULT 0
#define CHRONOCURVE_LNG_DEFAULT 0
#define CHRONOCURVE_USE_ERR_MESURE_DEFAULT true
#define CHRONOCURVE_TIME_TYPE_DEFAULT ChronocurveSettings::eModeBayesian
#define CHRONOCURVE_VARIANCE_TYPE_DEFAULT ChronocurveSettings::eModeBayesian
#define CHRONOCURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT true
#define CHRONOCURVE_VARIANCE_FIXED_DEFAULT 1
#define CHRONOCURVE_COEFF_LISSAGE_TYPE_DEFAULT ChronocurveSettings::eModeBayesian
#define CHRONOCURVE_ALPHA_DEFAULT 0


class ChronocurveSettings
{
public:
    enum ProcessType
    {
        eProcessTypeUnivarie = 'U',
        eProcessTypeSpherique = 'S',
        eProcessTypeVectoriel = 'V',
    };

    enum VariableType // Type_var_cmt
    {
        eVariableTypeInclinaison = 'I',
        eVariableTypeDeclinaison = 'D',
        eVariableTypeIntensite = 'F',
        eVariableTypeProfondeur = 'P',
    };

    enum ProcessMode
    {
        eModeFixed = 'F',
        eModeBayesian = 'B',
    };
    
    ChronocurveSettings();
    ChronocurveSettings(const ChronocurveSettings& s);
    ChronocurveSettings& operator=(const ChronocurveSettings& s);
    void copyFrom(const ChronocurveSettings& s);
    ~ChronocurveSettings();

    static ChronocurveSettings fromJson(const QJsonObject& json);
    static ChronocurveSettings getDefault();
    void restoreDefault();
    QJsonObject toJson() const;
    
    
public:
    ProcessType mProcessType; // Type de traitement
    VariableType mVariableType; // Type de variable étudiée
    
    bool mSelectOuv; // bool_select_Ouv
    double mOuvMax;
    bool mUseCorrLat;
    double mLat;
    double mLng;
    
    bool mUseErrMesure; // bool_err_mes
    
    ProcessMode mTimeType; // bool_temps_fixes true ou false

    ProcessMode mVarianceType; // bool_Var_G_fixe
    bool mUseVarianceIndividual; // bool_VG_individuelle
    double mVarianceFixed; // Var_G

    ProcessMode mCoeffLissageType; // bool_Alpha_fixe
    double mAlpha;
};

QDataStream &operator<<( QDataStream &stream, const ChronocurveSettings &data );
QDataStream &operator>>( QDataStream &stream, ChronocurveSettings &data );

#endif // endif ChronocurveSettings_H
