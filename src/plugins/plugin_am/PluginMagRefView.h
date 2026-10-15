/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE
	Komlan NOUKPOAPE

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

#ifndef PLUGINMAGREFVIEW_H
#define PLUGINMAGREFVIEW_H

#if USE_PLUGIN_AM

#include "GraphViewRefAbstract.h"

class PluginMag;
class GraphView;
class QLocale;


class PluginMagRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit PluginMagRefView(QWidget* parent = 0);
    virtual ~PluginMagRefView();

    void setDate(const Date &d, const StudyPeriodSettings &settings);

public slots:
    void zoomX(const double min, const double max);
    void setMarginRight(const int margin);
protected:
    void resizeEvent(QResizeEvent*);

private:
    RefCurve combine_curve_ID(double incl, double decl, const double alpha95, const RefCurve &curve_I, const RefCurve &curve_D, double &mean_date) const;
    RefCurve combine_curve_IF(const double incl, const double alpha95, const double field, const double error_f, const RefCurve &curve_I, const RefCurve &curve_D, double &mean_date) const;
    RefCurve combine_curve_IDF(double incl, double decl, const double alpha95, double field, const double error_f, const RefCurve &curve_I, const RefCurve &curve_D, const RefCurve &curve_F, double &mean_date) const;

    double compute_mean_ID(double incl, double decl, const double alpha95) const;
};

QMap<double, double> low_pass_filter(QMap<double, double> &map, const double Tc);

double *hanning(int L, int N_fft);
QMap<double, double> window_filter(QMap<double, double> &map, const double L);

QMap<double, double> window_filter_complex(QMap<double, double> &map, const double L);



#endif
#endif
