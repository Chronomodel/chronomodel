#ifndef GraphViewRefAbstract_H
#define GraphViewRefAbstract_H

#include <QWidget>
#include "ProjectSettings.h"
#include "StdUtilities.h"
#include "Date.h"

class GraphView;


class GraphViewRefAbstract: public QWidget
{
    Q_OBJECT
public:
    explicit GraphViewRefAbstract(QWidget* parent = 0):QWidget(parent),mMeasureColor(56, 120, 50)
    {
        setMouseTracking(true);
    }
    virtual ~GraphViewRefAbstract(){}

    virtual void setDate(const Date& date, const ProjectSettings& settings)
    {
        mSettings = settings;
        
        mTminCalib = date.getTminCalib();
        mTmaxCalib = date.getTmaxCalib();
        
        mTminDisplay = qMin(mTminCalib, (double)mSettings.mTmin);
        mTmaxDisplay = qMax(mTmaxCalib, (double)mSettings.mTmax);
        
        mTminRef = date.getTminRefCurve();
        mTmaxRef = date.getTmaxRefCurve();
    }
    
    void setFormatFunctX(FormatFunc f)
    {
        mFormatFuncX = f;
    }
    
    
public slots:
    virtual void zoomX(double min, double max)
    {
        Q_UNUSED(min);
        Q_UNUSED(max);
    }
    virtual void setMarginRight(const int margin)
    {
        Q_UNUSED(margin);
    }
protected:
    ProjectSettings mSettings;
    QColor mMeasureColor;
    FormatFunc mFormatFuncX;
    
    double mTminCalib;
    double mTmaxCalib;
    
    double mTminRef;
    double mTmaxRef;
    
    double mTminDisplay;
    double mTmaxDisplay;
};

#endif
