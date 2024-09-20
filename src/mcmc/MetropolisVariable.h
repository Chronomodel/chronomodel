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

#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

#include "Functions.h"
#include "DateUtils.h"
#include "MCMCSettings.h"

#include <QMap>
#include <QList>
#include <QDataStream>
#include <QObject>
#include <QList>


class TValueStack
{
public :
    TValueStack();
    explicit TValueStack(double value = 0., std::string comment = "");

    virtual ~TValueStack();

    inline double value() const {return _value;};
    inline std::string comment() const {return _comment;};

protected :
    double _value;
    std::string _comment ;
};

class MetropolisVariable
{
public:
    enum Support
    {
        eR = 0, // on R
        eRp = 1, // on R+
        eRm = 2, // on R-
        eRpStar = 3, // on R+*
        eRmStar = 4, // on R-*
        eBounded = 5 // on bounded support
    };
    double mX;
    std::shared_ptr<std::vector<double>> mRawTrace;
    std::shared_ptr<std::vector<double>> mFormatedTrace;


    // if we use std::vector we can not use QDataStream to save,
    //because QDataStream provides support for multi system and takes account of endians
    Support mSupport;
    DateUtils::FormatDate mFormat;

    // Posterior density results.
    // mFormatedHisto is calculated using all run parts of all chains traces.
    // mChainsHistos constains posterior densities for each chain, computed using only the "run" part of the trace.
    // This needs to be re-calculated each time we change fftLength or bandwidth.
    // See generateHistos() for more.
    std::map<double, double> mFormatedHisto;
    std::vector<std::map<double, double> > mChainsHistos;

    // List of correlations for each chain.
    // They are calculated once, when the MCMC is ready, from the run part of the trace.
    //QList<QList<double> > mCorrelations;
    std::vector<std::vector<double>> mCorrelations;

    std::map<double, double> mFormatedHPD;
    QList<QPair<double, QPair<double, double> > > mRawHPDintervals;

    std::pair<double, double> mRawCredibility;
    std::pair<double, double> mFormatedCredibility;

    double mExactCredibilityThreshold;

    DensityAnalysis mResults;
    std::vector<DensityAnalysis> mChainsResults;

    int mfftLenUsed;
    double mBandwidthUsed;
    double mThresholdUsed;

    double mtminUsed;
    double mtmaxUsed;

public:
    MetropolisVariable();
    explicit MetropolisVariable(const MetropolisVariable& origin);

    virtual ~MetropolisVariable();
    virtual MetropolisVariable& operator=(const MetropolisVariable& origin);
    virtual MetropolisVariable& operator=(MetropolisVariable&& origin) noexcept;

    virtual void memo();
    virtual void memo(double* valueToSave);
    virtual void clear();
    virtual void shrink_to_fit() noexcept;
    virtual void clear_and_shrink() noexcept;

    virtual void remove_smoothed_densities();
    virtual void reserve(const size_t reserve);

    void setFormat(const DateUtils::FormatDate fm);

    inline QString getQStringName() const {return QString::fromStdString(_name);}
    inline std::string name() const {return _name;}
    inline void setName(const std::string name) {_name = name;}
    inline void setName(const QString name) {_name = name.toStdString();}

    // -----
    //  These functions are time consuming!
    // -----
    void generateCorrelations(const std::vector<ChainSpecs> &chains);

    void generateHistos(const std::vector<ChainSpecs> &chains, const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);
    void memoHistoParameter(const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);
    bool HistoWithParameter(const int fftLen = 1024, const double bandwidth = 0.9, const double tmin = 0., const double tmax = 0.);

    void generateHPD(const double threshold = 95);
    void generateCredibility(const std::vector<ChainSpecs> &chains, double threshold = 95.);


    // Virtual because MHVariable subclass adds some information
    virtual void generateNumericalResults(const std::vector<ChainSpecs>& chains);
    void updateFormatedCredibility(const DateUtils::FormatDate fm);


   // QMap<double, double> generateHisto(const QList<double>& data, const int fftLen, const  double bandwidth, const double tmin = 0., const double tmax = 0.);
    std::map<double, double> generateHisto(const std::vector<double> &dataSrc, const int fftLen, const double bandwidth, const double tmin, const double tmax);

    // -----
    // These functions do not make any calculation
    // -----
    std::map<double, double> &fullHisto();
    std::map<double, double> &histoForChain(const size_t index);

    // Full trace for the chain (burn + adapt + run)
    std::vector<double> fullTraceForChain(const std::vector<ChainSpecs> &chains, const size_t index);

    // Trace for run part as a vector
    template <template<typename...> class C, typename T>
    C<T> full_run_trace(C<T>* trace, const std::vector<ChainSpecs>& chains)
    {
        if (trace == nullptr || trace->size() == 0)
            return C<T>(0);

        else if (trace->size() == chains.size()) // Cas des variables fixes
            return C<T>(*trace);

        // Calcul reserve space
        int reserveSize = 0;

        for (const ChainSpecs& chain : chains)
            reserveSize += chain.mRealyAccepted;

        C<T> result(reserveSize);

        int shift = 0;
        int shiftTrace = 0;

        for (const ChainSpecs& chain : chains) {
            // we add 1 for the init
            const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
            const int runTraceSize = chain.mRealyAccepted;
            const int firstRunPosition = shift + burnAdaptSize;
            std::copy(trace->begin() + firstRunPosition , trace->begin() + firstRunPosition + runTraceSize , result.begin() + shiftTrace);

            shiftTrace += runTraceSize;
            shift = firstRunPosition +runTraceSize;
        }
        return result;
    }
/*
    template <typename T>
    QList<T> full_run_trace(std::vector<T>* trace, const std::vector<ChainSpecs>& chains)
    {
        if (trace == nullptr || trace->size() == 0)
            return QList<T>(0);

        else if (trace->size() == chains.size()) // Cas des variables fixes
            return QList<T>(trace->begin(), trace->end());

        // Calcul reserve space
        int reserveSize = 0;

        for (const ChainSpecs& chain : chains)
            reserveSize += chain.mRealyAccepted;

        QList<T> result(reserveSize);

        int shift = 0;
        int shiftTrace = 0;

        for (const ChainSpecs& chain : chains) {
            // we add 1 for the init
            const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
            const int runTraceSize = chain.mRealyAccepted;
            const int firstRunPosition = shift + burnAdaptSize;
            std::copy(trace->begin() + firstRunPosition , trace->begin() + firstRunPosition + runTraceSize , result.begin() + shiftTrace);

            shiftTrace += runTraceSize;
            shift = firstRunPosition +runTraceSize;
        }
        return result;
    }
*/

    template <typename T>
    std::vector<T> full_run_trace(std::shared_ptr<std::vector<T>> trace, const std::vector<ChainSpecs>& chains)
    {
        if (trace == nullptr || trace->size() == 0)
            return std::vector<T>(0);

        else if (trace->size() == chains.size()) // Cas des variables fixes
            return std::vector<T>(trace->begin(), trace->end());

        // Calcul reserve space
        int reserveSize = 0;

        for (const ChainSpecs& chain : chains)
            reserveSize += chain.mRealyAccepted;

        std::vector<T> result(reserveSize);

        int shift = 0;
        int shiftTrace = 0;

        for (const ChainSpecs& chain : chains) {
            // we add 1 for the init
            const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
            const int runTraceSize = chain.mRealyAccepted;
            const int firstRunPosition = shift + burnAdaptSize;
            std::copy(trace->begin() + firstRunPosition , trace->begin() + firstRunPosition + runTraceSize , result.begin() + shiftTrace);

            shiftTrace += runTraceSize;
            shift = firstRunPosition +runTraceSize;
        }
        return result;
    }

    inline std::vector<double> fullRunFormatedTrace(const std::vector<ChainSpecs>& chains) {return full_run_trace(mFormatedTrace, chains);}
    inline std::vector<double> fullRunRawTrace(const std::vector<ChainSpecs>& chains) {return full_run_trace(mRawTrace, chains);}

    std::vector<double>::iterator findIter_element(const long unsigned iter, const std::vector<ChainSpecs>& chains, const size_t index ) const;

    // Trace for run part of the chain as a vector

    template <template<typename...> class C, typename T>
    C<T> run_trace_for_chain(C<T>* trace, const std::vector<ChainSpecs>& chains, const size_t index) {

        if (!trace || trace->size() == 0) {
            return C<T>(0);

        } else if (trace->size() == 1) { // Cas des variables fixes
            return C<T>(*trace);

        } else  {

            int shift = 0;
            for (size_t i = 0; i<chains.size(); ++i)  {
                const ChainSpecs& chain = chains.at(i);
                // We add 1 for the init
                const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
                const int traceSize = chain.mRealyAccepted;

                if (i == index) {
                    return C<T> (trace->begin() + shift + burnAdaptSize, trace->begin() + shift + burnAdaptSize + traceSize );
                    break;
                }
                shift += traceSize + burnAdaptSize ;
            }
            return C<T>(0);
        }
    }
    template <template<typename...> class C, typename T>
    C<T> run_trace_for_chain(std::shared_ptr<C<T>> trace, const std::vector<ChainSpecs>& chains, const size_t index) {

        if (!trace || trace->size() == 0) {
            return C<T>(0);

        } else if (trace->size() == 1) { // Cas des variables fixes
            return C<T>(*trace);

        } else  {

            int shift = 0;
            for (size_t i = 0; i<chains.size(); ++i)  {
                const ChainSpecs& chain = chains.at(i);
                // We add 1 for the init
                const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
                const int traceSize = chain.mRealyAccepted;

                if (i == index) {
                    return C<T> (trace->begin() + shift + burnAdaptSize, trace->begin() + shift + burnAdaptSize + traceSize );
                    break;
                }
                shift += traceSize + burnAdaptSize ;
            }
            return C<T>(0);
        }
    }

    inline std::vector<double> runRawTraceForChain(const std::vector<ChainSpecs>& chains, const size_t index) {
        const std::vector<double> &trace = run_trace_for_chain(mRawTrace, chains, index);
        return std::vector<double>(trace.begin(), trace.end());
    };


    inline std::vector<double> runFormatedTraceForChain(const std::vector<ChainSpecs>& chains, const size_t index) {
        const std::vector<double> &trace = run_trace_for_chain(mFormatedTrace, chains, index);
        return std::vector<double>(trace.begin(), trace.end());
    };

    std::vector<double> correlationForChain(const size_t index);

    virtual QString resultsString(const QString& noResultMessage = QObject::tr("No result to display"),
                                  const QString& unit = QString()) const;

    QStringList getResultsList(const QLocale locale, const int precision = 0, const bool withDateFormat = true) const;

    void updateFormatedTrace(const DateUtils::FormatDate fm);

    inline void load_stream(QDataStream& stream) {load_stream_v328(stream);}

private:

    std::string _name;

    void generateBufferForHisto(double* input, const std::vector<double> &dataSrc, const int numPts, const double a, const double b);
    QMap<double, double> bufferToMap(const double* buffer);

    void load_stream_v328(QDataStream& stream);
    //void load_stream_v327(QDataStream& stream);


friend class MHVariable;
};

QDataStream &operator<<( QDataStream& stream, const MetropolisVariable& data );

QDataStream &operator>>( QDataStream& stream, MetropolisVariable& data );
#endif
