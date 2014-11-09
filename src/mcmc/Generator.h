#ifndef GENERATOR_H
#define GENERATOR_H

#include <random>

#ifndef M_PI
#define M_PI 3.1415927
#endif

class Generator
{
public:
    static int createSeed();
    static void initGenerator(const int seed);
    
    static float randomUniform(float min = 0., float max = 1.);
    static float gaussByDoubleExp(const float mean, const float sigma, const float min, const float max);
    static float gaussByBoxMuller(const float mean, const float sigma);
    
private:
    Generator(){}
    static float boxMuller();
    
    static std::mt19937 sGenerator;
    static std::uniform_real_distribution<float> sDistribution;
};

#endif
