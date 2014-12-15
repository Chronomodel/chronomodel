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
    
    QString mName;
    QPen mPen;
    bool mFillUnder;
    bool mIsHisto;
    bool mIsRectFromZero; // draw a vertical line when graph value leaves 0 : usefull for HPD and Typo!
    
    bool mUseVectorData;
    QVector<float> mDataVector;
    
    bool mIsHorizontalLine;
    float mHorizontalValue;
    
    bool mIsVerticalLine;
    float mVerticalValue;
    
    bool mIsHorizontalSections;
    QList<QPair<float, float>> mSections;
    
    bool mIsVertical;
};

#endif
