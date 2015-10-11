#include "PluginGaussRefView.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include <QtWidgets>


PluginGaussRefView::PluginGaussRefView(QWidget* parent):GraphViewRefAbstract(parent),
mGraph(0)
{
    mGraph = new GraphView(this);
    
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->setRendering(GraphView::eHD);
    mGraph->autoAdjustYScale(true);
    mMeasureColor=QColor(56, 120, 50);
}

PluginGaussRefView::~PluginGaussRefView()
{
    
}

void PluginGaussRefView::setDate(const Date& d, const ProjectSettings& settings)
{
    QLocale locale=QLocale();
    GraphViewRefAbstract::setDate(d, settings);
    
    Date date = d;
    
    mGraph->removeAllCurves();
    mGraph->clearInfos();
    mGraph->showInfos(true);
    mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    mGraph->setCurrentX(mSettings.mTmin, mSettings.mTmax);
    mGraph->setFormatFunctX(mFormatFuncX);
    
    if(!date.isNull())
    {
        double age = date.mData.value(DATE_GAUSS_AGE_STR).toDouble();
        double error = date.mData.value(DATE_GAUSS_ERROR_STR).toDouble();
        double a = date.mData.value(DATE_GAUSS_A_STR).toDouble();
        double b = date.mData.value(DATE_GAUSS_B_STR).toDouble();
        double c = date.mData.value(DATE_GAUSS_C_STR).toDouble();
        QString mode = date.mData.value(DATE_GAUSS_MODE_STR).toString();
        QString ref_curve = date.mData.value(DATE_GAUSS_CURVE_STR).toString();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------
        
        GraphCurve curve;
        curve.mName = "Reference";
        curve.mPen.setColor(Qt::blue);
        curve.mIsHisto = false;
        
        double yMin;
        double yMax;
        
        if(mode == DATE_GAUSS_MODE_NONE)
        {
            for(double t=mSettings.mTmin; t<=mSettings.mTmax; t+=mSettings.mStep)
                curve.mData[t] = t;
            mGraph->addCurve(curve);
            
            // Adjust scale :
            yMin = mSettings.mTmin;
            yMax = mSettings.mTmax;
            
            yMin = qMin(yMin, age);
            yMax = qMax(yMax, age);
            
            mGraph->setRangeY(yMin, yMax);
        }
        else if(mode == DATE_GAUSS_MODE_EQ)
        {
            for(double t=mSettings.mTmin; t<=mSettings.mTmax; t+=mSettings.mStep)
                curve.mData[t] = a * t * t + b * t + c;
            mGraph->addCurve(curve);
            
            // Adjust scale :
            yMin = map_min_value(curve.mData);
            yMax = map_max_value(curve.mData);
            
            yMin = qMin(yMin, age);
            yMax = qMax(yMax, age);
            
            mGraph->setRangeY(yMin, yMax);
        }
        else if(mode == DATE_GAUSS_MODE_CURVE)
        {
            PluginGauss* plugin = (PluginGauss*)date.mPlugin;
            
            const QMap<QString, QMap<double, double> >& curves = plugin->getRefData(ref_curve);
            
            
            QMap<double, double> curveG;
            QMap<double, double> curveG95Sup;
            QMap<double, double> curveG95Inf;
            
            yMin = curves["G95Inf"][mSettings.mTmin];
            yMax = curves["G95Sup"][mSettings.mTmin];
            
            double tMinGraph=curves["G"].firstKey()>mSettings.mTmin ? curves["G"].firstKey(): mSettings.mTmin;
            double tMaxGraph=curves["G"].lastKey()<mSettings.mTmax  ? curves["G"].lastKey() : mSettings.mTmax;
            
            for(double t=tMinGraph; t<=tMaxGraph; ++t) {
                curveG[t] = curves["G"][t];
                curveG95Sup[t] = curves["G95Sup"][t];
                curveG95Inf[t] = curves["G95Inf"][t];
                
                yMin = qMin(yMin, curveG95Inf[t]);
                yMax = qMax(yMax, curveG95Sup[t]);
            }
            
            GraphCurve graphCurveG;
            graphCurveG.mName = "G";
            graphCurveG.mData = curveG;
            graphCurveG.mPen.setColor(Qt::blue);
            graphCurveG.mIsHisto = false;
            mGraph->addCurve(graphCurveG);
            
            GraphCurve graphCurveG95Sup;
            graphCurveG95Sup.mName = "G95Sup";
            graphCurveG95Sup.mData = curveG95Sup;
            graphCurveG95Sup.mPen.setColor(QColor(180, 180, 180));
            graphCurveG95Sup.mIsHisto = false;
            mGraph->addCurve(graphCurveG95Sup);
            
            GraphCurve graphCurveG95Inf;
            graphCurveG95Inf.mName = "G95Inf";
            graphCurveG95Inf.mData = curveG95Inf;
            graphCurveG95Inf.mPen.setColor(QColor(180, 180, 180));
            graphCurveG95Inf.mIsHisto = false;
            mGraph->addCurve(graphCurveG95Inf);
            
            // Display reference curve name
            mGraph->addInfo(tr("Ref : ") + ref_curve);
            
            // Adjust scale :
            yMin = qMin(yMin, age);
            yMin = floor(yMin/10)*10;
            
            yMax = qMax(yMax, age);
            yMax = ceil(yMax/10)*10;
            
            mGraph->setRangeY(yMin, yMax);
        }
        
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
        mGraph->addInfo(tr("Measure : ") + locale.toString(age) + " ± " + locale.toString(error));
        
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

void PluginGaussRefView::zoomX(double min, double max)
{
    mGraph->zoomX(min, max);
}

void PluginGaussRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(0, 0, width(), height());
}

#endif
