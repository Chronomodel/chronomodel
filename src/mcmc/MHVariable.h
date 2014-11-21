#ifndef MHVARIABLE_H
#define MHVARIABLE_H

#include "MetropolisVariable.h"


class MHVariable: public MetropolisVariable
{
public:
    MHVariable();
    virtual ~MHVariable();
    
    virtual void reset();
    
    float getCurrentAcceptRate();
    void saveCurrentAcceptRate();
    
    bool tryUpdate(const double x, const double rapportToTry);
    
    QMap<float, float> acceptationForChain(const QList<Chain>& chains, int index);
    void generateGlobalRunAcceptation(const QList<Chain>& chains);
    
    void generateResults(const QList<Chain>& chains, float tmin, float tmax);
    QString resultsText() const;
    
public:
    double mSigmaMH;
    
    // Buffer glissant de la taille d'un batch pour calculer la courbe d'évolution
    // du taux d'acceptation chaine par chaine
    QVector<bool> mLastAccepts;
    int mLastAcceptsLength;
    
    // Buffer contenant toutes les acceptations cumulées pour toutes les chaines
    // sur les parties acquisition uniquement
    QVector<bool> mAllAccepts;
    int mAllAcceptsLength;
    float mGlobalAcceptation;
    
    //QVector<float> mHistorySigmaMH;
    QVector<float> mHistoryAcceptRateMH;
    
    QString mProposal;
};

#endif
