/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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
    enum SamplerProposal{
        // Event
        eFixe = -1,  //<  use with Type==eKnown
        eDoubleExp = 0, //<  The default method for Event->theta
        eBoxMuller = 1,
        eMHAdaptGauss = 2,
        // Data
        eMHSymetric = 3,
        eInversion = 4,
        eMHSymGaussAdapt = 5
    };

    MHVariable();
    explicit MHVariable(const MHVariable &origin);
    virtual ~MHVariable();

    virtual void reset();
    virtual void reserve( const int reserve);
    MHVariable& copy(MHVariable const& origin);
    MHVariable& operator=(MHVariable const& origin);

    double getCurrentAcceptRate() const;
    void saveCurrentAcceptRate();

    bool tryUpdate(const double x, const double rapportToTry);
    bool adapt (const double coef_min = 0.42, const double coef_max = 0.46, const double delta = 0.01);

    QVector<double> acceptationForChain(const QList<ChainSpecs>& chains, int index);
    void generateGlobalRunAcceptation(const QList<ChainSpecs>& chains);

    void generateNumericalResults(const QList<ChainSpecs>& chains);
    QString resultsString(const QString& nl = "<br>",
                          const QString& noResultMessage = QObject::tr("No result to display"),
                          const QString& unit = QString(),
                          DateConversion formatFunc = nullptr, const bool forCSV = false) const;

    static QString getSamplerProposalText(const MHVariable::SamplerProposal sp);
    static MHVariable::SamplerProposal getSamplerProposalFromText(const QString& text);

public:
    double mSigmaMH;

    // Buffer glissant de la taille d'un batch pour calculer la courbe d'évolution
    // du taux d'acceptation chaine par chaine

    QVector<bool> mLastAccepts;

    int mLastAcceptsLength;

    // Buffer contenant toutes les acceptations cumulées pour toutes les chaines
    // sur les parties acquisition uniquement.
    // A stocker dans le fichier résultats .res !

    QVector<bool>* mAllAccepts;

    // Computed at the end as numerical result :
    double mGlobalAcceptation;

    // Buffer contenant tous les taux d'acceptation calculés (1 par batch)
    // On en affiche des sous-parties (correspondant aux chaines) dans la vue des résultats
    // A stocker dans les résultats!

    QVector<double>* mHistoryAcceptRateMH;

    SamplerProposal mSamplerProposal;

};

QDataStream &operator<<( QDataStream &stream, const MHVariable &data );

QDataStream &operator>>( QDataStream &stream, MHVariable &data );

#endif
