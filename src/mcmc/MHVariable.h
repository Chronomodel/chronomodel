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

#ifndef MHVARIABLE_H
#define MHVARIABLE_H

#include "MetropolisVariable.h"

class MHVariable: public MetropolisVariable
{
public:
    enum SamplerProposal {
        // Event
        eFixe = -1,  //<  use with Type==eBound
        eDoubleExp = 0, //<  The default method for Event->theta
        eBoxMuller = 1,
        eMHAdaptGauss = 2, // also for data
        // Data
        eMHPrior = 3,
        eInversion = 4,
        //eMHSymGaussAdapt = 5
    };

    MHVariable();
    explicit MHVariable(const MHVariable& origin);
    /** move constructor */
    MHVariable(MHVariable&& other) noexcept;

    explicit MHVariable(const MetropolisVariable& origin);
    virtual ~MHVariable();
    void shrink_to_fit() noexcept override;

    void clear() override;
    void clear_and_shrink() noexcept override;

    void remove_smoothed_densities() override;
    void reserve(const size_t reserve) override;
    //MHVariable& copy(MHVariable const& origin);
    MHVariable& operator=(const MHVariable& origin);

    double getCurrentAcceptRate() const;
    void saveCurrentAcceptRate();

    bool tryUpdate(const double x, const double rate);
    bool adapt (const double coef_min = 0.42, const double coef_max = 0.46, const double delta = 0.01);

    inline bool accept_buffer_full() {return mLastAccepts.size() == mLastAcceptsLength;};
    inline void memo_accept(const unsigned i_chain) {if (accept_buffer_full()) ++mAllAccepts[i_chain];} // ??

    std::vector<double> acceptationForChain(const std::vector<ChainSpecs>& chains, size_t index);
    void generateGlobalRunAcceptation(const std::vector<ChainSpecs>& chains);

    void generateNumericalResults(const std::vector<ChainSpecs> &chains) override;
    QString resultsString(const QString &noResultMessage = QObject::tr("No result to display"),
                          const QString &unit = QString()) const override;

    static QString getSamplerProposalText(const MHVariable::SamplerProposal sp);
    static MHVariable::SamplerProposal getSamplerProposalFromText(const QString &text);

public:
    double mSigmaMH;

    // Buffer glissant de la taille d'un batch pour calculer la courbe d'évolution
    // du taux d'acceptation chaine par chaine

    std::vector<bool> mLastAccepts;

    decltype(mLastAccepts.size()) mLastAcceptsLength;

    // Buffer contenant toutes les acceptations cumulées pour toutes les chaines
    // sur les parties acquisition uniquement.
    // A stocker dans le fichier résultats .res !

    std::vector<long long> mAllAccepts;
    // Computed at the end as numerical result :
    double mGlobalAcceptationPerCent;

    // Buffer contenant tous les taux d'acceptation calculés (1 par batch)
    // On en affiche des sous-parties (correspondant aux chaines) dans la vue des résultats
    // A stocker dans les résultats!

    std::shared_ptr<std::vector<double>> mHistoryAcceptRateMH;

    SamplerProposal mSamplerProposal;
    inline void load_stream(QDataStream& stream) {load_stream_v328(stream);};

private:
    void load_stream_v328(QDataStream& stream);
    void load_stream_v327(QDataStream& stream);

};

QDataStream &operator<<( QDataStream &stream, const MHVariable &data );

QDataStream &operator>>( QDataStream &stream, MHVariable &data );

#endif
