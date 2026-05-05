/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
    MHVariable::SamplerProposal getDataMethod() const override;
    QList<MHVariable::SamplerProposal> allowedDataMethods() const override;
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
