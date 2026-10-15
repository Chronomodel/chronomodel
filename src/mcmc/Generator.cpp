/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "Generator.h"

#include <ctgmath>

#include <chrono>
#include <QDebug>
#include <QObject>


std::default_random_engine CharGenerator (int(std::chrono::system_clock::now().time_since_epoch().count()));

int randomChar::operator()() {
    return CharDistribution(CharGenerator);
}

c_UUID Generator::UUID;

//http://xoroshiro.di.unimi.it/
std::uint64_t Generator::xorshift64starSeed(35); /**< used with Generator::xorshift64star(void) */




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





