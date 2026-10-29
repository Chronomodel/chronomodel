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

#ifndef MODELCURVE_H
#define MODELCURVE_H

#include "Model.h"
#include "CurveUtilities.h"
#include "CurveSettings.h"
#include "Matrix.h"
#include "version.h"
//#include "Project.h"

#include <QFile>

extern QString res_file_version;// used when loading

// Avec μ=3, l'écart-type (0.866c) est même plus grand que la moyenne (0.5c)
// — signe d'une distribution très étalée où "la moyenne" n'est pas franchement représentative d'un point typique.
//constexpr int mu_lambda = 3;

class ModelCurve: public std::enable_shared_from_this<ModelCurve>, public Model
{
public:
    CurveSettings mCurveSettings;

    MHVariable mLambdaSpline;
    double mC_lambda;
    double mMu_Lambda;

#ifdef KOMLAN
    MHVariable mS02Vg;
    double mSO2Vg_beta; // used for updated
#else
    double mS02Vg;
#endif

    MCMCSpline mSpline; // valeurs courrantes de la spline

    std::vector<MCMCSpline> mSplinesTrace; // memo des valeurs aux noeuds

    PosteriorMeanG mPosteriorMeanG; // valeurs en tout t
    std::vector<PosteriorMeanG> mPosteriorMeanGByChain; // valeurs en tout t par chaine

    bool compute_Y, compute_XYZ, compute_X_only;
    bool is_curve;

    /**
     * @brief Critère de convergence sur la courbe elle-même : R̂/ESS de G(t_k) sur une
     * grille de points t_k, pour chaque composante calculée, en retenant le pire cas.
     * Brut (échantillonneur) et, si un filtre de courbe est actif, conditionnel à A
     * (R̂_A, ESS_A, excursions) : montre qu'on cible bien la courbe filtrée.
     */
    struct CurveConvergence
    {
        bool   valid = false;
        int    nPoints = 0;               // nombre de points (t_k, composante) évalués
        int    nGrid = 0;                 // nombre d'instants t_k de la grille
        int    nComponents = 0;           // nombre de composantes (X, XY ou XYZ)
        double maxRhat = std::numeric_limits<double>::quiet_NaN();
        double tMaxRhat = std::numeric_limits<double>::quiet_NaN();
        double minBulkESS = std::numeric_limits<double>::quiet_NaN();
        double minTailESS = std::numeric_limits<double>::quiet_NaN();

        bool   filtered = false;
        double p = std::numeric_limits<double>::quiet_NaN(); // proportion de courbes retenues
        int    minExcursions = 0;
        int    nChainsInA = 0;           // chaînes avec au moins 4 courbes retenues
        double maxRhatA = std::numeric_limits<double>::quiet_NaN();
        double tMaxRhatA = std::numeric_limits<double>::quiet_NaN();
        double minBulkESSA = std::numeric_limits<double>::quiet_NaN();
        double minTailESSA = std::numeric_limits<double>::quiet_NaN();
    };
    CurveConvergence mCurveConvergence;
    void generateCurveConvergence(const std::vector<ChainSpecs>& chains, int nGrid = 32);

    ModelCurve();
    explicit ModelCurve(const QJsonObject &json);
    ~ModelCurve() override;

    void setProject();
    void updateDesignFromJson();

    void saveToStream(QDataStream *out) const override;

    bool loadFromStream(QDataStream *in) override
    {
        //std::cout << "[ModelCurve::restoreFromFile] entering";
        static const QSet<QString> compatible_version_400 = {
            QStringLiteral("4.0.0")
        };
        static const QSet<QString> compatible_version_338 = {
            QStringLiteral("3.3.8"),
            QStringLiteral("3.3.9")
        };
        static const QSet<QString> compatible_version_335 = {
            QStringLiteral("3.3.5"),
            QStringLiteral("3.3.6"),
            QStringLiteral("3.3.7")
        };
        static const QSet<QString> compatible_version_330 = {
            QStringLiteral("3.3.0"),
            QStringLiteral("3.3.1")
        };
        static const QSet<QString> compatible_version_328 = {
            QStringLiteral("3.2.4"),
            QStringLiteral("3.2.6"),
            QStringLiteral("3.2.9")
        };

        *in >> res_file_version;

        if (compatible_version_400.contains(res_file_version)) {
            // Les .res 4.0.0 écrits avant l'ajout du compteur d'acceptation
            // n'ont pas le marqueur : format identique à v338
            if (peekResFormatTag(in, kResFormatTag_v400))
                return loadFromStream_v400(in);
            else
                return loadFromStream_v338(in);
        }

        else if (compatible_version_338.contains(res_file_version))
            return loadFromStream_v338(in);

        else if (compatible_version_335.contains(res_file_version))
            return loadFromStream_v335(in);

        else if (compatible_version_330.contains(res_file_version))
            return loadFromStream_v330(in);

        else if (compatible_version_328.contains(res_file_version))
            return loadFromStream_v328(in);

        else {
            std::cout << "[" << __func__ << "] 📚 version not compatible " << res_file_version.toStdString();
            return false;
        }
    };

    bool loadFromStream_v323(QDataStream *in);
    bool loadFromStream_v324(QDataStream *in);
    bool loadFromStream_v328(QDataStream *in);
    bool loadFromStream_v330(QDataStream *in);
    bool loadFromStream_v335(QDataStream *in);

    bool loadFromStream_v338(QDataStream *in);
    bool loadFromStream_v400(QDataStream *in);

    QJsonObject toJson() const override;
    void fromJson( const QJsonObject &json) override;

    virtual void generateModelLog();
    void generateResultsLog() override;

    void generatePosteriorDensities(const std::vector<ChainSpecs> &chains, int fftLen, BandwidthType bwt, double bandwidth = 1.) override;
    void generateCorrelations(const std::vector<ChainSpecs> &chains) override;

    void generateDensityNumericalResults() override;
    void generateTraceNumericalResults(const std::vector<ChainSpecs>& chains) override;

    void generateCredibility(const double thresh) override;
    void generateHPD(const double thresh) override;
    
    void clearThreshold() override;
    void remove_smoothed_densities() override;
    void clearCredibilityAndHPD() override;
    void clearTraces() override;
    void clear();
    void shrink_to_fit() noexcept;
    void clear_and_shrink() noexcept;

    void updateFormatSettings() override;

    void setThreshold(const double threshold) override;
    void setThresholdToAllModel(const double threshold) override;
    
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteX();
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteY();
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteZ();


#if VERSION_MAJOR == 3 && VERSION_MINOR >= 2 && VERSION_PATCH >= 0
    inline void memo_PosteriorG_3D(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted) {
        if (curveType == CurveSettings::eProcess_Vector ||  curveType == CurveSettings::eProcess_Spherical)
            memo_PosteriorG_IDF(postG, spline, curveType, realyAccepted);
        else
            memo_PosteriorG_XYZ(postG, spline, curveType, realyAccepted);
    };
    void memo_PosteriorG_XYZ(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted);
    void memo_PosteriorG_IDF(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted);


#endif

    // G, G', G'' de la spline courante sur la grille en t : évalués une seule fois par itération,
    // puis accumulés dans la moyenne de la chaîne et dans la moyenne totale
    struct GGridComposante {
        std::vector<double> g, gp, gs;
        void resize(const int n) { g.resize(n); gp.resize(n); gs.resize(n); }
    };
    struct GGrid {
        GGridComposante x, y, z;
        void resize(const int n) { x.resize(n); y.resize(n); z.resize(n); }
    };

/*  Versions > 3.3.5
 *  ou Versions >= 4.0.0 (tout numéro majeur > 3)          */
#if (VERSION_MAJOR > 3) || (VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5)
    void memo_PosteriorG_3D_335(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted);
    void eval_PosteriorG_3D_335(GGrid &grid, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int nbPtsX) const;
    void accumulate_PosteriorG_3D_335(PosteriorMeanG &postG, const GGrid &grid, const int realyAccepted);
#endif

    void memo_PosteriorG(PosteriorMeanGComposante &postGCompo, const MCMCSplineComposante &splineComposante, const int realyAccepted);
    void eval_PosteriorG(GGridComposante &grid, const MCMCSplineComposante &splineComposante, const int nbPtsX) const;
    void accumulate_PosteriorG(PosteriorMeanGComposante &postGCompo, const GGridComposante &grid, const int realyAccepted);

    GGrid mGGrid; // tampon réutilisé par memo_PosteriorG*() : pas de réallocation à chaque itération
    void memo_PosteriorG_filtering(PosteriorMeanGComposante &postGCompo, const MCMCSplineComposante &splineComposante, int &realyAccepted_old, const std::pair<double, double> GPfilter);
    bool is_accepted_by_filter(const MCMCSplineComposante& splineComposante, const std::pair<double, double> GPfilter);

    // same as void GraphView::exportReferenceCurves()
    void exportMeanGComposanteToReferenceCurves(const PosteriorMeanGComposante pMeanCompoXYZ, const QString &defaultPath, QLocale csvLocale, const QString &csvSep) const;
    void exportMeanGPComposanteToReferenceCurves(const PosteriorMeanGComposante pMeanCompoXYZ, const QString &defaultPath, QLocale csvLocale, const QString &csvSep) const;

    void exportHpdGComposanteToReferenceCurves(const PosteriorMeanGComposante pMeanCompoXYZ, const QString &defaultPath, QLocale csvLocale, const QString &csvSep) const;
    void exportHpdGPComposanteToReferenceCurves(const PosteriorMeanGComposante pMeanCompoXYZ, const QString &defaultPath, QLocale csvLocale, const QString &csvSep) const;


    std::vector<MCMCSpline> acceptedSplineTraceForAllChain(const std::vector<ChainSpecs> &chains);
    std::vector<MCMCSpline> acceptedSplineTraceForChain(const std::vector<ChainSpecs>& chains, const size_t index);

protected:
    // Ajoute aux variables de Model : Vg des events, lambda (et S02Vg si KOMLAN)
    std::vector<MetropolisVariable*> acquiredVariables() override;
public:

public slots:
    void saveMapToFile(QFile *file, const QString csvSep, const CurveMap &map);
    void saveSplinesToFile(QFile *file, const QString csvSep) const;


#pragma mark Loop
    //void memo_accepted_state(const unsigned i_chain);
    void setParametersForChain() override;

private:
    void settings_from_Json( const QJsonObject &json);

    friend class MCMCLoopCurve;


};

#endif
