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

#ifndef MODELUTILITIES_H
#define MODELUTILITIES_H

#include "CalibrationCurve.h"
#include "ModelCurve.h"
#include "Date.h"
#include "Event.h"
#include "Phase.h"

#include <QIcon>
#include <QString>

namespace ModelUtilities
{
    std::vector<std::vector<std::shared_ptr<Event>>> getNextBranches(const std::vector<std::shared_ptr<Event>> &curBranch, std::shared_ptr<Event> lastNode);
    std::vector<std::vector<std::shared_ptr<Event>>> getBranchesFromEvent(std::shared_ptr<Event> start);
    std::vector<std::vector<std::shared_ptr<Event>>> getAllEventsBranches(const std::vector<std::shared_ptr<Event>>& events);

    std::vector<std::vector<Phase *> > getNextBranches(const std::vector<Phase *> &curBranch, const Phase *lastNode, const double gammaSum, const double maxLength);
    std::vector<std::vector<Phase *> > getBranchesFromPhase(Phase *start, const double maxLength);
    std::vector<std::vector<Phase*> > getAllPhasesBranches(const std::vector<std::shared_ptr<Phase> > &events, const double maxLength);

    std::vector<std::shared_ptr<Event>> unsortEvents(const std::vector<std::shared_ptr<Event>> &events);
    QString modelDescriptionHTML(const std::shared_ptr<ModelCurve> model);
    QString getMCMCSettingsLog(const std::shared_ptr<ModelCurve> model = nullptr);
    QString modelStateDescriptionHTML(const std::shared_ptr<ModelCurve> model = nullptr, QString stateDescript = "");
    QString modelStateDescriptionText(const std::shared_ptr<ModelCurve> model = nullptr, QString stateDescript = "");

#pragma mark Results in HTML format
    QString dateResultsHTML(const Date* d, const std::shared_ptr<ModelCurve> &model = nullptr);
    QString dateResultsHTML(const Date* d, const double tmin_formated, const double tmax_formated);
    QString sigmaTiResultsHTML(const Date* d);

    QString eventResultsHTML(const std::shared_ptr<Event> e, const bool withDates, const std::shared_ptr<ModelCurve> model = nullptr);
    QString eventResultsHTML(const std::shared_ptr<Event> e, const bool withDates, const double tmin_formated, const double tmax_formated, bool with_curve = false);
    QString EventS02ResultsHTML(const std::shared_ptr<Event> e);
    QString VgResultsHTML(const std::shared_ptr<Event> e);

    QString phaseResultsHTML(const std::shared_ptr<Phase> p);

    QString tempoResultsHTML(const std::shared_ptr<Phase> p);
    QString activityResultsHTML(const std::shared_ptr<Phase> p);
    QString durationResultsHTML(const std::shared_ptr<Phase> p);
    QString constraintResultsHTML(const std::shared_ptr<PhaseConstraint> p);

    QString curveResultsHTML(const std::shared_ptr<ModelCurve> model = nullptr);
    QString lambdaResultsHTML(const std::shared_ptr<ModelCurve> model = nullptr);
    QString S02VgResultsHTML(const std::shared_ptr<ModelCurve> model  = nullptr);

    short HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const std::shared_ptr<ModelCurve> model);
    short HPDOutsideSudyPeriod(const std::map<double, double> &hpd, const std::shared_ptr<ModelCurve> model);

    short HPDOutsideSudyPeriod(const QMap<double, double> &hpd, const double tmin_formated, const double tmax_formated);
    short HPDOutsideSudyPeriod(const std::map<double, double> &hpd, const double tmin_formated, const double tmax_formated);

}; // namespace ModelUtilities

std::string html_to_plain_text(const std::string &html);
QString html_to_plain_text(const QString &html);


/**
 * @brief  Tire un temps aléatoire dans la répartition, en respectant les
 *         bornes `tMin` / `tMax`.
 *
 * @param calibrateCurve  Courbe de calibration contenant la répartition.
 * @param tMin            Borne inférieure souhaitée (peut être > mTmin).
 * @param tMax            Borne supérieure souhaitée (peut être < mTmax).
 *
 * @return std::optional<double>  Valeur tirée ou `std::nullopt` si la
 *                                 répartition est vide / trop petite.
 *
 * @note   La fonction est `noexcept`.  En cas d’échec d’interpolation,
 *         on retombe sur un tirage uniforme dans `[tMin,tMax]`.
 */
inline std::optional<double> sample_in_repartition(
    const std::shared_ptr<CalibrationCurve>& calibrateCurve,
    double tMin,
    double tMax) noexcept
{
    // -------------------------------------------------------------
    // 0️⃣  Vérifications de base
    // -------------------------------------------------------------
    if (!calibrateCurve || calibrateCurve->mRepartition.empty()
        || calibrateCurve->mRepartition.size() < 2)
        return std::nullopt;                     // rien à faire
    const double unionTmin = calibrateCurve->mTmin;
    const double unionTmax = calibrateCurve->mTmax;
    const std::size_t n   = calibrateCurve->mRepartition.size();
    const double unionStep = (unionTmax - unionTmin) / static_cast<double>(n - 1);
    // -------------------------------------------------------------
    // 1️⃣  Cas où l’intervalle demandé est complètement hors du domaine
    // -------------------------------------------------------------
    if (unionTmax < tMin || tMax < unionTmin) {
        // On renvoie un tirage gaussien centré sur le milieu du domaine,
        // avec un sigma au moins égal à la moitié de la largeur du domaine.
        const double mean  = (unionTmax + unionTmin) * 0.5;
        const double sigma = std::max(unionStep,
                                      (unionTmax - unionTmin) * 0.5);
        return Generator::gaussByDoubleExp(mean, sigma, tMin, tMax);
    }
    // -------------------------------------------------------------
    // 2️⃣  On restreint l’intervalle à l’intersection avec le domaine
    // -------------------------------------------------------------
    const double effectiveMin = std::max(unionTmin, tMin);
    const double effectiveMax = std::min(unionTmax, tMax);
    // Interpolation de la CDF aux deux extrémités de l’intervalle
    const double minRepartition = calibrateCurve->repartition_interpolate(effectiveMin);
    const double maxRepartition = calibrateCurve->repartition_interpolate(effectiveMax);
    // -----------------------------------------------------------------
    // 3️⃣  Gestion d’une zone à densité nulle (ex. entre deux pics)
    // -----------------------------------------------------------------
    if (minRepartition >= maxRepartition) {
        // La densité est nulle sur l’intervalle → on retourne un tirage
        // uniforme direct dans [tMin,tMax].
        return Generator::randomUniform(tMin, tMax);
    }
    // -----------------------------------------------------------------
    // 4️⃣  Tirage d’une valeur de la CDF dans la sous‑portion voulue
    // -----------------------------------------------------------------
    const double value = Generator::randomUniform(minRepartition, maxRepartition);
    // -----------------------------------------------------------------
    // 5️⃣  Conversion CDF → indice (interpolation dans le tableau)
    // -----------------------------------------------------------------
    // On limite la recherche d’indice aux indices qui correspondent à
    // `effectiveMin` et `effectiveMax` afin d’éviter une recherche sur tout
    // le tableau (gain de performances).
    const std::size_t idxInfStart = static_cast<std::size_t>(
        std::floor( (effectiveMin - unionTmin) / unionStep ));
    const std::size_t idxSupStart = static_cast<std::size_t>(
        std::ceil( (effectiveMax - unionTmin) / unionStep ));

    const double idx = interpolate_index_range(value,
                                               calibrateCurve->mRepartition,
                                               idxInfStart,
                                               idxSupStart);


    // -----------------------------------------------------------------
    // 6️⃣  Interpolation temps ↔ indice avec std::lerp
    // -----------------------------------------------------------------
    // `idx` varie de 0 à n‑1 → on le normalise en t ∈ [0,1]
    const double tNorm = idx / static_cast<double>(n - 1);
    const double t     = std::lerp(unionTmin, unionTmax, tNorm);
    // -----------------------------------------------------------------
    // 7️⃣  Validation finale : on s’assure que le résultat est bien dans
    //     l’intervalle demandé.  Si ce n’est pas le cas (arrondis, etc.),
    //     on retombe sur un tirage uniforme.
    // -----------------------------------------------------------------
    if (t >= tMin && t <= tMax)
        return t;
    // (optionnel) : on peut logger le dépassement en mode DEBUG
#ifdef DEBUG
    if (t < tMin)
        qDebug() << "[" << __func__ <<"] t < tMin :" << t << tMin;
    else
        qDebug() << "[" << __func__ <<"] t > tMax :" << t << tMax;
#endif
    return Generator::randomUniform(tMin, tMax);
}

void sampleInCumulatedRepartition_thetaFixe (std::shared_ptr<Event> event, const StudyPeriodSettings &settings);
double sample_in_Repartition_date_fixe(const Date &d, const StudyPeriodSettings& settings);

// These 2 global functions are used to sort events and phases lists in result view

bool sortEvents(std::shared_ptr<Event> e1, std::shared_ptr<Event> e2);
bool sortPhases(std::shared_ptr<Phase> p1, std::shared_ptr<Phase> p2);


#endif
