/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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
std::normal_distribution<double> Generator::sNormalDistribution(0.0, 1.0);

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

/* ol code
 double Generator::normalDistribution(const double mean, const double sigma)
{
    return mean + boxMuller() * sigma;
}
*/

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




double Generator::gammaDistribution(const double alpha, const double beta)
{
    std::gamma_distribution<double>  gamma(alpha, beta);
    return gamma(sEngine);
}

double Generator::exponentialeDistribution(const double meanexp)
{
    std::exponential_distribution<double> exponential(meanexp);
    return exponential(sEngine);
}

