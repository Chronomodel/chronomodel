#ifndef MCMCLOOP_H
#define MCMCLOOP_H

#include <QThread>
#include "MCMCSettings.h"
//#include "Project.h"

#define ABORTED_BY_USER "Aborted by user"

class Project;

class MCMCLoop : public QThread
{
    Q_OBJECT
public:
    enum State
    {
        eBurning = 0,
        eAdapting = 1,
        eRunning = 2
    };
    
    MCMCLoop();
    virtual ~MCMCLoop();
    
    void setMCMCSettings(const MCMCSettings& settings);
    const QList<ChainSpecs>& chains() const;
    const QString& getChainsLog() const;
    const QString getInitLog() const;
    const QString getMCMCSettingsLog() const ;
    
    void run();
    
signals:
    void stepChanged(QString title, int min, int max);
    void stepProgressed(int value);
    
protected:
    virtual QString calibrate() = 0;
    virtual void initVariablesForChain() = 0;
    virtual QString initMCMC() = 0;
    virtual void update() = 0;
    virtual void finalize() = 0;
    virtual bool adapt() = 0;
    
protected:
    QList<ChainSpecs> mChains;
    int mChainIndex;
    State mState;

    QString mChainsLog;
    QString mInitLog;
    
public:
    QString mAbortedReason;
    Project* mProject;
};

#endif
