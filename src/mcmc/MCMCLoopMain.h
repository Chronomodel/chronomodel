#ifndef MCMCLOOPMAIN_H
#define MCMCLOOPMAIN_H

#include "MCMCLoop.h"
#include "Model.h"


class MCMCLoopMain: public MCMCLoop
{
    Q_OBJECT
public:
    MCMCLoopMain(Model* model);
    ~MCMCLoopMain();
    
protected:
    virtual QString calibrate();
    virtual void initVariablesForChain();
    virtual QString initMCMC();
    virtual void update();
    virtual bool adapt();
    virtual void finalize();

public:
    Model* mModel;
};

#endif
