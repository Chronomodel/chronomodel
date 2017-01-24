#include "PluginAMRefView.h"
#if USE_PLUGIN_AM

#include "PluginAM.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>


PluginAMRefView::PluginAMRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mGraphI = new GraphView(this);
    mGraphI->setXAxisMode(GraphView::eAllTicks);
    mGraphI->setYAxisMode(GraphView::eAllTicks);
    mGraphI->setRendering(GraphView::eHD);
    mGraphI->autoAdjustYScale(true);
    mGraphI->setTipXLab("t");
    mGraphI->setTipYLab("value");
    
    mGraphD = new GraphView(this);
    mGraphD->setXAxisMode(GraphView::eAllTicks);
    mGraphD->setYAxisMode(GraphView::eAllTicks);
    mGraphD->setRendering(GraphView::eHD);
    mGraphD->autoAdjustYScale(true);
    mGraphD->setTipXLab("t");
    mGraphD->setTipYLab("value");
    
    mGraphF = new GraphView(this);
    mGraphF->setXAxisMode(GraphView::eAllTicks);
    mGraphF->setYAxisMode(GraphView::eAllTicks);
    mGraphF->setRendering(GraphView::eHD);
    mGraphF->autoAdjustYScale(true);
    mGraphF->setTipXLab("t");
    mGraphF->setTipYLab("value");
    
    mMeasureColor = QColor(56, 120, 50);
}

PluginAMRefView::~PluginAMRefView()
{
    
}

void PluginAMRefView::setDate(const Date& date, const ProjectSettings& settings)
{
    GraphViewRefAbstract::setDate(date, settings);
    
    double tminDisplay;
    double tmaxDisplay;
    {
        const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
        const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);
        const double t3 = date.getFormatedTminCalib();
        const double t4 = date.getFormatedTmaxCalib();

        tminDisplay = qMin(t1, qMin(t2,t3));
        tmaxDisplay = qMax(t1, qMax(t2,t4));
    }

    mGraphI->setRangeX(tminDisplay, tmaxDisplay);
    mGraphI->setCurrentX(tminDisplay, tmaxDisplay);
    mGraphI->removeAllCurves();
    mGraphI->clearInfos();
    mGraphI->showInfos(true);
    mGraphI->setFormatFunctX(0);
    
    mGraphD->setRangeX(tminDisplay, tmaxDisplay);
    mGraphD->setCurrentX(tminDisplay, tmaxDisplay);
    mGraphD->removeAllCurves();
    mGraphD->clearInfos();
    mGraphD->showInfos(true);
    mGraphD->setFormatFunctX(0);
    
    mGraphF->setRangeX(tminDisplay, tmaxDisplay);
    mGraphF->setCurrentX(tminDisplay, tmaxDisplay);
    mGraphF->removeAllCurves();
    mGraphF->clearInfos();
    mGraphF->showInfos(true);
    mGraphF->setFormatFunctX(0);
    
    if (!date.isNull())
    {
        QString mode = date.mData.value(DATE_AM_MODE).toString();
        QString curveNameI = date.mData.value(DATE_AM_CURVE_I).toString().toLower();
        QString curveNameD = date.mData.value(DATE_AM_CURVE_D).toString().toLower();
        QString curveNameF = date.mData.value(DATE_AM_CURVE_F).toString().toLower();
        
        bool needsI = (mode == DATE_AM_MODE_I || mode == DATE_AM_MODE_ID || mode == DATE_AM_MODE_IF || mode == DATE_AM_MODE_IDF);
        bool needsD = (mode == DATE_AM_MODE_D || mode == DATE_AM_MODE_ID || mode == DATE_AM_MODE_IDF);
        bool needsF = (mode == DATE_AM_MODE_F || mode == DATE_AM_MODE_IF || mode == DATE_AM_MODE_IDF);
        
        if(needsI)
        {
            setRefCurve(date, curveNameI, "I", tminDisplay, tmaxDisplay);
        }
        if(needsD)
        {
            setRefCurve(date, curveNameD, "D", tminDisplay, tmaxDisplay);
        }
        if(needsF)
        {
            setRefCurve(date, curveNameF, "F", tminDisplay, tmaxDisplay);
        }
        
        mMode = mode;
        mGraphI->setVisible(needsI);
        mGraphD->setVisible(needsD);
        mGraphF->setVisible(needsF);
    }
    update();
}

void PluginAMRefView::setRefCurve(const Date& date,
                                  const QString& curveName,
                                  const QString& type,
                                  const double& tminDisplay,
                                  const double& tmaxDisplay)
{
    GraphView* graphView = 0;
    if(type == "I") graphView = mGraphI;
    else if(type == "D") graphView = mGraphD;
    else if(type == "F") graphView = mGraphF;
    
    QLocale locale = QLocale();
    double tminRef = date.getFormatedTminRefCurve();
    double tmaxRef = date.getFormatedTmaxRefCurve();
    QColor color2(150, 150, 150);
    PluginAM* plugin = (PluginAM*)date.mPlugin;
    const RefCurve& curve = plugin->mRefCurves.value(curveName);
    
    double i = date.mData.value(DATE_AM_I).toDouble();
    double d = date.mData.value(DATE_AM_D).toDouble();
    double f = date.mData.value(DATE_AM_F).toDouble();
    double alpha95 = date.mData.value(DATE_AM_ALPHA_95).toDouble();
    double sigmaF = date.mData.value(DATE_AM_SIGMA_F).toDouble();
    
    if (curve.mDataMean.isEmpty()) {
        GraphZone zone;
        zone.mColor = Qt::gray;
        zone.mColor.setAlpha(25);
        zone.mXStart = tminDisplay;
        zone.mXEnd = tmaxDisplay;
        zone.mText = tr("No reference data");
        graphView->addZone(zone);
        return;
    }
    
    if(tminDisplay < tminRef){
        GraphZone zone;
        zone.mColor = QColor(217, 163, 69);
        zone.mColor.setAlpha(35);
        zone.mXStart = tminDisplay;
        zone.mXEnd = tminRef;
        zone.mText = tr("Outside reference area");
        graphView->addZone(zone);
    }
    
    if (tmaxRef < tmaxDisplay) {
        GraphZone zone;
        zone.mColor = QColor(217, 163, 69);
        zone.mColor.setAlpha(35);
        zone.mXStart = tmaxRef;
        zone.mXEnd = tmaxDisplay;
        zone.mText = tr("Outside reference area");
        graphView->addZone(zone);
    }
    
    const double t0 = DateUtils::convertFromAppSettingsFormat(qMax(tminDisplay, tminRef));
    double yMin = plugin->getRefCurveValueAt(curveName, t0);
    double yMax = yMin;
    
    QMap<double, double> curveG;
    QMap<double, double> curveG95Sup;
    QMap<double, double> curveG95Inf;
    
    for(double t = tminDisplay; t <= tmaxDisplay; ++t)
    {
        if(t > tminRef && t < tmaxRef)
        {
            const double tRaw = DateUtils::convertFromAppSettingsFormat(t);
            const double value = plugin->getRefCurveValueAt(curveName, tRaw);
            const double error = plugin->getRefCurveValueAt(curveName, tRaw) * 1.96;
            
            curveG[t] = value;
            curveG95Sup[t] = value + error;
            curveG95Inf[t] = value - error;
            
            yMin = qMin(yMin, curveG95Inf[t]);
            yMax = qMax(yMax, curveG95Sup[t]);
        }
    }
    
    graphView->setRangeX(tminDisplay,tmaxDisplay);
    graphView->setCurrentX(tminDisplay, tmaxDisplay);
    
    GraphCurve graphCurveG;
    graphCurveG.mName = "G";
    graphCurveG.mData = curveG;
    graphCurveG.mPen.setColor(Painting::mainColorDark);
    graphCurveG.mIsHisto = false;
    graphView->addCurve(graphCurveG);
    
    GraphCurve graphCurveG95Sup;
    graphCurveG95Sup.mName = "G95Sup";
    graphCurveG95Sup.mData = curveG95Sup;
    graphCurveG95Sup.mPen.setColor(QColor(180, 180, 180));
    graphCurveG95Sup.mIsHisto = false;
    graphView->addCurve(graphCurveG95Sup);
    
    GraphCurve graphCurveG95Inf;
    graphCurveG95Inf.mName = "G95Inf";
    graphCurveG95Inf.mData = curveG95Inf;
    graphCurveG95Inf.mPen.setColor(QColor(180, 180, 180));
    graphCurveG95Inf.mIsHisto = false;
    graphView->addCurve(graphCurveG95Inf);
    
    // Display reference curve name
    graphView->addInfo(tr("Ref : ") + curveName);
    
    
    // ----------------------------------------------
    //  Measure curve
    // ----------------------------------------------
    double error = 0.;
    double avg = 0.;
    if(type == "I")
    {
        avg = i;
        error = alpha95 / 2.448f;
    }
    else if(type == "D")
    {
        avg = d;
        error = alpha95 / (2.448f * cosf(i * M_PI / 180.f));
    }
    else if(type == "F")
    {
        avg = f;
        error = alpha95;
    }
    
    yMin = qMin(yMin, avg - error);
    yMax = qMax(yMax, avg + error);
    yMin = yMin - 0.05 * (yMax - yMin);
    yMax = yMax + 0.05 * (yMax - yMin);
    
    graphView->setRangeY(yMin, yMax);
    
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
    for(double t=yMin; t<yMax; t+=yStep)
    {
        double v = exp(-0.5 * pow((t - avg) / error, 2.));
        measureCurve[t] = v;
    }
    measureCurve = normalize_map(measureCurve);
    curveMeasure.mData = measureCurve;
    //curveMeasure.mData = normalize_map(curveMeasure.mData);
    graphView->addCurve(curveMeasure);
    
    // Write measure values :
    if(type == "I")
    {
        graphView->addInfo(tr("Inclination : ") + locale.toString(i) + ", α : " + locale.toString(alpha95));
    }
    else if(type == "D")
    {
        graphView->addInfo(tr("Declination : ") + locale.toString(d) + ", α : " + locale.toString(alpha95));
    }
    else if(type == "F")
    {
        graphView->addInfo(tr("Intensity : ") + locale.toString(f) + " ± : " + locale.toString(sigmaF));
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
    
    graphView->setRangeY(floor(yMin), floor(yMax));
    graphView->addCurve(curveMeasureAvg);
    graphView->addCurve(curveMeasureSup);
    graphView->addCurve(curveMeasureInf);
    graphView->setFormatFunctY(0);
}

void PluginAMRefView::zoomX(const double min, const double max)
{
    mGraphI->zoomX(min, max);
    mGraphD->zoomX(min, max);
    mGraphF->zoomX(min, max);
}
void PluginAMRefView::setMarginRight(const int margin)
{
    mGraphI->setMarginRight(margin);
    mGraphD->setMarginRight(margin);
    mGraphF->setMarginRight(margin);
}
void PluginAMRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int w = rect().width();
    
    if(mMode == DATE_AM_MODE_I)
    {
        int h = rect().height();
        mGraphI->setGeometry(QRect(0, 0, w, h));
    }
    else if(mMode == DATE_AM_MODE_D)
    {
        int h = rect().height();
        mGraphD->setGeometry(QRect(0, 0, w, h));
    }
    else if(mMode == DATE_AM_MODE_F)
    {
        int h = rect().height();
        mGraphF->setGeometry(QRect(0, 0, w, h));
    }
    else if(mMode == DATE_AM_MODE_ID)
    {
        int h = rect().height() / 2;
        mGraphI->setGeometry(QRect(0, 0, w, h));
        mGraphD->setGeometry(QRect(0, h, w, h));
    }
    else if(mMode == DATE_AM_MODE_IF)
    {
        int h = rect().height() / 2;
        mGraphI->setGeometry(QRect(0, 0, w, h));
        mGraphF->setGeometry(QRect(0, h, w, h));
    }
    else if(mMode == DATE_AM_MODE_IDF)
    {
        int h = rect().height() / 3;
        mGraphI->setGeometry(QRect(0, 0, w, h));
        mGraphD->setGeometry(QRect(0, h, w, h));
        mGraphF->setGeometry(QRect(0, 2*h, w, h));
    }
}

#endif
