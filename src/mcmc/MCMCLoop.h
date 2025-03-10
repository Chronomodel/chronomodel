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

#ifndef MCMCLOOP_H
#define MCMCLOOP_H

#include "CurveSettings.h"
#include "MCMCSettings.h"
#include "ModelCurve.h"

#include <QThread>

#define ABORTED_BY_USER "Aborted by user"

#if PARALLEL
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif


class MCMCLoop : public QThread
{
    Q_OBJECT
public:

    QString mAbortedReason;
    std::shared_ptr<ModelCurve> mModel;

    enum State
    {
        eInit = 0,
        eBurning = 1,
        eAdapting = 2,
        eAquisition = 3
    };


    explicit MCMCLoop (std::shared_ptr<ModelCurve> model);
    virtual ~MCMCLoop();

    void setMCMCSettings(const MCMCSettings& settings);
    const std::vector<ChainSpecs> &chains() const;
    void run();

signals:
    void stepChanged(QString title, int min, int max);
    void stepProgressed(int value);
    void setMessage(QString message);

protected:

    // Variable for update function
    double tminPeriod;
    double tmaxPeriod;
    CurveSettings mCurveSettings;

    virtual QString calibrate() = 0;
    virtual QString initialize() = 0;

    QString initialize_time();
    virtual bool update() = 0;
    virtual void memo() = 0;
    //virtual void memo_accept(const unsigned int i_chain) = 0;
    virtual void finalize() = 0;
    virtual bool adapt(const int batchIndex) = 0;

    std::vector<ChainSpecs> mLoopChains;
    size_t mChainIndex;
    State mState;

};

#endif
