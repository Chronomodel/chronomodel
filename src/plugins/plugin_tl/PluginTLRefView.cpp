#include "PluginTLRefView.h"
#if USE_PLUGIN_14C

#include "PluginTL.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include <QtWidgets>


PluginTLRefView::PluginTLRefView(QWidget* parent):GraphViewRefAbstract(parent),
mGraph(0)
{
    mGraph = new GraphView(this);
    mGraph->showYValues(true);
    mGraph->showAxis(false);
}

PluginTLRefView::~PluginTLRefView()
{
    
}

void PluginTLRefView::setDate(const Date& d, const ProjectSettings& settings)
{
    GraphViewRefAbstract::setDate(d, settings);
    Date date = d;
    
    mGraph->removeAllCurves();
    mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    
    if(!date.isNull())
    {
        float age = date.mData.value(DATE_TL_AGE_STR).toDouble();
        float error = date.mData.value(DATE_TL_ERROR_STR).toDouble();
        float ref_year = date.mData.value(DATE_TL_REF_YEAR_STR).toDouble();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------
        
        GraphCurve curve;
        curve.mName = "Reference";
        curve.mPen.setColor(Qt::blue);
        
        for(int t=mSettings.mTmin; t<=mSettings.mTmax; t+=mSettings.mStep)
            curve.mData[t] = ref_year - t;
        mGraph->addCurve(curve);
        
        // ----------------------------------------------
        
        float yMin = map_min_value(curve.mData);
        float yMax = map_max_value(curve.mData);
        
        yMin = qMin(yMin, age);
        yMax = qMax(yMax, age);
        
        mGraph->setRangeY(yMin, yMax);
        
        // ----------------------------------------------
        //  Measure curve
        // ----------------------------------------------
        
        GraphCurve curveMeasure;
        curveMeasure.mName = "Measure";
        curveMeasure.mPen.setColor(mMeasureColor);
        curveMeasure.mFillUnder = true;
        curveMeasure.mIsVertical = true;
        
        for(int t=yMin; t<yMax; ++t)
        {
            float v = exp(-0.5 * pow((t - age) / error, 2));
            curveMeasure.mData[t] = v;
        }
        curveMeasure.mData = normalize_map(curveMeasure.mData);
        mGraph->addCurve(curveMeasure);
        
        // ----------------------------------------------
        //  Error on measure
        // ----------------------------------------------
        
        GraphCurve curveMeasureAvg;
        curveMeasureAvg.mName = "MeasureAvg";
        curveMeasureAvg.mPen.setColor(mMeasureColor);
        curveMeasureAvg.mPen.setStyle(Qt::SolidLine);
        curveMeasureAvg.mIsHorizontalLine = true;
        
        GraphCurve curveMeasureSup;
        curveMeasureSup.mName = "MeasureSup";
        curveMeasureSup.mPen.setColor(mMeasureColor);
        curveMeasureSup.mPen.setStyle(Qt::DashLine);
        curveMeasureSup.mIsHorizontalLine = true;
        
        GraphCurve curveMeasureInf;
        curveMeasureInf.mName = "MeasureInf";
        curveMeasureInf.mPen.setColor(mMeasureColor);
        curveMeasureInf.mPen.setStyle(Qt::DashLine);
        curveMeasureInf.mIsHorizontalLine = true;
        
        curveMeasureAvg.mHorizontalValue = age;
        curveMeasureSup.mHorizontalValue = age + error;
        curveMeasureInf.mHorizontalValue = age - error;
        
        mGraph->addCurve(curveMeasureAvg);
        mGraph->addCurve(curveMeasureSup);
        mGraph->addCurve(curveMeasureInf);
    }
}

void PluginTLRefView::zoomX(float min, float max)
{
    mGraph->zoomX(min, max);
}

void PluginTLRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(0, 0, width(), height());
}

#endif
