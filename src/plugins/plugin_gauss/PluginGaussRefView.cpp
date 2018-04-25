#include "PluginGaussRefView.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>


PluginGaussRefView::PluginGaussRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(56, 120, 50);

    mGraph = new GraphView(this);
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    //mGraph->setRendering(GraphView::eHD);
    mGraph->autoAdjustYScale(true);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("x");
    mGraph->setMarginBottom(mGraph->font().pointSize() * 2.2 );
}

PluginGaussRefView::~PluginGaussRefView()
{
    
}

void PluginGaussRefView::setDate(const Date& date, const ProjectSettings& settings)
{
    GraphViewRefAbstract::setDate(date, settings);
    
    double tminDisplay;
    double tmaxDisplay;

    const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
    const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);

    if (!date.isNull() && date.mIsValid) {
        const double t3 = date.getFormatedTminCalib();
        const double t4 = date.getFormatedTmaxCalib();

        tminDisplay = qMin(t1,qMin(t2,t3));
        tmaxDisplay = qMax(t1,qMax(t2,t4));

    } else {
        tminDisplay = qMin(t1, t2);
        tmaxDisplay = qMax(t1, t2);
    }

    mGraph->setRangeX(tminDisplay, tmaxDisplay);
    mGraph->setCurrentX(tminDisplay, tmaxDisplay);
    
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->showInfos(true);
    mGraph->setFormatFunctX(0);
    
    if (!date.isNull()) {
        const double age = date.mData.value(DATE_GAUSS_AGE_STR).toDouble();
        const double error = date.mData.value(DATE_GAUSS_ERROR_STR).toDouble();
        const double a = date.mData.value(DATE_GAUSS_A_STR).toDouble();
        const double b = date.mData.value(DATE_GAUSS_B_STR).toDouble();
        const double c = date.mData.value(DATE_GAUSS_C_STR).toDouble();
        const QString mode = date.mData.value(DATE_GAUSS_MODE_STR).toString();
        const QString ref_curve = date.mData.value(DATE_GAUSS_CURVE_STR).toString();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------

        const double tminRef = date.getFormatedTminRefCurve();
        const double tmaxRef = date.getFormatedTmaxRefCurve();

        GraphCurve curve;
        curve.mName = "Reference";
        curve.mPen.setColor(Painting::mainColorDark);
        curve.mIsHisto = false;
        
        double yMin (tminDisplay);
        double yMax (tmaxDisplay);
        
        if (mode == DATE_GAUSS_MODE_NONE) {
          // nothing to do
            
        } else if (mode == DATE_GAUSS_MODE_EQ) {
            QMap<double,double> refCurve;
            double stepDisplay (tmaxDisplay - tminDisplay);
            if (a>0)
                stepDisplay = stepDisplay/1000.;

            for (double t = tminDisplay; t<=tmaxDisplay; t += stepDisplay) {
                const double tRaw = DateUtils::convertFromAppSettingsFormat(t);
                refCurve[t] = a * tRaw * tRaw + b * tRaw + c;
            }
            curve.mData = refCurve;
            mGraph->addCurve(curve);

          yMin = qMin( refCurve.first(), refCurve.last());
          yMax = qMax( refCurve.first(), refCurve.last());
            // Y scale and RangeY are define in graphView::zommX()


            
        } else if (mode == DATE_GAUSS_MODE_CURVE) {
            PluginGauss* plugin = (PluginGauss*)date.mPlugin;
            
            const RefCurve& curve = plugin->mRefCurves.value(ref_curve);
            
            if (curve.mDataMean.isEmpty()) {
                GraphZone zone;
                zone.mColor = Qt::gray;
                zone.mColor.setAlpha(75);
                zone.mXStart = tminDisplay;
                zone.mXEnd = tmaxDisplay;
                zone.mText = tr("No reference data");
                mGraph->addZone(zone);
                return;
            }

            if (tminDisplay < tminRef) {
                GraphZone zone;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mXStart = tminDisplay;
                zone.mXEnd = tminRef;
                zone.mText = tr("Outside reference area");
                mGraph->addZone(zone);
            }

            if (tmaxRef < tmaxDisplay) {
                GraphZone zone;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mXStart = tmaxRef;
                zone.mXEnd = tmaxDisplay;
                zone.mText = tr("Outside reference area");
                mGraph->addZone(zone);
            }

            const double t0 = DateUtils::convertFromAppSettingsFormat(qMax(tminDisplay, tminRef));
            yMin = plugin->getRefValueAt(date.mData, t0);
            yMax = yMin;

            QMap<double, double> curveG;
            QMap<double, double> curveG95Sup;
            QMap<double, double> curveG95Inf;

            /*
             *  Define the first point, which is often not the first point of the ref curve
             */

            if (tminDisplay>curve.mDataMean.firstKey() && tminDisplay<curve.mDataMean.lastKey()) {
                // This actually return the iterator with the nearest greater key !!!
                 QMap<double, double>::const_iterator iter = curve.mDataMean.lowerBound(tminDisplay);
                 // the higher value must be mTmax.
                 double v;
                 if (iter != curve.mDataError.constBegin()) {
                     const double t_upper = iter.key();
                     const double v_upper = iter.value();
                     --iter;
                     const double t_under = iter.key();
                     const double v_under = iter.value();

                     v = interpolate(tminDisplay, t_under, t_upper, v_under, v_upper);
                 } else
                     v = iter.value();

                 const double error = plugin->getRefErrorAt(date.mData, tminDisplay, mode) * 1.96;

                 curveG[tminDisplay] = v;
                 curveG95Sup[tminDisplay] = v + error;
                 curveG95Inf[tminDisplay] = v - error;

                 yMin = qMin(yMin, curveG95Inf.value(tminDisplay));
                 yMax = qMax(yMax, curveG95Sup.value(tminDisplay));
            }


            for ( QMap<double, double>::const_iterator &&iPt = curve.mDataMean.cbegin();  iPt!=curve.mDataMean.cend(); ++iPt) {
                const double t (iPt.key());
                const double tDisplay = DateUtils::convertToAppSettingsFormat(t);
                if (tDisplay>=tminDisplay && tDisplay<=tmaxDisplay) {
                    const double error = plugin->getRefErrorAt(date.mData, t, mode) * 1.96;

                    curveG[tDisplay] = iPt.value();
                    curveG95Sup[tDisplay] = iPt.value() + error;
                    curveG95Inf[tDisplay] = iPt.value() - error;

                    yMin = qMin(yMin, curveG95Inf.value(tDisplay));
                    yMax = qMax(yMax, curveG95Sup.value(tDisplay));
                }
            }
            if (tmaxDisplay>curve.mDataMean.firstKey() && tmaxDisplay<curve.mDataMean.lastKey()) {
                 QMap<double, double>::const_iterator iter = curve.mDataMean.lowerBound(tmaxDisplay);
                 double v;
                 if (iter != curve.mDataError.constBegin()) {
                     const double t_upper = iter.key();
                     const double v_upper = iter.value();
                     --iter;
                     const double t_under = iter.key();
                     const double v_under = iter.value();

                     v = interpolate(tmaxDisplay, t_under, t_upper, v_under, v_upper);
                 } else
                     v = iter.value();

                 const double error = plugin->getRefErrorAt(date.mData, tmaxDisplay, mode) * 1.96;

                 curveG[tmaxDisplay] = v;
                 curveG95Sup[tmaxDisplay] = v + error;
                 curveG95Inf[tmaxDisplay] = v - error;

                 yMin = qMin(yMin, curveG95Inf.value(tmaxDisplay));
                 yMax = qMax(yMax, curveG95Sup.value(tmaxDisplay));
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

        }
        
        if (mode != DATE_GAUSS_MODE_NONE) {
            yMin = qMin(yMin, age - error * 1.96);
            yMax = qMax(yMax, age + error * 1.96);

            //mGraph->setRangeY(yMin, yMax);
            
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
            const double step = (yMax - yMin) / 5000.;
            QMap<double, double> measureCurve;
            for (double t=yMin; t<yMax; t += step) {
                double v = exp(-0.5 * pow((t - age) / error, 2.));
                measureCurve[t] = v;
            }
            measureCurve = normalize_map(measureCurve);

            curveMeasure.mData = measureCurve;
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
}

void PluginGaussRefView::zoomX(const double min, const double max)
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

