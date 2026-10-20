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

#include "RefCurve.h"

#include <algorithm>
#include <cmath>

RefCurve::RefCurve():
    mMinStep (INFINITY)
{

}

RefCurve::~RefCurve()
{

}

double RefCurve::interpolate_mean(const double t) const
{
    if (t >= mTmin && t <= mTmax) {
        // This actually return the iterator with the nearest greater key !!!
        QMap<double, double>::const_iterator iter = mDataMean.lowerBound(t);
        if (iter != mDataMean.constBegin())  {
            const double t_upper = iter.key();
            const double v_upper = iter.value();

            --iter;
            const double t_under = iter.key();
            const double v_under = iter.value();

            return std::lerp(v_under, v_upper, (t - t_under) / (t_upper - t_under));
        }
        else {
            return iter.value();
        }
    }

    return 0.;
}

double RefCurve::interpolate_error(const double t) const
{
    if (t >= mTmin && t <= mTmax) {
        // This actually return the iterator with the nearest greater key !!!
        QMap<double, double>::const_iterator iter = mDataError.lowerBound(t);
        if (iter != mDataError.constBegin())  {
            const double t_upper = iter.key();
            const double v_upper = iter.value();

            --iter;
            const double t_under = iter.key();
            const double v_under = iter.value();

            return std::lerp(v_under, v_upper, (t - t_under) / (t_upper - t_under));
        }
        else {
            return iter.value();
        }
    }

    return 0.;
}

void RefCurve::buildFlat()
{
    mFlatT.clear();
    mFlatMean.clear();
    mFlatError.clear();

    // Les deux courbes doivent partager exactement les memes noeuds
    if (mDataMean.isEmpty() || mDataMean.size() != mDataError.size())
        return;

    const auto n = static_cast<size_t>(mDataMean.size());
    mFlatT.reserve(n);
    mFlatMean.reserve(n);
    mFlatError.reserve(n);

    auto itM = mDataMean.constBegin();
    auto itE = mDataError.constBegin();
    for (; itM != mDataMean.constEnd(); ++itM, ++itE) {
        if (itM.key() != itE.key()) {
            mFlatT.clear();
            mFlatMean.clear();
            mFlatError.clear();
            return;
        }
        mFlatT.push_back(itM.key());
        mFlatMean.push_back(itM.value());
        mFlatError.push_back(itE.value());
    }
}

void RefCurve::meanErrorAt(const double t, double& mean, double& error) const noexcept
{
    // hors domaine : memes valeurs que getRefCurveValueAt / getRefCurveErrorAt
    if (t < mTmin || t > mTmax) {
        mean  = (mDataSupMax + mDataInfMin) * 0.5;
        error = 1.0e+6 * (mDataSupMax - mDataInfMin);
        return;
    }

    // equivalent de QMap::lowerBound : premier noeud de cle >= t
    const auto itUpper = std::lower_bound(mFlatT.cbegin(), mFlatT.cend(), t);
    const size_t k = static_cast<size_t>(itUpper - mFlatT.cbegin());

    if (k == 0) {                       // avant (ou sur) le premier noeud
        mean  = mFlatMean.front();
        error = mFlatError.front();
        return;
    }
    if (k == mFlatT.size()) {           // apres le dernier noeud (exclu par le test de domaine)
        mean  = mFlatMean.back();
        error = mFlatError.back();
        return;
    }

    // meme formule que interpolate() de StdUtilities.h : std::lerp(y0, y1, (x - x0) / (x1 - x0))
    const double tLower = mFlatT[k - 1];
    const double tUpper = mFlatT[k];
    const double u      = (t - tLower) / (tUpper - tLower);
    mean  = std::lerp(mFlatMean[k - 1],  mFlatMean[k],  u);
    error = std::lerp(mFlatError[k - 1], mFlatError[k], u);
}
