#ifndef GraphZone_H
#define GraphZone_H

#include <QColor>
#include <QString>


class GraphZone
{
public:
    explicit GraphZone();
    virtual ~GraphZone();

public:
    double mXStart;
    double mXEnd;
    QColor mColor;
    QString mText;
};

#endif
