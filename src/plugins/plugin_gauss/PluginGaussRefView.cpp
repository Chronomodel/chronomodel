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
    
    const float t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
    const float t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);
    const float t3 = date.getFormatedTminCalib();
    const float t4 = date.getFormatedTmaxCalib();

    float  tminDisplay = qMin(t1,qMin(t2,t3));
    float  tmaxDisplay = qMax(t1,qMax(t2,t4));
    

    mGraph->setRangeX(tminDisplay, tmaxDisplay);
    mGraph->setCurrentX(tminDisplay, tmaxDisplay);
    
    mGraph->removeAllCurves();
    mGraph->clearInfos();
    mGraph->showInfos(true);
    mGraph->setFormatFunctX(0);
    
    if(!date.isNull()) {
        const float age = date.mData.value(DATE_GAUSS_AGE_STR).toDouble();
        const float error = date.mData.value(DATE_GAUSS_ERROR_STR).toDouble();
        const float a = date.mData.value(DATE_GAUSS_A_STR).toDouble();
        const float b = date.mData.value(DATE_GAUSS_B_STR).toDouble();
        const float c = date.mData.value(DATE_GAUSS_C_STR).toDouble();
        const QString mode = date.mData.value(DATE_GAUSS_MODE_STR).toString();
        const QString ref_curve = date.mData.value(DATE_GAUSS_CURVE_STR).toString();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------

        const float tminRef = date.getFormatedTminRefCurve();
        const float tmaxRef = date.getFormatedTmaxRefCurve();

        GraphCurve curve;
        curve.mName = "Reference";
        curve.mPen.setColor(Painting::mainColorDark);
        curve.mIsHisto = false;
        
        float yMin = tminDisplay;
        float yMax = tmaxDisplay;
        
        if(mode == DATE_GAUSS_MODE_NONE){
          // nothing to do
            
        } else if (mode == DATE_GAUSS_MODE_EQ) {
            QMap<float,float> refCurve;
            
            for(float t=tminDisplay; t<=tmaxDisplay; t+=mSettings.mStep) {
                const float tRaw = DateUtils::convertFromAppSettingsFormat(t);
                refCurve[t] = a * tRaw * tRaw + b * tRaw + c;
            }
            curve.mData = refCurve;
            mGraph->addCurve(curve);
            
            // Adjust scale :
            yMin = map_min_value(refCurve);
            yMax = map_max_value(refCurve);
            
        } else if (mode == DATE_GAUSS_MODE_CURVE) {
            PluginGauss* plugin = (PluginGauss*)date.mPlugin;
            
            const RefCurve& curve = plugin->mRefCurves.value(ref_curve);
            
            if (curve.mDataMean.isEmpty()) {
                GraphZone zone;
                zone.mColor = Qt::gray;
                zone.mColor.setAlpha(25);
                zone.mXStart = tminDisplay;
                zone.mXEnd = tmaxDisplay;
                zone.mText = tr("No reference data");
                mGraph->addZone(zone);
                return;
            }

            if (tminDisplay < tminRef) {
                GraphZone zone;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(35);
                zone.mXStart = tminDisplay;
                zone.mXEnd = tminRef;
                zone.mText = tr("Outside reference area");
                mGraph->addZone(zone);
            }

            if (tmaxRef < tmaxDisplay) {
                GraphZone zone;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(35);
                zone.mXStart = tmaxRef;
                zone.mXEnd = tmaxDisplay;
                zone.mText = tr("Outside reference area");
                mGraph->addZone(zone);
            }

            const float t0 = DateUtils::convertFromAppSettingsFormat(qMax(tminDisplay, tminRef));
            yMin = plugin->getRefValueAt(date.mData, t0);
            yMax = yMin;

            QMap<float, float> curveG;
            QMap<float, float> curveG95Sup;
            QMap<float, float> curveG95Inf;
            
            for(float t=tminDisplay; t<=tmaxDisplay; ++t) {
                if (t > tminRef && t < tmaxRef) {
                    const float tRaw = DateUtils::convertFromAppSettingsFormat(t);
                    const float value = plugin->getRefValueAt(date.mData, tRaw);
                    const float error = plugin->getRefErrorAt(date.mData, tRaw, mode) * 1.96;

                    curveG[t] = value;
                    curveG95Sup[t] = value + error;
                    curveG95Inf[t] = value - error;

                    yMin = qMin(yMin, curveG95Inf.value(t));
                    yMax = qMax(yMax, curveG95Sup.value(t));
                }
            }

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
        }
        
        if(mode != DATE_GAUSS_MODE_NONE) {
            yMin = qMin(yMin, age - error * 1.96f);
            yMax = qMax(yMax, age + error * 1.96f);
            
            //qDebug() << "ymin : " << yMin << ", ymax : " << yMax;
            
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
            const float step = (yMax - yMin) / 5000.f;
            QMap<float,float> measureCurve;
            for(float t=yMin; t<yMax; t += step)
            {
                float v = exp(-0.5 * pow((t - age) / error, 2));
                //curveMeasure.mData[t] = v;
                measureCurve[t] = v;
            }
            measureCurve = normalize_map(measureCurve);
            //curveMeasure.mData = normalize_map(curveMeasure.mData);
            curveMeasure.mData = measureCurve;
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

void PluginGaussRefView::zoomX(const float min, const float max)
{
    mGraph->zoomX(min, max);
}
void PluginGaussRefView::setMarginRight(const int margin)
{
    mGraph->setMarginRight(margin);
}
void PluginGaussRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(0, 0, width(), height());
}

#endif

