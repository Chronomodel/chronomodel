#include "PluginMagRefView.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include <QtWidgets>


PluginMagRefView::PluginMagRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mGraph = new GraphView(this);
    mGraph->showYValues(true);
    mGraph->showAxis(false);
}

PluginMagRefView::~PluginMagRefView()
{
    
}

void PluginMagRefView::setDate(const Date& d, const ProjectSettings& settings)
{
    GraphViewRefAbstract::setDate(d, settings);
    Date date = d;
    
    mGraph->removeAllCurves();
    mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    
    if(!date.isNull())
    {
        bool is_inc = date.mData.value(DATE_AM_IS_INC_STR).toBool();
        bool is_dec = date.mData.value(DATE_AM_IS_DEC_STR).toBool();
        bool is_int = date.mData.value(DATE_AM_IS_INT_STR).toBool();
        
        float inc = date.mData.value(DATE_AM_INC_STR).toDouble();
        float dec = date.mData.value(DATE_AM_DEC_STR).toDouble();
        float intensity = date.mData.value(DATE_AM_INTENSITY_STR).toDouble();
        float error = date.mData.value(DATE_AM_ERROR_STR).toDouble();
        QString ref_curve = date.mData.value(DATE_AM_REF_CURVE_STR).toString();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------
        
        QColor color2(150, 150, 150);
        
        PluginMag* plugin = (PluginMag*)date.mPlugin;
        const QMap<QString, QMap<float, float>>& curves = plugin->getRefData(ref_curve);
        
        QMap<float, float> curveG;
        QMap<float, float> curveG95Sup;
        QMap<float, float> curveG95Inf;
        
        //qDebug() << curves["G"][0];
        
        for(int t=mSettings.mTmin; t<=mSettings.mTmax; t+=mSettings.mStep)
        {
            curveG[t] = curves["G"][t];
            curveG95Sup[t] = curves["G95Sup"][t];
            curveG95Inf[t] = curves["G95Inf"][t];
            
            //qDebug() << t << ", " << curves["G"][t];
        }
        
        GraphCurve graphCurveG;
        graphCurveG.mName = "G";
        graphCurveG.mData = curveG;
        graphCurveG.mPen.setColor(Qt::blue);
        mGraph->addCurve(graphCurveG);
        
        GraphCurve graphCurveG95Sup;
        graphCurveG95Sup.mName = "G95Sup";
        graphCurveG95Sup.mData = curveG95Sup;
        graphCurveG95Sup.mPen.setColor(QColor(180, 180, 180));
        mGraph->addCurve(graphCurveG95Sup);
        
        GraphCurve graphCurveG95Inf;
        graphCurveG95Inf.mName = "G95Inf";
        graphCurveG95Inf.mData = curveG95Inf;
        graphCurveG95Inf.mPen.setColor(QColor(180, 180, 180));
        mGraph->addCurve(graphCurveG95Inf);
        
        // ----------------------------------------------
        
        float yMin = map_min_value(curveG95Inf);
        float yMax = map_max_value(curveG95Sup);
        
        if(is_inc)
        {
            yMin = qMin(yMin, inc);
            yMax = qMax(yMax, inc);
        }
        else if(is_dec)
        {
            yMin = qMin(yMin, dec);
            yMax = qMax(yMax, dec);
        }
        else if(is_int)
        {
            yMin = qMin(yMin, intensity);
            yMax = qMax(yMax, intensity);
        }
        
        mGraph->setRangeY(yMin, yMax);
        
        // ----------------------------------------------
        //  Measure curve
        // ----------------------------------------------
        
        GraphCurve curveMeasure;
        curveMeasure.mName = "Measure";
        curveMeasure.mPen.setColor(mMeasureColor);
        curveMeasure.mFillUnder = true;
        curveMeasure.mIsVertical = true;
        
        float yStep = (yMax - yMin) / 500.f;
        
        for(float t=yMin; t<yMax; t+=yStep)
        {
            float v = 0.f;
            
            if(is_inc)
                v = exp(-0.5 * pow((t - inc) / error, 2));
            else if(is_dec)
                v = exp(-0.5 * pow((t - dec) / error, 2));
            else if(is_int)
                v = exp(-0.5 * pow((t - intensity) / error, 2));
            
            curveMeasure.mData[t] = v;
        }
        curveMeasure.mData = normalize_map(curveMeasure.mData);
        mGraph->addCurve(curveMeasure);
    }
}

void PluginMagRefView::zoomX(float min, float max)
{
    mGraph->zoomX(min, max);
}

void PluginMagRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(rect());
}

#endif
