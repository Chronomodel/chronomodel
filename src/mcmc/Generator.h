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

#ifndef GENERATOR_H
#define GENERATOR_H

#include <chrono>
#include <algorithm>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419716939937510582
#endif

static std::default_random_engine CharGenerator (int(std::chrono::system_clock::now().time_since_epoch().count()));

struct randomChar {
    int _a, _b;
    std::uniform_int_distribution<int> CharDistribution;
    randomChar() {
        _a = 97;
        _b = 122;
        CharDistribution.param(std::uniform_int_distribution<int>::param_type(97, 122));
    }

    
    void reset () { CharDistribution.reset(); }
    
    int operator()() {return CharDistribution(CharGenerator);}
} ;


struct c_unique {
    int current;
    c_unique() {current=0;}
    int operator()() {return ++current;}
    std::string  tostring () {return std::to_string(++current);}
} ;


struct c_UUID {
    c_unique UniqueNumber;
    randomChar CharGen;
    c_UUID() {
        c_unique UniqueNumber;
        static randomChar CharGen;
    }
    std::string operator()()
        {
        using std::chrono::system_clock;
        const system_clock::time_point now = system_clock::now();
          
        auto duration = now.time_since_epoch();
        
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
            duration -= hours;
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
            duration -= minutes;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
            duration -= seconds;
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
            duration -= milliseconds;
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
            duration -= microseconds;
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
         
        std::string uuidName (5, '?');
        
        std::generate_n (uuidName.begin(),5, CharGen);
        
        std::string uuid ;
        
        uuid += uuidName;
        uuid += ':' + std::to_string(hours.count());
        uuid += ':' + std::to_string(minutes.count());
        uuid += ':' + std::to_string(seconds.count());
        uuid += ':' + std::to_string(milliseconds.count());
        uuid += ':' + std::to_string(microseconds.count());
        uuid += ':' + std::to_string(nanoseconds.count());
        uuid += ':';
        uuid.append( UniqueNumber.tostring());
        
        return uuid;
        }

};



class Generator
{
public:
    Generator();
    virtual ~Generator();
    static unsigned createSeed();
    static void initGenerator(const unsigned seed);

    static double randomUniform(const double &min = 0., const double &max = 1.) ;
    static int randomUniformInt(const int& min = 0, const int& max = 1);

    static double gaussByDoubleExp (const double mean, const double sigma, const double min, const double max) ;
    static double gaussByBoxMuller (const double mean, const double sigma);
    static double shrinkage (const double variance, const double shrinkage); // à controler

    static double xorshift64star(void);
    static uint64_t xorshift64starSeed;
    static inline double to_double(uint64_t x) {
       // const union { uint64_t i; double d; } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 }; // don't work with MSVC2015
       // return u.d - 1.0;
        return static_cast<double>(x);
    }

    static c_UUID UUID;
    
    
private:

    
    static double boxMuller() ;

    static std::mt19937 sEngine;
    static std::uniform_real_distribution<double> sDoubleDistribution;

    //https://en.wikipedia.org/wiki/Xorshift


};

#endif
