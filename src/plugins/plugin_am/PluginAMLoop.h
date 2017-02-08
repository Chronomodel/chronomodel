#ifndef PluginAMLoop_H
#define PluginAMLoop_H

#include "MCMCLoop.h"
#include "MHVariable.h"

class Date;


class PluginAMLoop: public MCMCLoop
{
    Q_OBJECT
public:
    PluginAMLoop(const Date* date, const ProjectSettings& settings);
    ~PluginAMLoop();
    
protected:
    virtual QString calibrate();
    virtual void initVariablesForChain();
    virtual QString initMCMC();
    virtual void update();
    virtual bool adapt();
    virtual void finalize();
    
    const Date* mDate;
    const ProjectSettings mSettings;
    
    MHVariable mT;
    MHVariable mSigma2;
    
    long double mSi;
    long double mSd;
    long double mSf;
    
    long double mSo2;
};

#endif
