#ifndef GraphCurve_H
#define GraphCurve_H

#include <QMap>
#include <QString>
#include <QPen>

typedef double type_data;

class GraphCurve
{
public:
    explicit GraphCurve();
    virtual ~GraphCurve();
    
    void setPen(QPen pen);

    QMap<type_data, type_data> mData;
    
    QString mName;
    QPen mPen;
    QBrush mBrush;
    bool mIsHisto;
    bool mIsRectFromZero; // draw a vertical line when graph value leaves 0 : usefull for HPD and Typo!
    
    bool mUseVectorData; // Used for traces, correlations and acceptations.
    QVector<type_data> mDataVector;
    
    bool mIsHorizontalLine; // Used for calib measures, 44% targets, quartiles, ...
    type_data mHorizontalValue;
    
    bool mIsVerticalLine; // Used for bounds (in results view)
    type_data mVerticalValue;
    
    bool mIsHorizontalSections; // Used for bounds (in scene and property views) and typo (scene view)
    bool mIsTopLineSections; // Used for credibilities (and "one day" for phases alpha/beta interval??)
    QList<QPair<type_data, type_data> > mSections;
    
    bool mIsVertical;
    
    bool mVisible;
};

#endif

