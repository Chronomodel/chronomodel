/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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
#include <QVector>
#include <QList>
#include <QDataStream>
#include <QObject>
#include <QList>

class TValueStack
{
public :
    std::string mName;
    double mValue;
    std::string mComment ;

    TValueStack():mName("name"), mValue(0.), mComment("comment") {};
    explicit TValueStack(std::string name, double value = 0., std::string comment ="") :mName(name), mValue(value), mComment(comment) {};

};

class MetropolisVariable: public QObject
{
    Q_OBJECT
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
    QVector<double>* mRawTrace;
    QVector<double>* mFormatedTrace;


    // if we use std::vector we can not use QDataStream to save,
    //because QDataStream provides support for multi system and takes account of endians
    Support mSupport;
    DateUtils::FormatDate mFormat;

    // Posterior density results.
    // mFormatedHisto is calculated using all run parts of all chains traces.
    // mChainsHistos constains posterior densities for each chain, computed using only the "run" part of the trace.
    // This needs to be re-calculated each time we change fftLength or bandwidth.
    // See generateHistos() for more.
    QMap<double, double> mFormatedHisto;
    QList<QMap<double, double> > mChainsHistos;

    // List of correlations for each chain.
    // They are calculated once, when the MCMC is ready, from the run part of the trace.
    QList<QVector<double> > mCorrelations;

    QMap<double, double> mFormatedHPD;
    QList<QPair<double, QPair<double, double> > > mRawHPDintervals;

    std::pair<double, double> mRawCredibility;
    std::pair<double, double> mFormatedCredibility;

    //double mCredibilityThresholdUsed;
    double mExactCredibilityThreshold;

    DensityAnalysis mResults;
    QList<DensityAnalysis> mChainsResults;

    int mfftLenUsed;
    double mBandwidthUsed;
    double mThresholdUsed;

    double mtminUsed;
    double mtmaxUsed;


private:
    QString mName;
public:
    MetropolisVariable(QObject *parent = nullptr);
    explicit MetropolisVariable(const MetropolisVariable& origin);

    virtual ~MetropolisVariable();
    MetropolisVariable& operator=(const MetropolisVariable & origin);


    void memo(double *valueToSave = nullptr);
    virtual void reset();
    virtual void reserve( const int reserve);

    void setFormat(const DateUtils::FormatDate fm);

    inline QString getName() {return mName;}
    void setName(const QString name) {mName = name;}
    // -----
    //  These functions are time consuming!
    // -----
    void generateCorrelations(const QList<ChainSpecs> &chains);

    void generateHistos(const QList<ChainSpecs> &chains, const int fftLen = 1024, const double bandwidth = 1.06, const double tmin = 0., const double tmax = 0.);
    void memoHistoParameter(const int fftLen = 1024, const double bandwidth = 1.06, const double tmin = 0., const double tmax = 0.);
    bool HistoWithParameter(const int fftLen = 1024, const double bandwidth = 1.06, const double tmin = 0., const double tmax = 0.);

    void generateHPD(const double threshold = 95);
    void generateCredibility(const QList<ChainSpecs>& chains, double threshold = 95.);


    // Virtual because MHVariable subclass adds some information
    virtual void generateNumericalResults(const QList<ChainSpecs>& chains);
    void updateFormatedCredibility(const DateUtils::FormatDate fm);


    QMap<double, double> generateHisto(const QVector<double>& data, const int fftLen, const  double bandwidth, const double tmin = 0., const double tmax = 0.);

    // -----
    // These functions do not make any calculation
    // -----
    QMap<double, double>& fullHisto();
    QMap<double, double>& histoForChain(const int index);

    // Full trace for the chain (burn + adapt + run)
    QVector<double> fullTraceForChain(const QList<ChainSpecs> &chains,const int index);

    // Trace for run part as a vector
    QVector<double> fullRunFormatedTrace(const QList<ChainSpecs>& chains);
    QVector<double> fullRunRawTrace(const QList<ChainSpecs>& chains);
    QList<double>::Iterator findIter_element(const long unsigned iter, const QList<ChainSpecs>& chains, const int chainIndex ) const;

    // Trace for run part of the chain as a vector
    QVector<double> runRawTraceForChain(const QList<ChainSpecs>& chains, const int index);
    QVector<double> runFormatedTraceForChain(const QList<ChainSpecs>& chains, const int index);

    QVector<double> correlationForChain(const int index);

    // -----

    virtual QString resultsString(const QString& nl = "<br>",
                                  const QString& noResultMessage = QObject::tr("No result to display"),
                                  const QString& unit = QString(),
                                  DateConversion formatFunc = nullptr,
                                  const bool forCSV = false) const;

    QStringList getResultsList(const QLocale locale, const int precision = 0, const bool withDateFormat = true);


    /* obsolete change with the operator& << and >>
     * QDataStream &operator<<( QDataStream &stream, const MetropolisVariable &data );
     *
     * QDataStream &operator>>( QDataStream &stream, MetropolisVariable &data );
    */
     /*   void saveToStreamOfQByteArray(QDataStream *out);
        void saveToStream(QDataStream &out);

        void loadFromStreamOfQByteArray(QDataStream *in);
        void loadFromStream(QDataStream &in);
*/
//public slots:
      void updateFormatedTrace(const DateUtils::FormatDate fm);

private:
    void generateBufferForHisto(double* input, const QVector<double> &dataSrc, const int numPts, const double a, const double b);
    QMap<double, double> bufferToMap(const double* buffer);



signals:
   // void formatChanged(); // unused


};

QDataStream &operator<<( QDataStream &stream, const MetropolisVariable &data );

QDataStream &operator>>( QDataStream &stream, MetropolisVariable &data );
#endif
