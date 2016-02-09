#ifndef GENERATOR_H
#define GENERATOR_H

#include "random"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419716939937510582
#endif

class Generator
{
public:
    static int createSeed();
    static void initGenerator(const int seed);
    
    static double randomUniform(const double min = 0., const double max = 1.) ;
    static double gaussByDoubleExp(const double mean, const double sigma, const double min, const double max) ;
    static double gaussByBoxMuller(const double mean, const double sigma) ;
    
private:
    Generator(){}
    static double boxMuller() ;
    
    static std::mt19937 sGenerator;
    static std::uniform_real_distribution<double> sDistribution;
};

#endif
