#ifndef MCMCLOOPMAIN_H
#define MCMCLOOPMAIN_H

#include "MCMCLoop.h"
#include "Model.h"


class MCMCLoopMain: public MCMCLoop
{
    Q_OBJECT
public:
    MCMCLoopMain(Model& model);
    ~MCMCLoopMain();
    
protected:
    virtual bool initModel();
    virtual void calibrate();
    virtual void initMCMC();
    virtual void update();
    virtual bool adapt();
    virtual void finalize();

protected:
    Model& mModel;
};

#endif
