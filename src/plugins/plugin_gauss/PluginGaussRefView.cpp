#include "PluginGaussRefView.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>


PluginGaussRefView::PluginGaussRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mGraph = new GraphView(this);
    
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->setRendering(GraphView::eHD);
    mGraph->autoAdjustYScale(true);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("x");
    mMeasureColor=QColor(56, 120, 50);
}

PluginGaussRefView::~PluginGaussRefView()
{
    
}

void PluginGaussRefView::setDate(const Date& date, const ProjectSettings& settings)
{
    QLocale locale = QLocale();
    GraphViewRefAbstract::setDate(date, settings);
    
    //Date date = d;
    
    double tminCalib = date.getTminCalib();
    double tmaxCalib = date.getTmaxCalib();
    
    double tminDisplay = qMin(tminCalib, (double)mSettings.mTmin);
    double tmaxDisplay = qMax(tmaxCalib, (double)mSettings.mTmax);
    
    mGraph->setRangeX(tminDisplay, tmaxDisplay);
    mGraph->setCurrentX(tminDisplay, tmaxDisplay);
    
    mGraph->removeAllCurves();
    mGraph->clearInfos();
    mGraph->showInfos(true);
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
        curve.mPen.setColor(Painting::mainColorDark);
        curve.mIsHisto = false;
        
        double yMin;
        double yMax;
        
        if(mode == DATE_GAUSS_MODE_NONE)
        {
          // nothing to do
        }
        else if(mode == DATE_GAUSS_MODE_EQ)
        {
            for(double t=tminDisplay; t<=tmaxDisplay; t+=mSettings.mStep)
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

            if(curves.isEmpty() || curves["G"].isEmpty()) {
                GraphZone zone;
                zone.mColor = Qt::gray;
                zone.mColor.setAlpha(25);
                zone.mXStart = mSettings.mTmin;
                zone.mXEnd = mSettings.mTmax;
                mGraph->addZone(zone);
                return;
            }


            QMap<double, double> curveG;
            QMap<double, double> curveG95Sup;
            QMap<double, double> curveG95Inf;


            double tminCalib = date.getTminCalib();
            double tmaxCalib = date.getTmaxCalib();

            double tminCurve = date.getTminRefCurve();
            double tmaxCurve = date.getTmaxRefCurve();

            double tminDisplay;
            double tmaxDisplay;


            if(mSettings.mTmin<tminCalib){
               tminDisplay = mSettings.mTmin;
            }
            else {
                tminDisplay = tminCalib;
            }

            if(tmaxCalib<mSettings.mTmax){
                   tmaxDisplay = mSettings.mTmax;
            }
            else {
                tmaxDisplay = tmaxCalib;
            }

            if(tminDisplay<tminCurve){
                GraphZone zone;
                zone.mColor = Qt::gray;
                zone.mColor.setAlpha(35);

                zone.mXStart = tminDisplay;
                zone.mXEnd = tminCurve;
                mGraph->addZone(zone);
            }

            if(tmaxCurve<tmaxDisplay){
                GraphZone zone;
                zone.mColor = Qt::gray;
                zone.mColor.setAlpha(35);

                zone.mXStart = tmaxCurve;
                zone.mXEnd = tmaxDisplay;
                mGraph->addZone(zone);
            }

            double yMin = plugin->getRefValueAt(date.mData, qMax(tminDisplay,tminCurve));
            double yMax = plugin->getRefValueAt(date.mData, qMin(tmaxDisplay,tmaxCurve));


            for(double t=tminDisplay; t<=tmaxDisplay; ++t)
            {
                if(t>tminCurve && t<tmaxCurve) {
                    double value = plugin->getRefValueAt(date.mData, t);
                    double error = plugin->getRefErrorAt(date.mData, t) * 1.96;

                    curveG[t] = value;
                    curveG95Sup[t] = value + error;
                    curveG95Inf[t] = value - error;

                    yMin = qMin(yMin, curveG95Inf[t]);
                    yMax = qMax(yMax, curveG95Sup[t]);
                }
            }

            mGraph->setRangeX(tminDisplay,tmaxDisplay);
            mGraph->setCurrentX(tminDisplay, tmaxDisplay);


            //---
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
            
            GraphCurve graphCurveG;
            graphCurveG.mName = "G";
            graphCurveG.mData = curveG;
            graphCurveG.mPen.setColor(Qt::blue);
            graphCurveG.mIsHisto = false;
            mGraph->addCurve(graphCurveG);
            
            // Display reference curve name
            mGraph->addInfo(tr("Ref : ") + ref_curve);
                     
            // ----------------------------------------------------
            
            // Adjust scale :
            yMin = qMin(yMin, age);
            yMin = floor(yMin/10)*10;
            
            yMax = qMax(yMax, age);
            yMax = ceil(yMax/10)*10;
            
            mGraph->setRangeY(yMin, yMax);
        }
        
        if(mode != DATE_GAUSS_MODE_NONE)
        {
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
