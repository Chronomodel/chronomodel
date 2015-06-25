#ifndef METROPOLISVARIABLE_H
#define METROPOLISVARIABLE_H

#include <QMap>
#include <QVector>
#include <QList>
#include "MCMCLoop.h"
#include "Functions.h"
#include "ProjectSettings.h"


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
    const QMap<double, double>& fullRawHisto() const;
    const QMap<double, double>& histoForChain(int index) const;
    const QMap<double, double>& rawHistoForChain(int index) const;
    
    // Full trace for the chain (burn + adapt + run)
    QVector<double> fullTraceForChain(const QList<Chain>& chains, int index);
    
    // Trace for run part as a vector
    QVector<double> fullRunTrace(const QList<Chain>& chains);
    // Trace for run part of the chain as a vector
    QVector<double> runTraceForChain(const QList<Chain>& chains, int index);
    
    QVector<double> correlationForChain(int index);
    
    // -----
    
    virtual QString resultsText(const QString& noResultMessage = QObject::tr("No result to display"),
                                const QString& unit = QString(),
                                FormatFunc formatFunc = 0) const;
    // -----
    
private:
    float* generateBufferForHisto(const QVector<double>& dataSrc, int numPts, double hFactor);
    QMap<double, double> bufferToMap(const double* buffer);
    QMap<double, double> generateRawHisto(const QVector<double>& data, int fftLen, double tmin, double tmax);
    QMap<double, double> generateHisto(const QVector<double>& data, int fftLen, double hFactor, double tmin, double tmax);
    
public:
    double mX;
    QVector<double> mTrace;
    //std::vector<double> mTrace; // todo PhD
  
    QMap<double, double> mHisto;

    QList<QMap<double, double> > mChainsHistos;
    
    QMap<double, double> mRawHisto;
    QList<QMap<double, double> > mChainsRawHistos;
    
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
