#include "DoubleValidator.h"
#include <QtWidgets>
#include <QtSvg>


DoubleValidator::DoubleValidator(QObject * parent):QDoubleValidator(parent)
{
    
}

DoubleValidator::~DoubleValidator()
{
    
}

void DoubleValidator::fixup(QString& input) const
{
    double v = input.toDouble();
    if(v < bottom())
        input = QString::number(bottom());
    else if(v > top())
        input = QString::number(top());
}