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

#ifndef BOUND_H
#define BOUND_H

#include "Event.h"

class Bound: public Event
{
private:
    double mFixed;

public:

    Bound();
    Bound(const Bound& origin);
    virtual ~Bound();

    explicit Bound(const QJsonObject& json);

    virtual Bound& operator=(const Bound& origin);
    /** Move assignment operator */
    virtual Bound& operator=(Bound&& origin) noexcept;

    static const Bound fromJson(const QJsonObject& json);
    virtual QJsonObject toJson() const;

    inline void setValue(const double& value) {mFixed = value;}
    inline double value() const {return mFixed;}
    double formatedFixedValue() const;

    inline virtual void updateTheta(const double, const double)
    {
        mTheta.accept_update(mFixed);
    }

    inline virtual void applyTheta(const double , const double, const double )
    {
        mTheta.setValue(mFixed);
    }




};

#endif
