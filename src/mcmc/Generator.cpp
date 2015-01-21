#include "Generator.h"
#include <cmath>
#include <errno.h>
#include <fenv.h>
#include <ctgmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <QDebug>
#include "StdUtilities.h"


std::mt19937 Generator::sGenerator = std::mt19937(0);
std::uniform_real_distribution<double> Generator::sDistribution = std::uniform_real_distribution<double>(0, 1);

void Generator::initGenerator(const int seed)
{
    sDistribution = std::uniform_real_distribution<double>(0, 1);
    sGenerator = std::mt19937(seed);
}

int Generator::createSeed()
{
    // obtain a seed from the system clock:
    // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    
    // http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
    
    //std::random_device rd;
    //return rd();
    
    return rand() % 1000;
}


double Generator::randomUniform(double min, double max)
{
    return min + sDistribution(sGenerator) * (max - min);
}

double Generator::gaussByDoubleExp(const double mean, const double sigma, const double min, const double max)
{
    if(min >= max)
    {
        if(min == max)
            qDebug() << "DOUBLE EXP WARNING : min == max";
        else
            throw QObject::tr("DOUBLE EXP ERROR : min = ") + QString::number(min) + ", max = " + QString::number(max);
        return min;
    }
    
    const double x_min = (min - mean) / sigma;
    const double x_max = (max - mean) / sigma;
    
    QString info = "DoubleExp : x_min = " + QString::number(x_min) + ", x_max = " + QString::number(x_max);

    /*if(abs(x_max - x_min) < 1E-20)
    {
        return randomUniform(min, max);
    }*/
    
    double x = (x_max + x_min) / 2.;
    const double sqrt_e = sqrt(exp(1.));
    
    feclearexcept(FE_ALL_EXCEPT);
    
    /*double exp_x_min = safeExp(x_min);
    checkFloatingPointException(info + ", calculating exp(x_min)");
    double exp_x_max = safeExp(x_max);
    checkFloatingPointException(info + ", calculating exp(x_max)");
    double exp_minus_x_min = safeExp(-x_min);
    checkFloatingPointException(info + ", calculating exp(-x_min)");
    double exp_minus_x_max = safeExp(-x_max);
    checkFloatingPointException(info + ", calculating exp(-x_max)");*/
    
    double exp_x_min = exp(x_min);
    double exp_x_max = exp(x_max);
    double exp_minus_x_min = exp(-x_min);
    double exp_minus_x_max = exp(-x_max);
    
    double ur = 1.;
    double rap = 0.;
    
    int trials = 0;
    int limit = 100000;
    
    while(rap < ur && trials < limit)
    {
        double u = (double)randomUniform();
        
        if(x_min < 0. && x_max > 0.)
        {
            const double c = 1. - 0.5 * (exp_x_min + exp_minus_x_max);
            //checkFloatingPointException(info + ", calculating c.");
            
            const double f0 = 0.5 * (1. - exp_x_min) / c;
            //checkFloatingPointException(info + ", calculating f0 with exp(x_min) = " + QString::number(exp_x_min));
            
            if(u <= f0)
            {
                x = log(exp_x_min + 2. * c * u);
                
                //x = safeLog(exp_x_min + 2. * c * u);
                //checkFloatingPointException(info + ", u <= f0");
            }
            else
            {
                x = -log(1. - 2.*c*(u-f0));
                
                //x = -safeLog(1. - 2.*c*(u-f0));
                //checkFloatingPointException(info + ", u > f0");
            }
        }
        else
        {
            if(x_min >= 0.)
            {
                x = -log(exp_minus_x_min - u * (exp_minus_x_min - exp_minus_x_max));
                
                //x = -safeLog(exp_minus_x_min - u * (exp_minus_x_min - exp_minus_x_max));
                //checkFloatingPointException(info + ", x_min >= 0");
            }
            else
            {
                x = log(exp_x_min - u * (exp_x_min - exp_x_max));
                
                //x = safeLog(exp_x_min - u * (exp_x_min - exp_x_max));
                //checkFloatingPointException(info + ", x_min < 0");
            }
        }
        
        ur = randomUniform();
        
        if(x_min >= 1.)
        {
            rap = exp(0.5 * (x_min * x_min - x * x) + x - x_min);
            
            //rap = safeExp(0.5 * (x_min * x_min - x * x) + x - x_min);
            //checkFloatingPointException(info + ", rap failed with x_min >= 1");
        }
        else if(x_max <= -1.)
        {
            rap = exp(0.5 * (x_max * x_max - x * x) + x_max - x);
            
            //rap = safeExp(0.5 * (x_max * x_max - x * x) + x_max - x);
            //checkFloatingPointException(info + ", rap failed with x_max <= 1");
        }
        else
        {
            rap = exp(-0.5 * x * x + abs(x)) / sqrt_e;
            
            //rap = safeExp(-0.5 * x * x + abs(x)) / sqrt_e;
            //checkFloatingPointException(info + ", rap failed");
        }
        
        ++trials;
    }
    //checkFloatingPointException(info);
    
    if(trials == limit)
    {
        throw "DoubleExp could not find a solution after " + QString::number(limit) + " trials! This may be ue to Taylor unsufficients developpement orders. Please try to run the calculations again!";
    }
    
    return (double)(mean + (x * sigma));
}

// Simulation d'une loi gaussienne centrée réduite
double Generator::boxMuller()
{
    double rand1 = randomUniform();
    double rand2 = randomUniform();
    return sqrt(-2. * log(rand1)) * cos(2. * M_PI * rand2);
    checkFloatingPointException("boxMuller");
}

double Generator::gaussByBoxMuller(const double mean, const double sigma)
{
    return mean + boxMuller() * sigma;
    
    /*double x;
    do
    {
        x = mean + boxMuller() * sigma;
    }while(x < min || x > max);
    return x;*/
}

