#include "Plugin14CRefView.h"
#if USE_PLUGIN_14C

#include "Plugin14C.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include <QtWidgets>


Plugin14CRefView::Plugin14CRefView(QWidget* parent):GraphViewRefAbstract(parent),
mGraph(0)
{
    mGraph = new GraphView(this);
    mGraph->showYValues(true);
    mGraph->showAxis(false);
}

Plugin14CRefView::~Plugin14CRefView()
{
    
}

void Plugin14CRefView::setDate(const Date& d, const ProjectSettings& settings)
{
    GraphViewRefAbstract::setDate(d, settings);
    Date date = d;
    
    mGraph->removeAllCurves();
    mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    
    if(!date.isNull())
    {
        float age = date.mData.value(DATE_14C_AGE_STR).toDouble();
        float error = date.mData.value(DATE_14C_ERROR_STR).toDouble();
        QString ref_curve = date.mData.value(DATE_14C_REF_CURVE_STR).toString().toLower();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------
        
        QColor color2(150, 150, 150);
        
        Plugin14C* plugin = (Plugin14C*)date.mPlugin;
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
            float v = expf(-0.5 * powf((age - t) / error, 2));
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

void Plugin14CRefView::zoomX(float min, float max)
{
    mGraph->zoomX(min, max);
}

void Plugin14CRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(rect());
}

#endif
