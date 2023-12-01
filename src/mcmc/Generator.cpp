/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "Generator.h"

#include <cmath>
#include <errno.h>
#include <fenv.h>
#include <ctgmath>

#include <chrono>
#include <QDebug>
#include <QObject>

// Mersenne Twister 19937 generator
std::mt19937 Generator::sEngine (0);
std::uniform_real_distribution<double> Generator::sDoubleDistribution (0.0, 1.0);

std::default_random_engine CharGenerator (int(std::chrono::system_clock::now().time_since_epoch().count()));

int randomChar::operator()() {
    return CharDistribution(CharGenerator);
}

c_UUID Generator::UUID;

//http://xoroshiro.di.unimi.it/
std::uint64_t Generator::xorshift64starSeed(35); /**< used with Generator::xorshift64star(void) */

void Generator::initGenerator(const unsigned seed)
{
   sEngine.seed(seed);
   sDoubleDistribution.reset();
   //qDebug()<<"initGenerator seed"<<seed;
   xorshift64starSeed = seed;
}

unsigned Generator::createSeed()
{
    // obtain a seed from the system clock:
    // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    // http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution

    //std::random_device rd;
    //return rd();

    //return rand() % 1000;
    //return arc4random() % 1000; // invalide for Windows os

    // Seed with a real random value, if available
    std::random_device r;

    // Choose a random mean between 1 and 6
    std::mt19937 gen(r());
    std::uniform_int_distribution<int> uniform_dist(1, 1000);
    return uniform_dist(gen);
}


double Generator::randomUniform(const double min, const double max)
{
    return min + sDoubleDistribution(sEngine) * (max - min);
    // return min + xorshift64star() * (max - min);
}

int Generator::randomUniformInt(const int min, const int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(sEngine);
   //return (int) round(randomUniform(min, max));
}

double Generator::gaussByDoubleExp(const double mean, const double sigma, const double min, const double max)
{
    errno = 0;
    if ((min >= max) || (sigma == 0)) {
        if ((min >= max) && (sigma != 0))
            throw QObject::tr("[Generator::gaussByDoubleExp]: min = %1 , max = %2").arg(QString::number(min), QString::number(max));

#ifdef DEBUG
        else
            qDebug() << "[Generator::gaussByDoubleExp] WARNING : min == max";
#endif

        if ((sigma == 0) && (min <= max))
            throw QObject::tr("[Generator::gaussByDoubleExp] sigma == 0, mean = %1").arg(QString::number(mean)) ;
#ifdef DEBUG
        else
            qDebug() << "[Generator::gaussByDoubleExp] WARNING : sigma == 0";
#endif

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

    if (x_min < 0. && x_max > 0.) {
        exp_x_min = expl(x_min);
        exp_minus_x_max = expl(-x_max);
        c = 1. - 0.5 * (exp_x_min + exp_minus_x_max);
        f0 = 0.5 * (1. - exp_x_min) / c;
    }
    else {
        if (x_min >= 0.) {
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
    if (errno != 0) {
        qDebug() << "DOUBLE EXP : errno apres exp_minus_x_max = "<<strerror(errno);
        qDebug() <<"DoubleExp : mean = "<< mean<<" min="<<min<<" max="<<max<<" sigma"<<sigma;
        qDebug() <<" x_min="<< (double)(x_min)<<" x_max="<<(double)(x_max);

        errno=0;
    }
    double ur = 1.0;
    long double rap = 0.0;

    int trials = 0.;
    const int limit = 100000;

    while (rap < ur && trials < limit) {
        const long double u = (long double)randomUniform();

        if (x_min < 0. && x_max > 0.) {

            if (u <= f0)
                x = logl(exp_x_min + 2.0 * c * u);
            else
                x = -logl(1. - 2.0*c*(u-f0));

        } else {
            if (x_min >= 0.)
                x = -logl(exp_minus_x_min - u * (exp_minus_x_min - exp_minus_x_max));
            else
                x = logl(exp_x_min - u * (exp_x_min - exp_x_max));
        }

        if (errno != 0) {
            //qDebug() << "DOUBLE EXP dans boucle = "<<strerror(errno);
            throw "DoubleExp could not find a solution after " + QString::number(limit) + " trials! This may be due to Taylor unsufficients developpement orders. Please try to run the calculations again!";
        }
        ur = randomUniform();

        if (x_min >= 1.)
            rap = expl(0.5 * (x_min * x_min - x * x) + x - x_min);

        else if (x_max <= -1.)
            rap = expl(0.5 * (x_max * x_max - x * x) + x_max - x);

        else
            rap = expl(-0.5 * x * x + std::fabs(x)) / sqrt_e;

        ++trials;
    }

    if (trials == limit)
        throw "DoubleExp could not find a solution after " + QString::number(limit) + " trials! This may bed ue to Taylor unsufficients developpement orders. Please try to run the calculations again!";
#ifdef DEBUG
    if ((x<x_min) || (x>x_max)) {
        qDebug() << "DOUBLE EXP DoubleExp : x = "<<(double)(x);
        qDebug() << "DOUBLE EXP DoubleExp : (mean + (x * sigma)) = "<<(double)(mean + (x * sigma));
        qDebug() <<" min="<< min<<" max=" <<(double)(x_max);

    }
#endif
    return (double)(mean + (x * sigma));
}


/**
 * @brief  Uniformly distributed random numbers with Box-Muller transform see: https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
 *
 *  \f$ U_1 \in [0;1]\f$ and \f$ U_2 \in[0;1] \f$
 *
 *  \f$ Z_0=\sqrt{-2 \ln{U_1}} * cos(2  \Pi * U_2) \f$
 */
double Generator::boxMuller()
{
    const double U1 = randomUniform();
    const double U2 = randomUniform();
    return sqrt(-2. * log(U1)) * cos(2. * M_PI * U2);
    //checkFloatingPointException("boxMuller");
}

double Generator::gaussByBoxMuller(const double mean, const double sigma)
{
    return mean + boxMuller() * sigma;
}

// obsolete
/*
  double Generator::shrinkage(const double variance, const double shrinkage)// à controler
{
   double x = std::sqrt(shrinkage) * boxMuller() + std::sqrt(1 - shrinkage) * boxMuller();
   return x * std::sqrt(variance);
}
*/

/** https://en.wikipedia.org/wiki/Xorshift
 *
 *  more information : http://xoroshiro.di.unimi.it/
 *
 *  uint64_t xorshift64starSeed;  The state must be seeded with a nonzero value.
 *
 *  This function is ready to use, but not used in this version
 */

double Generator::xorshift64star(void) {
       Generator::xorshift64starSeed ^= Generator::xorshift64starSeed >> 12; // a
       Generator::xorshift64starSeed ^= Generator::xorshift64starSeed << 25; // b
       Generator::xorshift64starSeed ^= Generator::xorshift64starSeed >> 27; // c

       return to_double(Generator::xorshift64starSeed * UINT64_C(2685821657736338717));
}

double Generator::shrinkageUniforme(const double shrinkage)
{
    const double u = Generator::randomUniform();
    const double x = shrinkage * ((1 - u) / u);
    return x;
}


double Generator::gammaDistribution(const double alpha, const double beta)
{
    std::gamma_distribution<double>  gamma(alpha, beta);
    return gamma(sEngine);
}

double Generator::exponentialeDistribution(const double meanexp)
{
    std::exponential_distribution<double>  exponential(meanexp);
    return exponential(sEngine);
}

