#ifndef PluginAMLoop_H
#define PluginAMLoop_H

#include "MCMCLoop.h"

class Project;
class Model;


class PluginAMLoop: public MCMCLoop
{
    Q_OBJECT
public:
    PluginAMLoop();
    ~PluginAMLoop();
    
protected:
    virtual QString calibrate();
    virtual void initVariablesForChain();
    virtual QString initMCMC();
    virtual void update();
    virtual bool adapt();
    virtual void finalize();
};

#endif
