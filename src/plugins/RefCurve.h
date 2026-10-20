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

#ifndef REFCURVE_H
#define REFCURVE_H

#include <QString>
#include <QMap>

#include <vector>

class RefCurve
{
public:
    QString mName;

    QMap<double, double> mDataMean;
    QMap<double, double> mDataError;
    QMap<double, double> mDataSup;
    QMap<double, double> mDataInf;

    double mTmin;
    double mTmax;

    double mMinStep;

    double mDataMeanMin;
    double mDataMeanMax;

    double mDataErrorMin;
    double mDataErrorMax;

    double mDataSupMin;
    double mDataSupMax;

    double mDataInfMin;
    double mDataInfMax;

    // Copie "a plat" (tableaux contigus) de mDataMean / mDataError, construite au
    // chargement (PluginAbstract::loadRefDatas). Vide si les deux QMap n'ont pas
    // exactement les memes noeuds : les appelants retombent alors sur le chemin QMap.
    std::vector<double> mFlatT;
    std::vector<double> mFlatMean;
    std::vector<double> mFlatError;

public:
    RefCurve();
    virtual ~RefCurve();
    double interpolate_mean(const double t) const;
    double interpolate_error(const double t) const;

    void buildFlat();
    bool hasFlat() const noexcept { return !mFlatT.empty(); }

    // Valeur et erreur en t, en une seule recherche binaire. Semantique identique a
    // PluginAbstract::getRefCurveValueAt / getRefCurveErrorAt (hors domaine compris).
    // Precondition : hasFlat().
    void meanErrorAt(const double t, double& mean, double& error) const noexcept;


};

#endif
