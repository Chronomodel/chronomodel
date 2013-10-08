#include "DatationC14.h"
#include <QtCore>
#include <QtGui>
#include <QColorDialog>
#include <cmath>

DatationC14::DatationC14()
{

}

const QString DatationC14::getMenuName() const
{
    return QString("Datation C14");
}

void DatationC14::showDialog()
{
    const QColor newColor = QColorDialog::getColor(QColor(0, 0, 0));
}

const QVector<double> DatationC14::getDensity() const
{
    QVector<double> density(2000);
    for(double i=-10; i<10; i+=0.01)
    {
        density.append(exp(-i*i/2.)/(2.*M_PI));
    }
    return density;
}
