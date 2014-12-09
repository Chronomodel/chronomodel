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
    if(min >= max)
    {
        std::cout << "FLOAT EXP ERROR : min == max";
        return min;
    }
    
    const float x_min = (min - mean) / sigma;
    const float x_max = (max - mean) / sigma;

    if(abs(x_max - x_min) < 1E-3)
    {
        return randomUniform(min, max);
    }
    
    float x = (x_max + x_min) / 2.f;
    const float sqrt_e = sqrtf(expf(1.f));
    
    float ur = 1.f;
    float rap = 0.f;
    
    while(rap < ur)
    {
        float u = randomUniform();
        
        if(x_min < 0.f && x_max > 0.f)
        {
            const float c = 1.f - 0.5f * (expf(x_min) + expf(-x_max));
            const float f0 = 0.5f * (1.f - expf(x_min)) / c;
            
            if(u <= f0)
            {
                x = logf(expf(x_min) + 2.f * c * u);
            }
            else
            {
                x = -logf(1.f - 2.f*c*(u-f0));
            }
        }
        else
        {
            if(x_min >= 0)
            {
                x = -logf(expf(-x_min) - u * (expf(-x_min) - expf(-x_max)));
            }
            else
            {
                x = logf(expf(x_min) - u * (expf(x_min) - expf(x_max)));
            }
        }
        
        ur = randomUniform();
        
        if(std::isinf(x))
        {
            std::cout << "floatExp : ERROR : infinity" << std::endl;
            if(x_min < 0.f && x_max > 0.f)
            {
                const float c = 1.f - 0.5f * (expf(x_min) + expf(-x_max));
                const float f0 = 0.5f * (1.f - expf(x_min)) / c;
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
            if(x_min >= 1.f)
            {
                rap = expf(0.5f * (x_min * x_min - x * x) + x - x_min);
                
                /*std::cout << "------" << std::endl;
                std::cout << "floatExp : x_min : " << x_min << std::endl;
                std::cout << "floatExp : min : " << min << std::endl;
                std::cout << "floatExp : max : " << max << std::endl;
                std::cout << "floatExp : mean : " << mean << std::endl;
                std::cout << "floatExp : sigma : " << sigma << std::endl;
                std::cout << "floatExp : rapport : " << rap << std::endl;
                std::cout << "floatExp : ur : " << ur << std::endl;*/
            }
            else if(x_max <= -1.f)
            {
                //std::cout << "floatExp : x_max : " << x_max << std::endl;
                rap = expf(0.5f * (x_max * x_max - x * x) + x_max - x);
            }
            else
            {
                rap = expf(-0.5f * x * x + abs(x)) / sqrt_e;
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
    return sqrtf(-2. * logf(rand1)) * cos(2. * M_PI * rand2);
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

