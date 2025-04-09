/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

#include "Matrix.h"

#include <iostream>
#include <vector>

/*CurveMap::CurveMap()
{
    _row = 0;
    _column = 0;
    min_value = 0.;
    max_value = 0.;
    data = std::valarray<double> ();
    rangeX = std::pair<double, double> (0, 0);
    rangeY = std::pair<double, double> (0, 0);
}*/

CurveMap::~CurveMap()
{
}

/** Write Data
 */
QDataStream &operator<<( QDataStream &stream , const CurveMap &map )
{
    stream << map._row;
    stream << map._column;
    stream << map.min_value;
    stream << map.max_value;
    stream << map.rangeX;
    stream << map.rangeY;

   // stream << (quint32) map.data.size();
    for (const double* v = begin(map.data); v != end(map.data); ++v)
        stream << *v;

   return stream;
}

/** Read Data
 */
QDataStream &operator>>( QDataStream &stream, CurveMap &map )
{
    stream >> map._row;
    stream >> map._column;
    stream >> map.min_value;
    stream >>map. max_value;
    stream >> map. rangeX;
    stream >> map.rangeY;

    unsigned long siz = map._column * map._row;

    map.data = std::valarray<double>(siz);


    double v;
    for (unsigned long i = 0; i < siz; ++i) {
        stream >> v;
        map.data[i] = v ;
    }

    return stream;

}

Matrix2D initMatrix2D(size_t rows, size_t cols)
{
   return std::valarray(std::valarray<t_matrix>(cols), rows) ;
}

#pragma mark Usefull to debug
void showMatrix(const Matrix2D&  m, const std::string& str)
{
  std::cout << str << "\n";
  std::cout <<"{"<<"\n";
  for(unsigned long i = 0; i < m.size(); i++) {
      std::cout <<"{";
    for (unsigned long j = 0; j < m[0].size(); j++) {
        if (std::is_same<Matrix2D::value_type::value_type, long double>::value == true) {
            //printf(" %+20.65Lf ", m[i][j]);
            printf(" %+5.5Lf ", m[i][j]);
        }
        else if (std::is_same<Matrix2D::value_type::value_type, double>::value == true) {
            //printf(" %+20.65f ",(double) m[i][j]);
            printf(" %+5.5f ",(double) m[i][j]);
        }
        if (j<m[0].size()-1)
            std::cout<<", ";
    }
    printf(" },\n");
  }
    std::cout <<"}"<<"\n";
  printf("\n");
}

void showMatrix(const MatrixDiag& m, const std::string& str)
{
  Matrix2D tmp = initMatrix2D(m.size(), m.size());

  for(unsigned long i = 0; i < m.size(); i++) {
      tmp[i][i] = m[i];
  }
  showMatrix(tmp, str);
}

void showVector(const std::vector<t_matrix> &m, const std::string& str)
{
 std::cout << str << "\n";
 for(unsigned long i = 0; i < m.size(); i++) {
     if (std::is_same<t_matrix, long double>::value == true) {
         printf(" %20.65Lf ", m[i]);
     }
     else if (std::is_same<t_matrix, double>::value == true) {
         printf(" %8.65f ",(double) m[i]);
     }

   printf("\n");
 }
 printf("\n");
}

void showVector(const std::vector<double> &m, const std::string& str)
{
    std::cout << str << "\n";
    for(unsigned long i = 0; i < m.size(); i++) {
        //printf(" %8.65f ",(double) m[i]);
        printf(" %8.5f ",(double) m[i]);
        printf("\n");
    }
    printf("\n");
}

