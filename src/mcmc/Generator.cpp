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


std::mt19937 Generator::sGenerator = std::mt19937(0);
std::uniform_real_distribution<double> Generator::sDistribution = std::uniform_real_distribution<double>(0, 1);

void Generator::initGenerator(const int seed)
{
    sGenerator = std::mt19937(seed);
}

int Generator::createSeed()
{
    // obtain a seed from the system clock:
    // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    
    // http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
    
    std::random_device rd;
    return rd();
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
            qDebug() << "DOUBLE EXP ERROR : min == max";
        else
            qDebug() << "DOUBLE EXP ERROR : min > max";
        return min;
    }
    
    const double x_min = (min - mean) / sigma;
    const double x_max = (max - mean) / sigma;

    if(abs(x_max - x_min) < 1E-3)
    {
        return randomUniform(min, max);
    }
    
    double x = (x_max + x_min) / 2.;
    const double sqrt_e = sqrt(exp(1.));
    
    double ur = 1.;
    double rap = 0.;
    
    while(rap < ur)
    {
        double u = (double)randomUniform();
        
        try{
            if(x_min < 0. && x_max > 0.)
            {
                const double c = 1. - 0.5 * (exp(x_min) + exp(-x_max));
                const double f0 = 0.5 * (1. - exp(x_min)) / c;
                
                if(u <= f0)
                {
                    x = log(exp(x_min) + 2. * c * u);
                }
                else
                {
                    x = -log(1. - 2.*c*(u-f0));
                }
            }
            else
            {
                if(x_min >= 0.)
                {
                    x = -log(exp(-x_min) - u * (exp(-x_min) - exp(-x_max)));
                }
                else
                {
                    x = log(exp(x_min) - u * (exp(x_min) - exp(x_max)));
                }
            }
        }
        catch(std::exception e){
            std::cout << "doubleExp : Exception raised" << std::endl;
            if(fetestexcept(FE_INVALID))
            {
                
            }
        }
        
        
        ur = randomUniform();
        
        if(std::isinf(x))
        {
            std::cout << "doubleExp : ERROR : infinity" << std::endl;
            if(x_min < 0. && x_max > 0.)
            {
                const double c = 1. - 0.5 * (exp(x_min) + exp(-x_max));
                const double f0 = 0.5 * (1. - exp(x_min)) / c;
                if(u <= f0)
                    std::cout << "u <= f0" << std::endl;
                else
                    std::cout << "u > f0" << std::endl;
            }
            else
            {
                if(x_min >= 0)
                    std::cout << "x_min >= 0" << std::endl;
                else
                    std::cout << "x_min < 0" << std::endl;
            }
            rap = 0;
        }
        else
        {
            if(x_min >= 1.)
            {
                rap = exp(0.5 * (x_min * x_min - x * x) + x - x_min);
                
                /*std::cout << "------" << std::endl;
                std::cout << "doubleExp : x_min : " << x_min << std::endl;
                std::cout << "doubleExp : min : " << min << std::endl;
                std::cout << "doubleExp : max : " << max << std::endl;
                std::cout << "doubleExp : mean : " << mean << std::endl;
                std::cout << "doubleExp : sigma : " << sigma << std::endl;
                std::cout << "doubleExp : rapport : " << rap << std::endl;
                std::cout << "doubleExp : ur : " << ur << std::endl;*/
            }
            else if(x_max <= -1.)
            {
                //std::cout << "doubleExp : x_max : " << x_max << std::endl;
                rap = exp(0.5 * (x_max * x_max - x * x) + x_max - x);
            }
            else
            {
                rap = exp(-0.5 * x * x + abs(x)) / sqrt_e;
            }
        }
        //ur = randomUniform();
    }
    
    return (double)(mean + (x * sigma));
}

// Simulation d'une loi gaussienne centrée réduite
double Generator::boxMuller()
{
    double rand1 = randomUniform();
    double rand2 = randomUniform();
    return sqrt(-2. * logf(rand1)) * cos(2. * M_PI * rand2);
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

