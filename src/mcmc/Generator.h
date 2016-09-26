#ifndef GENERATOR_H
#define GENERATOR_H

#include "random"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419716939937510582
#endif

class Generator
{
public:
    Generator();
    virtual ~Generator();
    static int createSeed();
    static void initGenerator(const int seed);
    
    static double randomUniform(const double min = 0., const double max = 1.) ;
    static double gaussByDoubleExp(const double mean, const double sigma, const double min, const double max) ;
    static double gaussByBoxMuller(const double mean, const double sigma) ;

    static double xorshift64star(void);
    static uint64_t xorshift64starSeed;
    static inline double to_double(uint64_t x) {
       const union { uint64_t i; double d; } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 };
       return u.d - 1.0;
    }

private:


    static double boxMuller() ;
    
    static std::mt19937 sEngine;
    static std::uniform_real_distribution<double> sDistribution;

    //https://en.wikipedia.org/wiki/Xorshift


};

#endif
