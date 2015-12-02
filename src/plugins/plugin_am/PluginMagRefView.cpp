#include "PluginMagRefView.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>


PluginMagRefView::PluginMagRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mGraph = new GraphView(this);
    
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->setRendering(GraphView::eHD);
    mGraph->autoAdjustYScale(true);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("value");
    mMeasureColor = QColor(56, 120, 50);
}

PluginMagRefView::~PluginMagRefView()
{
    
}

void PluginMagRefView::setDate(const Date& d, const ProjectSettings& settings)
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
        
        QColor color2(150, 150, 150);
        
        PluginMag* plugin = (PluginMag*)date.mPlugin;
        const QMap<QString, QMap<double, double> >& curves = plugin->getRefData(ref_curve);
        
        if(curves.isEmpty()) {
            qDebug()<<"in PluginMagRefView invalid ref curve"<<ref_curve;
            return;
        }

        QMap<double, double> curveG;
        QMap<double, double> curveG95Sup;
        QMap<double, double> curveG95Inf;
        
        double tMinGraph = qMax(curves["G"].firstKey(), (double)mSettings.mTmin);
        double tMaxGraph = qMin(curves["G"].lastKey(), (double)mSettings.mTmax);
        
        double yMin = plugin->getRefValueAt(date.mData, mSettings.mTmin);
        double yMax = plugin->getRefValueAt(date.mData, mSettings.mTmin);
        
        if(tMinGraph < tMaxGraph)
        {
            for(double t=tMinGraph; t<=tMaxGraph; ++t)
            {
                double value = plugin->getRefValueAt(date.mData, t);
                double error = plugin->getRefErrorAt(date.mData, t) * 1.96;
                
                curveG[t] = value;
                curveG95Sup[t] = value + error;
                curveG95Inf[t] = value - error;
                
                yMin = qMin(yMin, curveG95Inf[t]);
                yMax = qMax(yMax, curveG95Sup[t]);
            }
        }
        
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
        mGraph->addInfo(tr("Ref : ") + ref_curve);


        // ----------------------------------------------------
        //  Draw ref curve extensions if not defined everywhere on study period
        // ----------------------------------------------------
        GraphZone zone;
        zone.mColor = Qt::red;
        zone.mColor.setAlpha(20);

        zone.mXStart = mSettings.mTmin;
        zone.mXEnd = tMinGraph;
        mGraph->addZone(zone);

        zone.mXStart = tMaxGraph;
        zone.mXEnd = mSettings.mTmax;
        mGraph->addZone(zone);

        // ----------------------------------------------------
        
        double error = 0.f;
        double avg = 0.f;
        if(is_inc)
        {
            avg = inc;
            error = alpha / 2.448f;
        }
        else if(is_dec)
        {
            avg = dec;
            error = alpha / (2.448f * cosf(inc * M_PI / 180.f));
        }
        else if(is_int)
        {
            avg = intensity;
            error = alpha;
        }
        
        yMin = qMin(yMin, avg - error);
        yMax = qMax(yMax, avg + error);
        yMin = yMin - 0.05f * (yMax - yMin);
        yMax = yMax + 0.05f * (yMax - yMin);
        
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
        double yStep = (yMax - yMin) / 5000.;
        for(double t=yMin; t<yMax; t+=yStep)
        {
            double v = 0.f;
            if(is_inc)
            {
                v = exp(-0.5f * pow((t - inc) / error, 2.f));
            }
            else if(is_dec)
            {
                v = exp(-0.5f * pow((t - dec) / error, 2));
            }
            else if(is_int)
            {
                v = exp(-0.5f * pow((t - intensity) / error, 2));
            }
            curveMeasure.mData[t] = v;
        }
        curveMeasure.mData = normalize_map(curveMeasure.mData);
        mGraph->addCurve(curveMeasure);
        
        // Write measure values :
        if(is_inc)
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

void PluginMagRefView::zoomX(double min, double max)
{
    mGraph->zoomX(min, max);
}

void PluginMagRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(rect());
}

#endif
