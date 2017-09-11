#include "PluginMagRefView.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>


PluginMagRefView::PluginMagRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(56, 120, 50);

    mGraph = new GraphView(this);
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->setRendering(GraphView::eHD);
    mGraph->autoAdjustYScale(true);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("value");
    mGraph->setMarginBottom(mGraph->font().pointSizeF() + 10. );

}

PluginMagRefView::~PluginMagRefView()
{
    
}

void PluginMagRefView::setDate(const Date& date, const ProjectSettings& settings)
{
    //QLocale locale=QLocale();
    GraphViewRefAbstract::setDate(date, settings);

    double tminDisplay;
    double tmaxDisplay;
    {
        const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
        const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);
        const double t3 = date.getFormatedTminCalib();
        const double t4 = date.getFormatedTmaxCalib();

        tminDisplay = qMin(t1,qMin(t2,t3));
        tmaxDisplay = qMax(t1,qMax(t2,t4));
    }

    mGraph->setRangeX(tminDisplay, tmaxDisplay);
    mGraph->setCurrentX(tminDisplay, tmaxDisplay);

    mGraph->removeAllCurves();
    mGraph->clearInfos();
    mGraph->showInfos(true);
    mGraph->setFormatFunctX(0);
    
    if (!date.isNull()) {
        bool is_inc = date.mData.value(DATE_AM_IS_INC_STR).toBool();
        bool is_dec = date.mData.value(DATE_AM_IS_DEC_STR).toBool();
        bool is_int = date.mData.value(DATE_AM_IS_INT_STR).toBool();
        
        double inc = date.mData.value(DATE_AM_INC_STR).toDouble();
        double dec = date.mData.value(DATE_AM_DEC_STR).toDouble();
        double intensity = date.mData.value(DATE_AM_INTENSITY_STR).toDouble();
        double alpha = date.mData.value(DATE_AM_ERROR_STR).toDouble();
        QString ref_curve = date.mData.value(DATE_AM_REF_CURVE_STR).toString().toLower();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------

        double tminRef = date.getFormatedTminRefCurve();
        double tmaxRef = date.getFormatedTmaxRefCurve();

        QColor color2(150, 150, 150);
        
        PluginMag* plugin = (PluginMag*)date.mPlugin;
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

        if (tminDisplay < tminRef){
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

        const double t0 = DateUtils::convertFromAppSettingsFormat(qMax(tminDisplay, tminRef));
        double yMin = plugin->getRefValueAt(date.mData, t0);
        double yMax = yMin;

        QMap<double, double> curveG;
        QMap<double, double> curveG95Sup;
        QMap<double, double> curveG95Inf;

        /*
         * We need to skim the real map to fit with the real value of the calibration curve
         * The graphView function does the interpolation between the real point
         */
        for ( QMap<double, double>::const_iterator &&iPt = curve.mDataMean.cbegin();  iPt!=curve.mDataMean.cend(); ++iPt) {
            const double t (iPt.key());
            const double tDisplay = DateUtils::convertToAppSettingsFormat(t);
            if (tDisplay>tminDisplay && tDisplay<tmaxDisplay) {
                const double error = plugin->getRefErrorAt(date.mData, t) * 1.96;

                curveG[t] = iPt.value();
                curveG95Sup[t] = iPt.value() + error;
                curveG95Inf[t] = iPt.value() - error;

                yMin = qMin(yMin, curveG95Inf.value(t));
                yMax = qMax(yMax, curveG95Sup.value(t));
            }
        }
        mGraph->setRangeX(tminDisplay,tmaxDisplay);
        mGraph->setCurrentX(tminDisplay, tmaxDisplay);


        GraphCurve graphCurveG;
        graphCurveG.mName = "G";
        graphCurveG.mData = curveG;
        graphCurveG.mPen.setColor(Painting::mainColorDark);
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
        //mGraph->addInfo(tr("Ref : ") + ref_curve);


        // ----------------------------------------------
        //  Measure curve
        // ----------------------------------------------
        double error = 0.;
        double avg = 0.;
        if (is_inc)  {
            avg = inc;
            error = alpha / 2.448f;
        }
        else if(is_dec)
        {
            avg = dec;
            error = alpha / (2.448 * cosf(inc * M_PI / 180.));
        }
        else if (is_int)
        {
            avg = intensity;
            error = alpha;
        }
        
        yMin = qMin(yMin, avg - error);
        yMax = qMax(yMax, avg + error);
        yMin = yMin - 0.05 * (yMax - yMin);
        yMax = yMax + 0.05 * (yMax - yMin);
        
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
        const double yStep = (yMax - yMin) / 5000.;
        QMap<double,double> measureCurve;
        for (double t=yMin; t<yMax; t+=yStep) {
            double v = 0.;
            if (is_inc) {
                v = exp(-0.5 * pow((t - inc) / error, 2.));
            }
            else if(is_dec)
            {
                v = exp(-0.5 * pow((t - dec) / error, 2));
            }
            else if(is_int)
            {
                v = exp(-0.5 * pow((t - intensity) / error, 2));
            }
            //curveMeasure.mData[t] = v;
            measureCurve[t] = v;
        }
        measureCurve = normalize_map(measureCurve);
        curveMeasure.mData = measureCurve;
        //curveMeasure.mData = normalize_map(curveMeasure.mData);
        mGraph->addCurve(curveMeasure);
        
        // Write measure values :
    /*    if(is_inc)
        {
            mGraph->addInfo(tr("Inclination : ") + locale.toString(inc) + ", α : " + locale.toString(alpha));
        }
        else if(is_dec)
        {
            mGraph->addInfo(tr("Declination : ") + locale.toString(dec) + ", α : " + locale.toString(alpha) + tr(", Inclination : ") + locale.toString(inc));
        }
        else if(is_int)
        {
            mGraph->addInfo(tr("Intensity : ") + locale.toString(intensity) + " ± : " + locale.toString(alpha));
        }
     */
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

        curveMeasureAvg.mHorizontalValue = avg;
        curveMeasureSup.mHorizontalValue = avg + error;
        curveMeasureInf.mHorizontalValue = avg - error;
    
        qDebug()<<"PluginMagRefView::setDate"<<floor(yMin)<<floor(yMax);
        mGraph->setRangeY(floor(yMin), floor(yMax));
        mGraph->addCurve(curveMeasureAvg);
        mGraph->addCurve(curveMeasureSup);
        mGraph->addCurve(curveMeasureInf);
        mGraph->setFormatFunctY(0);
    }
}

void PluginMagRefView::zoomX(const double min, const double max)
{
    mGraph->zoomX(min, max);
}
void PluginMagRefView::setMarginRight(const int margin)
{
    mGraph->setMarginRight(margin);
}
void PluginMagRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(rect());
}

#endif
