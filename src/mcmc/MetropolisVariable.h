#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

#include <QMap>
#include <QVector>
#include <QList>
#include "MCMCLoop.h"
#include "Functions.h"
#include "ProjectSettings.h"
#include "DateUtils.h"
#include <QDataStream>
#include <QObject>

class MetropolisVariable: public QObject
{
    Q_OBJECT
public:
    MetropolisVariable(QObject *parent = 0);
    virtual ~MetropolisVariable();
    
    void memo();
    virtual void reset();
    MetropolisVariable& copy(MetropolisVariable const& origin);
    MetropolisVariable& operator=( MetropolisVariable const& origin);

    void setFormat(const DateUtils::FormatDate fm);
    QString getName() {return mName;}
    void setName(const QString name) {mName = name;}
    // -----
    //  These functions are time consuming!
    // -----
    void generateCorrelations(const QList<ChainSpecs> &chains);

    void generateHistos(const QList<ChainSpecs> &chains, const int fftLen, const double bandwidth, const double tmin = 0, const double tmax = 0);
    void memoHistoParameter(const int fftLen, const double bandwidth, const double tmin = 0, const double tmax = 0);
    bool HistoWithParameter(const int fftLen, const double bandwidth, const double tmin = 0, const double tmax = 0);

    void generateHPD(const double threshold);
    void generateCredibility(const QList<ChainSpecs>& chains, double threshold);

    void saveToStream(QDataStream *out);
    void loadFromStream(QDataStream *in);
    // Virtual because MHVariable subclass adds some information
    virtual void generateNumericalResults(const QList<ChainSpecs>& chains);

    // -----
    // These functions do not make any calculation
    // -----
    
    QMap<double, double>& fullHisto();
    QMap<double, double>& histoForChain(const int index);
    
    // Full trace for the chain (burn + adapt + run)
    QVector<double> fullTraceForChain(const QList<ChainSpecs> &chains,const int index);
    
    // Trace for run part as a vector
    QVector<double> fullRunTrace(const QList<ChainSpecs>& chains);
    // Trace for run part of the chain as a vector
    QVector<double> runRawTraceForChain(const QList<ChainSpecs>& chains, const int index);
    QVector<double> runFormatedTraceForChain(const QList<ChainSpecs>& chains, const int index);
    
    QVector<double> correlationForChain(const int index);
    
    // -----
    
    virtual QString resultsString(const QString& nl = "<br>",
                                  const QString& noResultMessage = QObject::tr("No result to display"),
                                  const QString& unit = QString(),
                                  FormatFunc formatFunc = 0) const;
    
    QStringList getResultsList(const QLocale locale, const bool withDateFormat = true);
    // -----
    
private:
    //float* generateBufferForHisto(const QVector<double>& dataSrc, int numPts, double a, double b);
    void generateBufferForHisto(float* input, const QVector<double> &dataSrc, const int numPts, const double a, const double b);
    QMap<double, double> bufferToMap(const double* buffer);
    QMap<double, double> generateHisto(const QVector<double>& data, const int fftLen, const  double bandwidth, const double tmin = 0, const double tmax = 0);

protected slots:
    void updateFormatedTrace();

signals:
    void formatChanged();
    
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
    QVector<double> mRawTrace, mFormatedTrace;
    Support mSupport;
    DateUtils::FormatDate mFormat;
    
    // Posterior density results.
    // mHisto is calcuated using all run parts of all chains traces.
    // mChainsHistos constains posterior densities for each chain, computed using only the "run" part of the trace.
    // This needs to be re-calculated each time we change fftLength or bandwidth.
    // See generateHistos() for more.
    QMap<double, double> mHisto;
    QList<QMap<double, double> > mChainsHistos;
    
    // List of correlations for each chain.
    // They are calculated once, when the MCMC is ready, from the run part of the trace.
    QList<QVector<double> > mCorrelations;
    
    QMap<double, double> mHPD;
    QPair<double, double> mCredibility;
    //double mThreshold;
    
    double mExactCredibilityThreshold;
    
    DensityAnalysis mResults;
    QList<DensityAnalysis> mChainsResults;
    //bool mIsDate;


    int mfftLenUsed;
    double mBandwidthUsed;
    double mThresholdUsed;

    double mtminUsed;
    double mtmaxUsed;


private:
    QString mName;
};

#endif
