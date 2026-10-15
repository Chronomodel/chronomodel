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
