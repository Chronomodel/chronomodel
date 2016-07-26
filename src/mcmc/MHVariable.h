#ifndef MHVARIABLE_H
#define MHVARIABLE_H

#include "MetropolisVariable.h"




class MHVariable: public MetropolisVariable
{
   // Q_OBJECT
public:
    MHVariable();
    virtual ~MHVariable();
    
    virtual void reset();
    MHVariable& copy(MHVariable const& origin);
    MHVariable& operator=(MHVariable const& origin);

    double getCurrentAcceptRate();
    void saveCurrentAcceptRate();
    
    bool tryUpdate(const double x, const double rapportToTry);
    
    QVector<double> acceptationForChain(const QList<ChainSpecs>& chains, int index);
    void generateGlobalRunAcceptation(const QList<ChainSpecs>& chains);
    
    void generateNumericalResults(const QList<ChainSpecs>& chains);
    QString resultsString(const QString& nl = "<br>",
                          const QString& noResultMessage = QObject::tr("No result to display"),
                          const QString& unit = QString(),
                          FormatFunc formatFunc = 0) const;
/* Obsolete function */
    void saveToStream(QDataStream &out);
    void saveToStreamOfQByteArray(QDataStream *out);
    void loadFromStream(QDataStream &in);
    void loadFromStreamOfQByteArray(QDataStream *in);
    
public:
    double mSigmaMH;
    
    // Buffer glissant de la taille d'un batch pour calculer la courbe d'évolution
    // du taux d'acceptation chaine par chaine
    
    QVector<bool> mLastAccepts;
    
    int mLastAcceptsLength;

    // Buffer contenant toutes les acceptations cumulées pour toutes les chaines
    // sur les parties acquisition uniquement.
    // A stocker dans le fichier résultats .dat !
    
    QVector<bool> mAllAccepts;
    
    // Computed at the end as numerical result :
    double mGlobalAcceptation;
    
    // Buffer contenant tous les taux d'acceptation calculés (1 par batch)
    // On en affiche des sous-parties (correspondant aux chaines) dans la vue des résultats
    // A stocker dans les résultats!
    
    QVector<float> mHistoryAcceptRateMH;
    
    QString mProposal;
};

QDataStream &operator<<( QDataStream &stream, const MHVariable &data );

QDataStream &operator>>( QDataStream &stream, MHVariable &data );

#endif
