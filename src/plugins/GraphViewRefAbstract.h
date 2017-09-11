#ifndef GraphViewRefAbstract_H
#define GraphViewRefAbstract_H

#include <QWidget>
#include "ProjectSettings.h"
#include "StdUtilities.h"
#include "Date.h"
#include "CalibrationCurve.h"

class GraphViewRefAbstract: public QWidget
{
    Q_OBJECT
public:
    explicit GraphViewRefAbstract(QWidget* parent = nullptr):QWidget(parent),
        mMeasureColor(56, 120, 50)
    {
        setMouseTracking(true);
    }
    explicit GraphViewRefAbstract(const GraphViewRefAbstract& graph, QWidget* parent = nullptr):QWidget(parent),
        mMeasureColor(graph.mMeasureColor)
    {
        setMouseTracking(true);
        mSettings = graph.mSettings;
        mFormatFuncX = graph.mFormatFuncX;

        mTminCalib = graph.mTminCalib;
        mTmaxCalib = graph.mTmaxCalib;

        mTminDisplay = graph.mTminDisplay;
        mTmaxDisplay = graph.mTmaxDisplay;

        mTminRef = graph.mTminRef;
        mTmaxRef = graph.mTmaxRef;

    }

    void copyFrom(const GraphViewRefAbstract& graph)
    {
        mMeasureColor = graph.mMeasureColor;
        setMouseTracking(true);
        mSettings = graph.mSettings;
        mFormatFuncX = graph.mFormatFuncX;

        mTminCalib = graph.mTminCalib;
        mTmaxCalib = graph.mTmaxCalib;

        mTminDisplay = graph.mTminDisplay;
        mTmaxDisplay = graph.mTmaxDisplay;

        mTminRef = graph.mTminRef;
        mTmaxRef = graph.mTmaxRef;

    }

    virtual ~GraphViewRefAbstract() {}


    virtual void setDate(const Date& date, const ProjectSettings& settings)
    {
        mSettings = settings;
        
        mTminCalib = date.mCalibration->mTmin;
        mTmaxCalib = date.mCalibration->mTmax;
        
        mTminDisplay = qMin(mTminCalib, (double)mSettings.mTmin);
        mTmaxDisplay = qMax(mTmaxCalib, (double)mSettings.mTmax);
        
        mTminRef = date.getTminRefCurve();
        mTmaxRef = date.getTmaxRefCurve();
    }
    
    void setFormatFunctX(FormatFunc f)
    {
        mFormatFuncX = f;
    }

    void setMarginLeft(const qreal &aMarginLeft) { mGraph->setMarginLeft(aMarginLeft);}
    void setMarginRight(const qreal &aMarginRight) { mGraph->setMarginRight(aMarginRight);}
    
public slots:
    virtual void zoomX(const double min, const double max)
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

public:
    GraphView* mGraph;
};

#endif
