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
    
    void setPen(QPen pen);
    QVector<double> getVectorDataInRange(double subMin, double subMax, double min, double max) const;
    QMap<double, double> getMapDataInRange(double subMin, double subMax, double min, double max) const;

public:
    QMap<double, double> mData;
    
    QString mName;
    QPen mPen;
    QBrush mBrush;
    bool mIsHisto;
    bool mIsRectFromZero; // draw a vertical line when graph value leaves 0 : usefull for HPD and Typo!
    
    bool mUseVectorData;
    QVector<double> mDataVector;
    
    bool mIsHorizontalLine;
    double mHorizontalValue;
    
    bool mIsVerticalLine;
    double mVerticalValue;
    
    bool mIsHorizontalSections;
    QList<QPair<double, double> > mSections;
    
    bool mIsVertical;
    
    bool mVisible;
};

#endif
