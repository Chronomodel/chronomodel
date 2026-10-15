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

#ifndef PLUGINTL_H
#define PLUGINTL_H

#if USE_PLUGIN_TL

#include <PluginAbstract.h>

#define DATE_TL_AGE_STR "age"
#define DATE_TL_ERROR_STR "error"
#define DATE_TL_REF_YEAR_STR "ref_year"


class DATATION_SHARED_EXPORT PluginTL : public PluginAbstract
{
    Q_OBJECT
    //Q_PLUGIN_METADATA(IID "chronomodel.PluginAbstract.PluginTL")
    //Q_INTERFACES(PluginAbstract)
public:
    PluginTL();
    virtual ~PluginTL();

    //virtual function
    
    bool withLikelihoodArg() override {return true; }

/**
 * @brief Gaussian likelihood for a TL (tree‑ring) measurement.
 *
 * The likelihood is assumed to be Gaussian:
 *
 * \f[
 *   L(t)=\frac{\exp\!\Bigl(-\tfrac12\,
 *        \bigl(\frac{age-(ref\_year-t)}{error}\bigr)^{2}\Bigr)}{error}
 * \f]
 *
 * All arithmetic is performed in **long double** precision to avoid
 * loss of accuracy when the measurement error is small.
 *
 * @param[in] t    The time (or age) at which the reference year is shifted.
 * @param[in] data JSON object that must contain the keys
 *                 - @c DATE_TL_AGE_STR        – measured age (double)
 *                 - @c DATE_TL_ERROR_STR      – measurement error (double)
 *                 - @c DATE_TL_REF_YEAR_STR   – reference year (double)
 *
 * @return The likelihood value as a @c long double.
 *
 * @note The function is declared @c noexcept because it never throws.
 */
    long double getLikelihood(const double t, const QJsonObject &data) const noexcept override;
    std::pair<long double, long double > getLikelihoodArg(const double t, const QJsonObject &data) const noexcept override;
    QPair<double, double> getTminTmaxRefsCurve(const QJsonObject &data) const override;
    double getMinStepRefsCurve(const QJsonObject &data) override;
    
    bool areDatesMergeable(const QJsonArray &dates) override;
    QJsonObject mergeDates(const QJsonArray &dates) override;

    QString getName() const override;
    QIcon getIcon() const override;
    bool doesCalibration() const override;
    bool wiggleAllowed() const override;
 #ifndef FIXEDPRIOR
    SamplerProposal getDataMethod() const override;
    QList<SamplerProposal> allowedDataMethods() const override;
#endif
    QStringList csvColumns() const override;
    QJsonObject fromCSV(const QStringList &list, const QLocale &csvLocale) const override;
    QStringList toCSV(const QJsonObject &data, const QLocale &csvLocale) const override;
    QString getDateDesc(const Date* date) const override;
    QJsonObject checkValuesCompatibility(const QJsonObject &values) override;

    PluginFormAbstract* getForm() override;
    GraphViewRefAbstract* getGraphViewRef() override;
    void deleteGraphViewRef(GraphViewRefAbstract* graph) override;
    PluginSettingsViewAbstract* getSettingsView() override;
};

#endif
#endif
