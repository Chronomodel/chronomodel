#include "Generator.h"
#include <cmath>
#include <ctgmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>
#include <iostream>


std::mt19937 Generator::sGenerator = std::mt19937(0);
std::uniform_real_distribution<float> Generator::sDistribution = std::uniform_real_distribution<float>(0, 1);

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


float Generator::randomUniform(float min, float max)
{
    return min + sDistribution(sGenerator) * (max - min);
}

float Generator::gaussByDoubleExp(const float mean, const float sigma, const float min, const float max)
{
    const float x_min = (min - mean) / sigma;
    const float x_max = (max - mean) / sigma;

    if(abs(x_max - x_min) < 1E-3)
    {
        return randomUniform(min, max);
    }
    
    float x = (x_max + x_min) / 2;
    const float sqrt_e = sqrt(exp(1.));
    
    float ur = 1;
    float rap = 0;
    
    while(rap < ur)
    {
        float u = randomUniform();
        
        if(x_min < 0 && x_max > 0)
        {
            const float c = 1 - 0.5 * (exp(x_min) + exp(-x_max));
            const float f0 = 0.5 * (1 - exp(x_min)) / c;
            
            if(u <= f0)
            {
                x = log(exp(x_min) + 2*c*u);
            }
            else
            {
                x = -log(1 - 2*c*(u-f0));
            }
        }
        else
        {
            if(x_min >= 0)
            {
                x = -log(exp(-x_min) - u * (exp(-x_min) - exp(-x_max)));
            }
            else
            {
                x = log(exp(x_min) - u * (exp(x_min) - exp(x_max)));
            }
        }
        
        ur = randomUniform();
        
        if(std::isinf(x))
        {
            std::cout << "floatExp : ERROR : infinity" << std::endl;
            rap = 0;
        }
        else
        {
            if(x_min > 1)
            {
                rap = exp(0.5 * (x_min * x_min - x * x) + x - x_min);
                
                /*std::cout << "------" << std::endl;
                std::cout << "floatExp : x_min : " << x_min << std::endl;
                std::cout << "floatExp : min : " << min << std::endl;
                std::cout << "floatExp : max : " << max << std::endl;
                std::cout << "floatExp : mean : " << mean << std::endl;
                std::cout << "floatExp : sigma : " << sigma << std::endl;
                std::cout << "floatExp : rapport : " << rap << std::endl;
                std::cout << "floatExp : ur : " << ur << std::endl;*/
            }
            else if(x_max < -1)
            {
                //std::cout << "floatExp : x_max : " << x_max << std::endl;
                rap = exp(0.5 * (x_max * x_max - x * x) + x_max - x);
            }
            else
            {
                rap = exp(-0.5 * x * x + abs(x)) / sqrt_e;
            }
        }
        //ur = randomUniform();
    }
    
    return (mean + (x * sigma));
}

// Simulation d'une loi gaussienne centrée réduite
float Generator::boxMuller()
{
    float rand1 = randomUniform();
    float rand2 = randomUniform();
    return sqrt(-2. * log(rand1)) * cos(2. * M_PI * rand2);
}

float Generator::gaussByBoxMuller(const float mean, const float sigma)
{
    return mean + boxMuller() * sigma;
    
    /*float x;
    do
    {
        x = mean + boxMuller() * sigma;
    }while(x < min || x > max);
    return x;*/
}

