#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <cmath>
#include <QDataStream>
#include <QtDebug>

class CurveMap
{
public:
    quint64 _row, _column;
    std::vector<double> data;
    std::pair<double, double> rangeY;
    std::pair<double, double> rangeX;

    double min_value;
    double max_value;

    CurveMap();
    explicit CurveMap(quint64 row, quint64 col):_row(row), _column(col), data(_row*_column), min_value(0), max_value(0) {}

    void setRangeX(double minX, double maxX) {rangeX = std::pair<double, double>(minX, maxX);}
    void setRangeY(double minY, double maxY) {rangeY = std::pair<double, double>(minY, maxY);}

    double& at(quint64 i, quint64 j) {
        try {
            return data.at(_row*i + j);
        }  catch (...) {
            qDebug() <<"CurveMap over ???"<<i << j;
            return data.at(0);
        }

    }
    double& operator()(quint64 i, quint64 j) { return data[_row*i + j];} //Set
    const double& operator()(quint64 i, quint64 j)  const { return data.at(_row*i + j);} //Get
    std::vector<double>::iterator ptr_at(quint64 i, quint64 j) {return data.begin() + (_row*i + j);}

    quint64 row(){return _row;}
    quint64 column(){return _column;}

    double minX() {return rangeX.first;}
    double maxX() {return rangeX.second;}
    double minY() {return rangeY.first;}
    double maxY() {return rangeY.second;}

    virtual ~CurveMap();

};

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
