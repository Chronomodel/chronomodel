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

    double& at(unsigned c, unsigned r) { // beCarefull invertion of row and column between definiton of CurveMap() and at()
        try {
            return data[_row*c + r];
        }  catch (...) {
            qDebug() <<"CurveMap over ???"<<c << r;
            return data[0];
        }

    }
    double& operator()(unsigned c, unsigned r) { return data[_row*c + r];} //Set
    const double& operator()(unsigned c, unsigned r)  const { return data[_row*c + r];} //Get
    double* ptr_at(unsigned c, unsigned r) {return (begin(data) + (_row*c + r));}

    unsigned row() {return _row;}
    unsigned column() {return _column;}

    double minX() const {return rangeX.first;}
    double maxX() const {return rangeX.second;}
    double minY() const {return rangeY.first;}
    double maxY() const {return rangeY.second;}

    virtual ~CurveMap();

};

typedef std::valarray<std::valarray<double>> Matrix2D;
Matrix2D initMatrix2D(size_t rows, size_t cols);

QDataStream &operator<<( QDataStream &stream, const CurveMap &map );
QDataStream &operator>>( QDataStream &stream, CurveMap &map );

void showMatrix(const Matrix2D & m, const std::string& str="");


void showVector(const std::vector<double> & m, const std::string &str="");
#endif // MATRIX_H
