#ifndef MCMCLOOP_H
#define MCMCLOOP_H

#include <QThread>
#include "MCMCSettings.h"


class MCMCLoop : public QThread
{
    Q_OBJECT
public:
    enum State
    {
        eBurning = 0,
        eAdapting = 0,
        eRunning = 0
    };
    
    MCMCLoop();
    virtual ~MCMCLoop();
    
    void setMCMCSettings(const MCMCSettings& settings);
    const QList<Chain>& chains();
    const QString& getLog() const;
    
    void run();
    
signals:
    void stepChanged(QString title, int min, int max);
    void stepProgressed(int value);
    
protected:
    virtual QString calibrate() = 0;
    virtual void initVariablesForChain() = 0;
    virtual void initMCMC() = 0;
    virtual void initMCMC2() = 0;
    virtual void update() = 0;
    virtual void finalize() = 0;
    virtual bool adapt() = 0;
    
protected:
    QList<Chain> mChains;
    int mChainIndex;
    State mState;
    QString mLog;
    
public:
    QString mAbortedReason;
};

#endif
