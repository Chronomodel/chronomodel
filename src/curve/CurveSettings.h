/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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

    ProcessMode mVarianceType; // bool_Var_G_fixe -> eFixed; eModeBayesian; eModeGlobal
   // bool mUseVarianceIndividual; // bool_VG_individuelle -> mVarianceType== eModeBayesian ??
    double mVarianceFixed; // Var_G

    ProcessMode mLambdaSplineType;
    double mLambdaSpline;
};

QDataStream &operator<<( QDataStream &stream, const CurveSettings &data );
QDataStream &operator>>( QDataStream &stream, CurveSettings &data );

#endif // endif CurveSettings_H
