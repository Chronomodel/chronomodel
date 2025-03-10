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

#ifndef CURVESETTINGS_H
#define CURVESETTINGS_H

#include <QJsonObject>
#include <QString>

#define CURVE_PROCESS_TYPE_DEFAULT CurveSettings::eProcess_None
#define CURVE_VARIABLE_TYPE_DEFAULT CurveSettings::eVariableTypeInclination
#define CURVE_USE_ERR_MESURE_DEFAULT true
#define CURVE_TIME_TYPE_DEFAULT CurveSettings::eModeBayesian
#define CURVE_VARIANCE_TYPE_DEFAULT CurveSettings::eModeBayesian
#define CURVE_USE_VARIANCE_INDIVIDUAL_DEFAULT true
#define CURVE_VARIANCE_FIXED_DEFAULT 1
#define CURVE_COEFF_LISSAGE_TYPE_DEFAULT CurveSettings::eModeBayesian
#define CURVE_ALPHA_LISSAGE_DEFAULT 0


class CurveSettings
{
public:
    enum ProcessType
    {
        eProcess_None = 'N',
        eProcess_Univariate = 'U',
        eProcess_2D = '2',
        eProcess_3D = '3',

        eProcess_Depth = 'P',

        eProcess_Inclination = 'I',
        eProcess_Declination = 'D',
        eProcess_Field = 'F',

        eProcess_Spherical = 'S',
        eProcess_Unknwon_Dec = 'K',
        eProcess_Vector = 'V'

    };

    enum ProcessMode
    {
        eModeFixed = 'F',
        eModeBayesian = 'B',
        eModeGlobal = 'G',
        eInterpolation = 'I'
    };
    
    CurveSettings();
    CurveSettings(const CurveSettings &s);
    CurveSettings(const QJsonObject &json);

    CurveSettings& operator=(const CurveSettings &s);
    bool operator!=( CurveSettings const &s) const ;
    bool operator==(CurveSettings const &s) const;
   // bool operator==(const CurveSettings& ls, const CurveSettings& rs);
    void copyFrom(const CurveSettings &s);
    bool isEqual(const CurveSettings &s) const ;
    ~CurveSettings();

    static CurveSettings fromJson(const QJsonObject &json);
    static CurveSettings getDefault();
    void restoreDefault();
    QJsonObject toJson() const;
    

    static ProcessType processType_fromJson(const QJsonObject &json);

    bool showX() const;
    bool showY() const;
    bool showZ() const;
    bool showYErr() const;

    QString XLabel() const;
    QString YLabel() const;
    QString ZLabel() const;

    QString X_short_name() const;
    QString Y_short_name() const;
    QString Z_short_name() const;

    QString processText() const;
    
public:
   // bool mEnabled;
    
    ProcessType mProcessType; // Type de traitement
   // VariableType mVariableType; // Type de variable étudiée
    double mThreshold; // Seuil de la vitesse de croissance

    bool mUseErrMesure; // bool_err_mes
    
    ProcessMode mTimeType; // bool_temps_fixes true ou false

    ProcessMode mVarianceType; // bool_Var_G_fixe
    bool mUseVarianceIndividual; // bool_VG_individuelle -> mVarianceType== eModeBayesian ??
    double mVarianceFixed; // Var_G

    ProcessMode mLambdaSplineType;
    double mLambdaSpline;
};

QDataStream &operator<<( QDataStream &stream, const CurveSettings &data );
QDataStream &operator>>( QDataStream &stream, CurveSettings &data );

#endif // endif CurveSettings_H
