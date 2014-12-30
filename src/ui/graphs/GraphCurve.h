#ifndef GraphCurve_H
#define GraphCurve_H

#include <QMap>
#include <QString>
#include <QPen>


class GraphCurve
{
public:
    explicit GraphCurve();
    virtual ~GraphCurve();

public:
    QMap<double, double> mData;
    
    QString mName;
    QPen mPen;
    bool mFillUnder;
    bool mIsHisto;
    bool mIsRectFromZero; // draw a vertical line when graph value leaves 0 : usefull for HPD and Typo!
    
    bool mUseVectorData;
    QVector<double> mDataVector;
    
    bool mIsHorizontalLine;
    double mHorizontalValue;
    
    bool mIsVerticalLine;
    double mVerticalValue;
    
    bool mIsHorizontalSections;
    QList<QPair<double, double>> mSections;
    
    bool mIsVertical;
};

#endif
