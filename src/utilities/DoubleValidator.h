#ifndef DOUBLEVALIDATOR_H
#define DOUBLEVALIDATOR_H

#include <QDoubleValidator>


class DoubleValidator: public QDoubleValidator
{
public:
    DoubleValidator(QObject * parent = 0);
    ~DoubleValidator();
    
    void fixup(QString & input) const;
};

#endif