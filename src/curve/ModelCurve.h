/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

class ModelCurve: public std::enable_shared_from_this<ModelCurve>, public Model
{
public:
    CurveSettings mCurveSettings;

    MHVariable mLambdaSpline;
    double mC_lambda;

    MHVariable mS02Vg;
    double mSO2_beta; // used for updated

    MCMCSpline mSpline; // valeurs courrantes de la spline

    std::vector<MCMCSpline> mSplinesTrace; // memo des valeurs aux noeuds

    PosteriorMeanG mPosteriorMeanG; // valeurs en tout t
    std::vector<PosteriorMeanG> mPosteriorMeanGByChain; // valeurs en tout t par chaine

    bool compute_Y, compute_Z, compute_X_only;
    bool is_curve;

    ModelCurve();
    explicit ModelCurve(const QJsonObject &json);
    virtual ~ModelCurve();

    void setProject();
    void updateDesignFromJson();

    virtual void saveToFile(QDataStream *out);

    virtual bool restoreFromFile(QDataStream *in) {
        //std::cout << "[ModelCurve::restoreFromFile] entering";
        QList<QString> compatible_version_335;
        compatible_version_335 << "3.3.5";

        QList<QString> compatible_version_330;
        compatible_version_330 << "3.3.0" << "3.3.5";

        QList<QString> compatible_version_328;
        compatible_version_328 << "3.2.4" << "3.2.6" << "3.2.9";

        int QDataStreamVersion;
        *in >> QDataStreamVersion;
        in->setVersion(QDataStreamVersion);

        *in >> res_file_version;

        if (compatible_version_335.contains(res_file_version))
            return restoreFromFile_v335(in);

        else if (compatible_version_330.contains(res_file_version))
            return restoreFromFile_v330(in);

        else if (compatible_version_328.contains(res_file_version))
            return restoreFromFile_v328(in);

        else return false;
    };

    bool restoreFromFile_v323(QDataStream *in);
    bool restoreFromFile_v324(QDataStream *in);
    bool restoreFromFile_v328(QDataStream *in);
    bool restoreFromFile_v330(QDataStream *in);
    bool restoreFromFile_v335(QDataStream *in);

    virtual QJsonObject toJson() const;
    virtual void fromJson( const QJsonObject &json);

    virtual void generateModelLog();
    virtual void generateResultsLog();

    void generatePosteriorDensities(const std::vector<ChainSpecs> &chains, int fftLen, double bandwidth);
    virtual void generateCorrelations(const std::vector<ChainSpecs> &chains);
    void generateNumericalResults(const std::vector<ChainSpecs> &chains);
    virtual void generateCredibility(const double thresh);
    void generateHPD(const double thresh);
    
    virtual void clearThreshold();
    virtual void remove_smoothed_densities();
    void clearCredibilityAndHPD();
    void clearTraces();
    void clear();
    void shrink_to_fit() noexcept;
    void clear_and_shrink() noexcept;

    virtual void updateFormatSettings();
    
    virtual void setThresholdToAllModel(const double threshold);
    
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteX();
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteY();
    QList<PosteriorMeanGComposante> getChainsMeanGComposanteZ();

    void memo_PosteriorG_3D(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted);

#if VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
    void memo_PosteriorG_3D_335(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted);
#endif

    void memo_PosteriorG(PosteriorMeanGComposante &postGCompo, const MCMCSplineComposante &splineComposante, const int realyAccepted);
    void memo_PosteriorG_filtering(PosteriorMeanGComposante &postGCompo, const MCMCSplineComposante &splineComposante, int &realyAccepted_old, const std::pair<double, double> GPfilter);
    bool is_accepted_by_filter(const MCMCSplineComposante& splineComposante, const std::pair<double, double> GPfilter);

    // same as void GraphView::exportReferenceCurves()
    void exportMeanGComposanteToReferenceCurves(const PosteriorMeanGComposante pMeanCompoXYZ, const QString &defaultPath, QLocale csvLocale, const QString &csvSep) const;

    std::vector<MCMCSpline> fullRunSplineTrace(const std::vector<ChainSpecs> &chains);
    std::vector<MCMCSpline> runSplineTraceForChain(const std::vector<ChainSpecs>& chains, const size_t index);

public slots:
    void saveMapToFile(QFile *file, const QString csvSep, const CurveMap &map);


#pragma mark Loop
    void memo_accept(const unsigned i_chain);
    void initVariablesForChain();

private:
    void settings_from_Json( const QJsonObject &json);

    void valeurs_G_varG_on_i(const MCMCSplineComposante &spline, double &G, double &varG, unsigned long &i);

    friend class MCMCLoopCurve;


};

#endif
