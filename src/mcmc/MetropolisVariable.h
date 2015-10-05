#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

#include <QMap>
#include <QVector>
#include <QList>
#include "MCMCLoop.h"
#include "Functions.h"
#include "ProjectSettings.h"
#include <QDataStream>

class MetropolisVariable
{
public:
    MetropolisVariable();
    virtual ~MetropolisVariable();
    
    void memo();
    virtual void reset();
    
    // -----
    //  These functions are time consuming!
    // -----
    
    void generateHistos(const QList<Chain>& chains, int fftLen, double hFactor, double tmin, double tmax);
    void generateCorrelations(const QList<Chain>& chains);
    void generateHPD(double threshold);
    void generateCredibility(const QList<Chain>& chains, double threshold);

    void saveToStream(QDataStream *out); // ajout PhD
    void loadFromStream(QDataStream *in); // ajout PhD
    // Virtual because MHVariable subclass adds some information
    virtual void generateNumericalResults(const QList<Chain>& chains);

    // -----
    // These functions do not make any calculation
    // -----
    
    const QMap<double, double>& fullHisto() const;
    const QMap<double, double>& histoForChain(int index) const;
    
    // Full trace for the chain (burn + adapt + run)
    QVector<double> fullTraceForChain(const QList<Chain>& chains, int index);
    
    // Trace for run part as a vector
    QVector<double> fullRunTrace(const QList<Chain>& chains);
    // Trace for run part of the chain as a vector
    QVector<double> runTraceForChain(const QList<Chain>& chains, int index);
    
    QVector<double> correlationForChain(int index);
    
    // -----
    
    virtual QString resultsString(const QString& nl = "<br>",
                                  const QString& noResultMessage = QObject::tr("No result to display"),
                                  const QString& unit = QString(),
                                  FormatFunc formatFunc = 0) const;
    
    QStringList getResultsList(const QLocale locale);
    // -----
    
private:
    float* generateBufferForHisto(QVector<double>& dataSrc, int numPts, double hFactor);
    QMap<double, double> bufferToMap(const double* buffer);
    QMap<double, double> generateHisto(QVector<double>& data, int fftLen, double hFactor, double tmin, double tmax);
    
public:
    double mX;
    QVector<double> mTrace;
    
    // Posterior density results.
    // mHisto is calcuated using all run parts of all chains traces.
    // mChainsHistos constains posterior densities for each chain, computed using only the "run" part of the trace.
    // This needs to be re-calculated each time we change fftLength or HFactor.
    // See generateHistos() for more.
    QMap<double, double> mHisto;
    QList<QMap<double, double> > mChainsHistos;
    
    // List of correlations for each chain.
    // They are calculated once, when the MCMC is ready, from the run part of the trace.
    QList<QVector<double> > mCorrelations;
    
    QMap<double, double> mHPD;
    QPair<double, double> mCredibility;
    double mThreshold;
    
    double mExactCredibilityThreshold;
    
    DensityAnalysis mResults;
    QList<DensityAnalysis> mChainsResults;
    bool mIsDate;
};

#endif
