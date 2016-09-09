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

    void generateHistos(const QList<ChainSpecs> &chains, const int fftLen = 1024, const float bandwidth = 1.06, const float tmin = 0, const float tmax = 0);
    void memoHistoParameter(const int fftLen = 1024, const float bandwidth = 1.06, const float tmin = 0, const float tmax = 0);
    bool HistoWithParameter(const int fftLen = 1024, const float bandwidth = 1.06, const float tmin = 0, const float tmax = 0);

    void generateHPD(const float threshold = 95);
    void generateCredibility(const QList<ChainSpecs>& chains, float threshold = 95);


    // Virtual because MHVariable subclass adds some information
    virtual void generateNumericalResults(const QList<ChainSpecs>& chains);

    // -----
    // These functions do not make any calculation
    // -----
    
    QMap<float, float>& fullHisto();
    QMap<float, float>& histoForChain(const int index);
    
    // Full trace for the chain (burn + adapt + run)
    QVector<float> fullTraceForChain(const QList<ChainSpecs> &chains,const int index);
    
    // Trace for run part as a vector
    QVector<float> fullRunTrace(const QList<ChainSpecs>& chains);
    // Trace for run part of the chain as a vector
    QVector<float> runRawTraceForChain(const QList<ChainSpecs>& chains, const int index);
    QVector<float> runFormatedTraceForChain(const QList<ChainSpecs>& chains, const int index);
    
    QVector<float> correlationForChain(const int index);
    
    // -----
    
    virtual QString resultsString(const QString& nl = "<br>",
                                  const QString& noResultMessage = QObject::tr("No result to display"),
                                  const QString& unit = QString(),
                                  FormatFunc formatFunc = 0) const;
    
    QStringList getResultsList(const QLocale locale, const bool withDateFormat = true);


    /* obsolete change with the operator& << and >>
     * QDataStream &operator<<( QDataStream &stream, const MetropolisVariable &data );
     *
     * QDataStream &operator>>( QDataStream &stream, MetropolisVariable &data );
    */
        void saveToStreamOfQByteArray(QDataStream *out);
        void saveToStream(QDataStream &out);

        void loadFromStreamOfQByteArray(QDataStream *in);
        void loadFromStream(QDataStream &in);

public slots:
      void updateFormatedTrace();

private:
    void generateBufferForHisto(float* input, const QVector<float> &dataSrc, const int numPts, const float a, const float b);
    QMap<float, float> bufferToMap(const float* buffer);
    QMap<float, float> generateHisto(const QVector<float>& data, const int fftLen, const  float bandwidth, const float tmin = 0, const float tmax = 0);



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
    QVector<float> mRawTrace, mFormatedTrace;
    //std::vector<double> mRawTrace, mFormatedTrace;
    // if we use std::vector we can not use QDataStream to save,
    //because QDataStream provides support for multi system and takes account of endians
    Support mSupport;
    DateUtils::FormatDate mFormat;
    
    // Posterior density results.
    // mHisto is calcuated using all run parts of all chains traces.
    // mChainsHistos constains posterior densities for each chain, computed using only the "run" part of the trace.
    // This needs to be re-calculated each time we change fftLength or bandwidth.
    // See generateHistos() for more.
    QMap<float, float> mHisto;
    QList<QMap<float, float> > mChainsHistos;
    
    // List of correlations for each chain.
    // They are calculated once, when the MCMC is ready, from the run part of the trace.
    QList<QVector<float> > mCorrelations;
    
    QMap<float, float> mHPD;
    QPair<float, float> mCredibility;
    
    float mExactCredibilityThreshold;
    
    DensityAnalysis mResults;
    QList<DensityAnalysis> mChainsResults;

    int mfftLenUsed;
    float mBandwidthUsed;
    float mThresholdUsed;

    float mtminUsed;
    float mtmaxUsed;


private:
    QString mName;
};

QDataStream &operator<<( QDataStream &stream, const MetropolisVariable &data );

QDataStream &operator>>( QDataStream &stream, MetropolisVariable &data );
#endif
