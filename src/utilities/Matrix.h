#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <cmath>
#include <QDataStream>
#include <QtDebug>
#include <valarray>



class CurveMap
{
public:
    unsigned _row, _column;
    std::valarray<double> data;
    std::pair<double, double> rangeY;
    std::pair<double, double> rangeX;

    double min_value;
    double max_value;

    CurveMap();
    explicit CurveMap(unsigned row, unsigned col):_row(row), _column(col), data(_row*_column), min_value(0), max_value(0) {}

    void setRangeX(double minX, double maxX) {rangeX = std::pair<double, double>(minX, maxX);}
    void setRangeY(double minY, double maxY) {rangeY = std::pair<double, double>(minY, maxY);}

    double& at(unsigned i, unsigned j) {
        try {
            return data[_row*i + j];
        }  catch (...) {
            qDebug() <<"CurveMap over ???"<<i << j;
            return data[0];
        }

    }
    double& operator()(unsigned i, unsigned j) { return data[_row*i + j];} //Set
    const double& operator()(unsigned i, unsigned j)  const { return data[_row*i + j];} //Get
    double* ptr_at(unsigned i, unsigned j) {return (begin(data) + (_row*i + j));}

    unsigned row() {return _row;}
    unsigned column() {return _column;}

    double minX() {return rangeX.first;}
    double maxX() {return rangeX.second;}
    double minY() {return rangeY.first;}
    double maxY() {return rangeY.second;}

    virtual ~CurveMap();

};

typedef std::valarray<std::valarray<long double>> Matrix2D;

QDataStream &operator<<( QDataStream& stream, const CurveMap &map );
QDataStream &operator>>( QDataStream& stream, CurveMap &map );


/*
class Matrix
{
public:
    Matrix();
   // Matrix(std::size_t N);
    //double& at(std::size_t i, std::size_t j);
    //double& operator[](std::size_t i, std::size_t j);

    virtual ~Matrix();
private:
    std::size_t N;
    std::vector<double> data;
};
*/
#endif // MATRIX_H
