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
    QMap<float, float> mData;
    QVector<float> mDataVector;
    bool mUseVectorData;
    
    QString mName;
    QPen mPen;
    bool mFillUnder;
    bool mIsHisto;
    
    bool mIsHorizontalLine;
    float mHorizontalValue;
    
    bool mIsVerticalLine;
    float mVerticalValue;
    
    bool mIsVertical;
};

#endif
