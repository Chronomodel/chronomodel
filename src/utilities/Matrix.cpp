#include "Matrix.h"
#include <vector>
#include <cmath>

CurveMap::CurveMap()
{
    _row = 0;
    _column = 0;
    min_value = 0.;
    max_value = 0.;
    data = std::vector<double> ();
    rangeX = std::pair<double, double> (0, 0);
    rangeY = std::pair<double, double> (0, 0);
}
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
    for (std::vector<double>::const_iterator v = map.data.cbegin(); v != map.data.cend(); ++v)
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

    if (map.data.size() > 0)
        map.data.clear();
    else
        map.data = std::vector<double>();

    map.data.reserve(siz);

    double v;
    for (unsigned long i = 0; i < siz; ++i) {
        stream >> v;
        map.data.push_back(v);
    }

    return stream;

}


/*
Matrix::Matrix()
{

}

*/
