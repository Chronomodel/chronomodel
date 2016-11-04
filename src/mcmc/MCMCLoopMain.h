#ifndef MCMCLOOPMAIN_H
#define MCMCLOOPMAIN_H

#include "MCMCLoop.h"
//#include "Model.h"
//#include "Project.h"

class Project;
class Model;

class MCMCLoopMain: public MCMCLoop
{
    Q_OBJECT
public:
    MCMCLoopMain(Model* model, Project* project);
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
