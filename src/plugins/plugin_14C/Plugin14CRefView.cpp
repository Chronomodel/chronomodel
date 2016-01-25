#include "Plugin14CRefView.h"
#if USE_PLUGIN_14C

#include "Plugin14C.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>



Plugin14CRefView::Plugin14CRefView(QWidget* parent):GraphViewRefAbstract(parent),
mGraph(0)
{
    mGraph = new GraphView(this);
    
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->setRendering(GraphView::eHD);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("age");
    mGraph->autoAdjustYScale(true);
}

Plugin14CRefView::~Plugin14CRefView()
{
    
}

void Plugin14CRefView::setDate(const Date& date, const ProjectSettings& settings)
{
    QLocale locale = QLocale();
    GraphViewRefAbstract::setDate(date, settings);
    
    mGraph->setRangeX(mTminDisplay, mTmaxDisplay);
    mGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
    
    mGraph->removeAllCurves();
    mGraph->clearInfos();
    mGraph->showInfos(true);
    mGraph->setFormatFunctX(mFormatFuncX);
    
    if(!date.isNull())
    {
        double age = date.mData.value(DATE_14C_AGE_STR).toDouble();
        double error = date.mData.value(DATE_14C_ERROR_STR).toDouble();
        double delta_r = date.mData.value(DATE_14C_DELTA_R_STR).toDouble();
        double delta_r_error = date.mData.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();
        QString ref_curve = date.mData.value(DATE_14C_REF_CURVE_STR).toString().toLower();
        
        // ----------------------------------------------
        //  Reference curve
        // ----------------------------------------------
        
        QColor color2(150, 150, 150);
        
        Plugin14C* plugin = (Plugin14C*)date.mPlugin;

        const RefCurve& curve = plugin->mRefCurves.value(ref_curve);
        
        if(curve.mDataMean.isEmpty())
        {
            GraphZone zone;
            zone.mColor = Qt::gray;
            zone.mColor.setAlpha(25);
            zone.mXStart = mTminDisplay;
            zone.mXEnd = mTmaxDisplay;
            zone.mText = tr("No reference data");
            mGraph->addZone(zone);
            return;
        }

        if(mTminDisplay < mTminRef){
            GraphZone zone;
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mXStart = mTminDisplay;
            zone.mXEnd = mTminRef;
            zone.mText = tr("Outside reference area");
            mGraph->addZone(zone);
        }

        if(mTmaxRef < mTmaxDisplay){
            GraphZone zone;
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mXStart = mTmaxRef;
            zone.mXEnd = mTmaxDisplay;
            zone.mText = tr("Outside reference area");
            mGraph->addZone(zone);
        }

        double yMin = plugin->getRefValueAt(date.mData, qMax(mTminDisplay, mTminRef));
        double yMax = yMin;

        QMap<double, double> curveG;
        QMap<double, double> curveG95Sup;
        QMap<double, double> curveG95Inf;

        for(double t=mTminDisplay; t<=mTmaxDisplay; ++t)
        {
            if(t>mTminRef && t<mTmaxRef)
            {
                double value = plugin->getRefValueAt(date.mData, t);
                double error = plugin->getRefErrorAt(date.mData, t) * 1.96;
                
                curveG[t] = value;
                curveG95Sup[t] = value + error;
                curveG95Inf[t] = value - error;

                yMin = qMin(yMin, curveG95Inf.value(t));
                yMax = qMax(yMax, curveG95Sup.value(t));
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
                
        // ----------------------------------------------
        //  Measure curve
        // ----------------------------------------------
        yMin = qMin(yMin, age - error * 1.96);
        yMax = qMax(yMax, age + error * 1.96);
        
        GraphCurve curveMeasure;
        curveMeasure.mName = "Measure";
        
        QColor penColor(mMeasureColor);
        QColor brushColor(mMeasureColor);
        
        // Lower opacity in case of delta r not null
        if(delta_r != 0 && delta_r_error != 0){
            penColor.setAlpha(100);
            brushColor.setAlpha(15);
        }else{
            penColor.setAlpha(255);
            brushColor.setAlpha(50);
        }
        curveMeasure.mPen = penColor;
        curveMeasure.mBrush = brushColor;
        curveMeasure.mIsVertical = true;
        curveMeasure.mIsHisto = false;
        
        // 5000 pts are used on vertical measure
        // because the y scale auto adjusts depending on x zoom.
        // => the visible part of the measure may be very reduced !
        double step = (yMax - yMin) / 5000.;
        for(double t=yMin; t<yMax; t += step)
        {
            double v = exp(-0.5 * pow((age - t) / error, 2));
            curveMeasure.mData[t] = v;
        }
        curveMeasure.mData = normalize_map(curveMeasure.mData);
        mGraph->addCurve(curveMeasure);
        
        // Infos to write :
        QString info = tr("Age BP : ") + locale.toString(age) + " ± " + locale.toString(error);
        
        // ----------------------------------------------
        //  Delta R curve
        // ----------------------------------------------
        if(delta_r != 0 && delta_r_error != 0)
        {
            // Apply reservoir effect
            age = (age - delta_r);
            error = sqrt(error * error + delta_r_error * delta_r_error);
            
            yMin = qMin(yMin, age - error * 1.96);
            yMax = qMax(yMax, age + error * 1.96);
            
            GraphCurve curveDeltaR;
            curveDeltaR.mName = "Delta R";
            
            penColor = mMeasureColor;
            brushColor = mMeasureColor;
            brushColor.setAlpha(50);
            
            curveDeltaR.mPen = penColor;
            curveDeltaR.mBrush = brushColor;
            
            curveDeltaR.mIsVertical = true;
            curveDeltaR.mIsHisto = false;
            
            // 5000 pts are used on vertical measure
            // because the y scale auto adjusts depending on x zoom.
            // => the visible part of the measure may be very reduced !
            step = (yMax - yMin) / 5000.;
            for(double t=yMin; t<yMax; t += step)
            {
                double v = exp(-0.5 * pow((age - t) / error, 2));
                curveDeltaR.mData[t] = v;
            }
            curveDeltaR.mData = normalize_map(curveDeltaR.mData);
            mGraph->addCurve(curveDeltaR);
            
            info += tr(", ΔR : ") + locale.toString(delta_r) + " ± " + locale.toString(delta_r_error);
        }
        
        // ----------------------------------------------
        //  Sub-dates curves (combination)
        // ----------------------------------------------
        for(int i=0; i<date.mSubDates.size(); ++i){
            const Date& d = date.mSubDates.at(i);
            
            GraphCurve curveSubMeasure;
            curveSubMeasure.mName = "Sub-Measure " + QString::number(i);
            
            double sub_age = d.mData.value(DATE_14C_AGE_STR).toDouble();
            double sub_error = d.mData.value(DATE_14C_ERROR_STR).toDouble();
            double sub_delta_r = d.mData.value(DATE_14C_DELTA_R_STR).toDouble();
            double sub_delta_r_error = d.mData.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();
            
            // Apply reservoir effect
            sub_age = (sub_age - sub_delta_r);
            sub_error = sqrt(sub_error * sub_error + sub_delta_r_error * sub_delta_r_error);
            
            yMin = qMin(yMin, sub_age - sub_error * 1.96);
            yMax = qMax(yMax, sub_age + sub_error * 1.96);
            
            QColor penColor = QColor(167, 126, 73);
            QColor brushColor = QColor(167, 126, 73);
            penColor.setAlpha(255);
            brushColor.setAlpha(50);
            
            curveSubMeasure.mPen = penColor;
            curveSubMeasure.mBrush = brushColor;
            curveSubMeasure.mIsVertical = true;
            curveSubMeasure.mIsHisto = false;
            
            // 5000 pts are used on vertical measure
            // because the y scale auto adjusts depending on x zoom.
            // => the visible part of the measure may be very reduced !
            double step = (yMax - yMin) / 5000.;
            for(double t=yMin; t<yMax; t += step)
            {
                double v = exp(-0.5 * pow((sub_age - t) / sub_error, 2));
                curveSubMeasure.mData[t] = v;
            }
            curveSubMeasure.mData = normalize_map(curveSubMeasure.mData);
            mGraph->addCurve(curveSubMeasure);
        }

        // ----------------------------------------------
        //  Textual info
        // ----------------------------------------------
        
        mGraph->addInfo(info);
        
        // ----------------------------------------------
        //  Error on measure (horizontal lines)
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
        
        // ----------------------------------------------
        mGraph->setRangeY(yMin, yMax);
    }
}

void Plugin14CRefView::zoomX(double min, double max)
{
    mGraph->zoomX(min, max);
}

void Plugin14CRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(rect());
}

#endif
