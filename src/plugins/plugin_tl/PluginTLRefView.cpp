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
    
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->setRendering(GraphView::eHD);
    mGraph->autoAdjustYScale(true);
    
    mMeasureColor=QColor(56, 120, 50);
}

PluginTLRefView::~PluginTLRefView()
{
    
}

void PluginTLRefView::setDate(const Date& d, const ProjectSettings& settings)
{
    GraphViewRefAbstract::setDate(d, settings);
    Date date = d;
    
    mGraph->removeAllCurves();
    mGraph->clearInfos();
    mGraph->showInfos(true);
    mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    mGraph->setCurrentX(mSettings.mTmin, mSettings.mTmax);
    
    if(!date.isNull())
    {
        double age = date.mData.value(DATE_TL_AGE_STR).toDouble();
        double error = date.mData.value(DATE_TL_ERROR_STR).toDouble();
        double ref_year = date.mData.value(DATE_TL_REF_YEAR_STR).toDouble();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------
        
        GraphCurve curve;
        curve.mName = "Reference";
        curve.mPen.setColor(Qt::blue);
        curve.mIsHisto = false;
        
        for(double t=mSettings.mTmin; t<=mSettings.mTmax; t+=mSettings.mStep)
            curve.mData[t] = ref_year - t;
        mGraph->addCurve(curve);
        
        // ----------------------------------------------
        
        double yMin = map_min_value(curve.mData);
        double yMax = map_max_value(curve.mData);
        
        yMin = qMin(yMin, age);       
        yMax = qMax(yMax, age);
        
        mGraph->setRangeY(yMin, yMax);
        
        // ----------------------------------------------
        //  Measure curve
        // ----------------------------------------------
        
        GraphCurve curveMeasure;
        curveMeasure.mName = "Measure";
        
        curveMeasure.mPen.setColor(mMeasureColor);
        QColor curveColor(mMeasureColor);
        curveColor.setAlpha(50);
        curveMeasure.mBrush = curveColor;
        
        curveMeasure.mIsVertical = true;
        curveMeasure.mIsHisto = false;
        
        // 5000 pts are used on vertical measure
        // because the y scale auto adjusts depending on x zoom.
        // => the visible part of the measure may be very reduced !
        double step = (yMax - yMin) / 5000.;
        for(double t=yMin; t<yMax; t += step)
        {
            double v = exp(-0.5 * pow((t - age) / error, 2));
            curveMeasure.mData[t] = v;
        }
        curveMeasure.mData = normalize_map(curveMeasure.mData);
        mGraph->addCurve(curveMeasure);
        
        // Write measure value :
        mGraph->addInfo(tr("Age : ") + QString::number(age) + " ± " + QString::number(error) + "(" + tr("Ref year") + " : " + ref_year + ")");
        
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

void PluginTLRefView::zoomX(double min, double max)
{
    mGraph->zoomX(min, max);
}

void PluginTLRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(0, 0, width(), height());
}

#endif
