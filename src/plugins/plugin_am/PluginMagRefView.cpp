/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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
    mGraph->autoAdjustYScale(true);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("value");

}

PluginMagRefView::~PluginMagRefView()
{

}

void PluginMagRefView::setDate(const Date& date, const ProjectSettings& settings)
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
    mGraph->setFormatFunctX(nullptr);

    if (!date.isNull()) {
        bool is_inc = date.mData.value(DATE_AM_IS_INC_STR).toBool();
        bool is_dec = date.mData.value(DATE_AM_IS_DEC_STR).toBool();
        bool is_int = date.mData.value(DATE_AM_IS_INT_STR).toBool();

        double inc = date.mData.value(DATE_AM_INC_STR).toDouble();
        double dec = date.mData.value(DATE_AM_DEC_STR).toDouble();
        double intensity = date.mData.value(DATE_AM_INTENSITY_STR).toDouble();
        double alpha = date.mData.value(DATE_AM_ERROR_STR).toDouble();
        QString ref_curve = date.mData.value(DATE_AM_REF_CURVE_STR).toString().toLower();

        /* ----------------------------------------------
         *  Reference curve
         * ---------------------------------------------- */

        double tminRef = date.getFormatedTminRefCurve();
        double tmaxRef = date.getFormatedTmaxRefCurve();

        PluginMag* plugin = static_cast<PluginMag*>(date.mPlugin);
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

        if (tminDisplay < tminRef){
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
        double yMin = plugin->getRefValueAt(date.mData, t0);
        double yMax = yMin;

        QMap<double, double> curveG;
        QMap<double, double> curveG95Sup;
        QMap<double, double> curveG95Inf;


        for ( QMap<double, double>::const_iterator &&iPt = curve.mDataMean.cbegin();  iPt!=curve.mDataMean.cend(); ++iPt) {
            const double t (iPt.key());
            const double tDisplay = DateUtils::convertToAppSettingsFormat(t);
            if (tDisplay>tminDisplay && tDisplay<tmaxDisplay) {
                const double error = plugin->getRefErrorAt(date.mData, t) * 1.96;

                curveG[tDisplay] = iPt.value();
                curveG95Sup[tDisplay] = iPt.value() + error;
                curveG95Inf[tDisplay] = iPt.value() - error;

               yMin = qMin(yMin, curveG95Inf.value(tDisplay));
               yMax = qMax(yMax, curveG95Sup.value(tDisplay));
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


        /* ----------------------------------------------
         *  Measure curve
         * ---------------------------------------------- */
        double error (0.);
        double avg (0.);
        if (is_inc)  {
            avg = inc;
            error = alpha / 2.448;

        } else if (is_dec) {
            avg = dec;
            error = alpha / (2.448 * cos(inc * M_PI / 180.));

        } else if (is_int) {
            avg = intensity;
            error = alpha;
        }

        yMin = qMin(yMin, avg - error);
        yMax = qMax(yMax, avg + error);
        yMin = yMin - 0.05 * (yMax - yMin);
        yMax = yMax + 0.05 * (yMax - yMin);

        /* ----------------------------------------------
         *  Measure curve
         * ---------------------------------------------- */

        GraphCurve curveMeasure;
        curveMeasure.mName = "Measurement";

        curveMeasure.mPen.setColor(mMeasureColor);
        QColor curveColor(mMeasureColor);
        curveColor.setAlpha(50);
        curveMeasure.mBrush = curveColor;

        curveMeasure.mIsVertical = true;
        curveMeasure.mIsHisto = false;

        /* 5000 pts are used on vertical measurement
         * because the y scale auto adjusts depending on x zoom.
         * => the visible part of the measurement may be very reduced ! */
        const double yStep = (yMax - yMin) / 5000.;
        QMap<double,double> measureCurve;
        for (double t=yMin; t<yMax; t+=yStep) {
            double v (0.);
            if (is_inc)
                v = exp(-0.5 * pow((t - inc) / error, 2.));

            else if (is_dec)
                v = exp(-0.5 * pow((t - dec) / error, 2.));

            else if(is_int)
                v = exp(-0.5 * pow((t - intensity) / error, 2.));

            measureCurve[t] = v;
        }
        measureCurve = normalize_map(measureCurve);
        curveMeasure.mData = measureCurve;
        //curveMeasure.mData = normalize_map(curveMeasure.mData);
        mGraph->addCurve(curveMeasure);


        // ----------------------------------------------
        //  Error on measurement
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

        mGraph->addCurve(curveMeasureAvg);
        mGraph->addCurve(curveMeasureSup);
        mGraph->addCurve(curveMeasureInf);
        mGraph->setFormatFunctY(nullptr);

        // Y scale and RangeY are define in graphView::zommX()

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
