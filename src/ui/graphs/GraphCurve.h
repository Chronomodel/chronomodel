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
    
    bool mUseVectorData; // Used for traces, correlations and acceptations.
    QVector<double> mDataVector;
    
    bool mIsHorizontalLine; // Used for calib measures, 44% targets, quartiles, ...
    double mHorizontalValue;
    
    bool mIsVerticalLine; // Used for bounds (in results view)
    double mVerticalValue;
    
    bool mIsHorizontalSections; // Used for bounds (in scene and property views) and typo (scene view)
    bool mIsTopLineSections; // Used for credibilities (and "one day" for phases alpha/beta interval??)
    QList<QPair<double, double> > mSections;
    
    bool mIsVertical;
    
    bool mVisible;
};

#endif
