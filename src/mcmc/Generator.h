/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

    static double randomUniform(const double &min = 0., const double &max = 1.) ;
    static int randomUniformInt(const int& min = 0, const int& max = 1);

    static double gaussByDoubleExp(const double mean, const double sigma, const double min, const double max) ;
    static double gaussByBoxMuller(const double &mean, const double &sigma) ;

    static double xorshift64star(void);
    static uint64_t xorshift64starSeed;
    static inline double to_double(uint64_t x) {
       // const union { uint64_t i; double d; } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 }; // don't work with MSVC2015
       // return u.d - 1.0;
        return static_cast<double>(x);
    }

private:


    static double boxMuller() ;

    static std::mt19937 sEngine;
    static std::uniform_real_distribution<double> sDoubleDistribution;

    //https://en.wikipedia.org/wiki/Xorshift


};

#endif
