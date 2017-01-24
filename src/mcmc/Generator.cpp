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

//int matherr(struct exception *e);

std::mt19937 Generator::sEngine(0);
std::uniform_real_distribution<double> Generator::sDoubleDistribution(0.0, 1.0);

//http://xoroshiro.di.unimi.it/
std::uint64_t Generator::xorshift64starSeed(35);

void Generator::initGenerator(const int seed)
{
   sEngine.seed(seed);
   sDoubleDistribution.reset();
   //qDebug()<<"initGenerator seed"<<seed;
   xorshift64starSeed = seed;

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


double Generator::randomUniform(const double min, const double max)
{
    //const double r = min + sDoubleDistribution(sEngine) * (max - min);
    //const double r = min + xorshift64star() * (max - min);

    return min + sDoubleDistribution(sEngine) * (max - min);
    // return min + xorshift64star() * (max - min);
}

int Generator::randomUniformInt(const int min, const int max)
{
   return (int) round(randomUniform(min, max));
}

double Generator::gaussByDoubleExp(const double mean, const double sigma, const double min, const double max)
{
    errno=0;
    if((min >= max) || (sigma == 0)) {
        if(min == max)
            qDebug() << "DOUBLE EXP WARNING : min == max";
        else
            throw QObject::tr("DOUBLE EXP ERROR : min = ") + QString::number(min) + ", max = " + QString::number(max);

        if(sigma == 0)
            qDebug() << "DOUBLE EXP WARNING : sigma == 0";
        else
            throw QObject::tr("DOUBLE EXP ERROR : sigma == 0, mman = ") + QString::number(mean) ;

        return min;
    }

    const long double x_min = (min - mean) / sigma;
    const long double x_max = (max - mean) / sigma;
    
    long double x = (x_max + x_min) / 2.0;// initialisation arbitraire, valeur écrasée ensuite
    //const long double sqrt_e = sqrtl(expl(1.0));
    const long double sqrt_e = 1.64872127070012814689;
    feclearexcept(FE_ALL_EXCEPT);
    
    long double exp_x_min = 0.0;
    long double exp_x_max = 0.0;
    long double exp_minus_x_min = 0.0;
    long double exp_minus_x_max = 0.0;
    long double c = 0.0;
    long double f0 = 0.0;

    if(x_min < 0. && x_max > 0.) {
        exp_x_min = expl(x_min);
        exp_minus_x_max = expl(-x_max);
        c = 1. - 0.5 * (exp_x_min + exp_minus_x_max);
        f0 = 0.5 * (1. - exp_x_min) / c;
    }
    else {
        if(x_min >= 0.) {
            exp_minus_x_min = expl(-x_min);
            exp_minus_x_max = expl(-x_max);
        } else {
            exp_x_min = expl(x_min);
            exp_x_max = expl(x_max);
        }
    }
    //info = "DoubleExp : exp_x_min = " + QString::number(exp_x_min) + ", exp_x_max = " + QString::number(exp_x_max);
    //qDebug() <<"DoubleExp : exp_x_min = "<<exp_x_min;
    //qDebug() << "exp(10 000=="<<exp((long double)(1000));
    //qDebug() << "DOUBLE EXP DoubleExp : errno apres = "<<strerror(errno);
    if(errno != 0) {
        qDebug() << "DOUBLE EXP : errno apres exp_minus_x_max = "<<strerror(errno);
        qDebug() <<"DoubleExp : mean = "<< mean<<" min="<<min<<" max="<<max<<" sigma"<<sigma;
        qDebug() <<" x_min="<< (double)(x_min)<<" x_max="<<(double)(x_max);

        errno=0;
    }
    double ur = 1.0;
    long double rap = 0.0;
    
    int trials = 0.;
    const int limit = 100000;
    
    while(rap < ur && trials < limit) {
        const long double u = (long double)randomUniform();
        
        if(x_min < 0. && x_max > 0.) {

            if(u <= f0)
                x = logl(exp_x_min + 2.0 * c * u);
            else
                x = -logl(1. - 2.0*c*(u-f0));

        } else {
            if(x_min >= 0.)
                x = -logl(exp_minus_x_min - u * (exp_minus_x_min - exp_minus_x_max));
            else
                x = logl(exp_x_min - u * (exp_x_min - exp_x_max));
        }

        if(errno != 0) {
            //qDebug() << "DOUBLE EXP dans boucle = "<<strerror(errno);
            throw "DoubleExp could not find a solution after " + QString::number(limit) + " trials! This may be ue to Taylor unsufficients developpement orders. Please try to run the calculations again!";
        }
        ur = randomUniform();
        
        if(x_min >= 1.)
            rap = expl(0.5 * (x_min * x_min - x * x) + x - x_min);

        else if(x_max <= -1.)
            rap = expl(0.5 * (x_max * x_max - x * x) + x_max - x);

        else
            rap = expl(-0.5 * x * x + std::fabs(x)) / sqrt_e;
        
        ++trials;
    }

    if(trials == limit)
        throw "DoubleExp could not find a solution after " + QString::number(limit) + " trials! This may be ue to Taylor unsufficients developpement orders. Please try to run the calculations again!";
#ifdef DEBUG
    if ((x<x_min) || (x>x_max)) {
        qDebug() << "DOUBLE EXP DoubleExp : x = "<<(double)(x);
        qDebug() << "DOUBLE EXP DoubleExp : (mean + (x * sigma)) = "<<(double)(mean + (x * sigma));
        qDebug() <<" min="<< min<<" max=" <<(double)(x_max);
        
    }
#endif
    return (double)(mean + (x * sigma));
}

// Simulation d'une loi gaussienne centrée réduite
double Generator::boxMuller()
{
    const double rand1 = randomUniform();
    const double rand2 = randomUniform();
    return sqrt(-2. * log(rand1)) * cos(2. * M_PI * rand2);
    //checkFloatingPointException("boxMuller");
}

double Generator::gaussByBoxMuller(const double mean, const double sigma)
{
    return mean + boxMuller() * sigma;
    
}

//https://en.wikipedia.org/wiki/Xorshift
//uint64_t xorshift64starSeed; /* The state must be seeded with a nonzero value. */

double Generator::xorshift64star(void) {
       Generator:: xorshift64starSeed ^= Generator::xorshift64starSeed >> 12; // a
        Generator::xorshift64starSeed ^= Generator::xorshift64starSeed << 25; // b
        Generator::xorshift64starSeed ^= Generator::xorshift64starSeed >> 27; // c
        return to_double(Generator::xorshift64starSeed * UINT64_C(2685821657736338717));
}
