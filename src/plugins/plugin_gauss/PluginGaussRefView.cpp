/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include "PluginGaussRefView.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"

#include <QtWidgets>

PluginGaussRefView::PluginGaussRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(98, 113, 123, 240);
    mGraph = new GraphView(this);

    mGraph->setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
    mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->autoAdjustYScale(true);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("x");

}

PluginGaussRefView::~PluginGaussRefView()
{

}

void PluginGaussRefView::setDate(const Date& date, const StudyPeriodSettings& settings)
{
    GraphViewRefAbstract::setDate(date, settings);
    
    if (date.mOrigin == Date::eSingleDate) {

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
        mGraph->remove_all_zones();
        mGraph->clearInfos();
        mGraph->showInfos(true);
        mGraph->setFormatFunctX(nullptr);

        if (!date.isNull()) {
            const double age = date.mData.value(DATE_GAUSS_AGE_STR).toDouble();
            const double error = date.mData.value(DATE_GAUSS_ERROR_STR).toDouble();
            const double a = date.mData.value(DATE_GAUSS_A_STR).toDouble();
            const double b = date.mData.value(DATE_GAUSS_B_STR).toDouble();
            const double c = date.mData.value(DATE_GAUSS_C_STR).toDouble();
            const QString mode = date.mData.value(DATE_GAUSS_MODE_STR).toString();
            const QString ref_curve = date.mData.value(DATE_GAUSS_CURVE_STR).toString();

            /* ----------------------------------------------
             *  Reference curve
             * ---------------------------------------------- */

            const double tminRef = date.getFormatedTminRefCurve();
            const double tmaxRef = date.getFormatedTmaxRefCurve();

            GraphCurve curve;
            curve.mVisible = true;
            curve.mName = "Reference";
            curve.mPen.setColor(Painting::mainColorDark);


            if (mode == DATE_GAUSS_MODE_NONE) {
              // nothing to do

            } else if (mode == DATE_GAUSS_MODE_EQ) {
                QMap<double,double> refCurve;
                double stepDisplay = tmaxDisplay - tminDisplay;
                if (a>0)
                    stepDisplay = stepDisplay/1000.;

                double t = 0;
                const int imax = (tmaxDisplay - tminDisplay +1) / stepDisplay;
                for (int i = 0; i<= imax; i++) {
                    t = tminDisplay + i*stepDisplay;
                    const double tRaw = DateUtils::convertFromAppSettingsFormat(t);
                    refCurve[t] = a * tRaw * tRaw + b * tRaw + c;
                }
                curve.mData = refCurve;
                mGraph->add_curve(curve);

               // Y scale and RangeY are define in graphView::zommX()



            } else if (mode == DATE_GAUSS_MODE_CURVE) {
                PluginGauss* plugin = static_cast<PluginGauss*> (date.mPlugin);

                const RefCurve &curve = plugin->mRefCurves.value(ref_curve);

                if (curve.mDataMean.isEmpty()) {
                    GraphZone zone;
                    zone.mColor = Qt::gray;
                    zone.mColor.setAlpha(75);
                    zone.mXStart = tminDisplay;
                    zone.mXEnd = tmaxDisplay;
                    zone.mText = tr("No reference data");
                    mGraph->add_zone(zone);
                    return;
                }

                if (tminDisplay < tminRef) {
                    GraphZone zone;
                    zone.mColor = QColor(217, 163, 69);
                    zone.mColor.setAlpha(75);
                    zone.mXStart = tminDisplay;
                    zone.mXEnd = tminRef;
                    zone.mText = tr("Outside reference area");
                    mGraph->add_zone(zone);
                }

                if (tmaxRef < tmaxDisplay) {
                    GraphZone zone;
                    zone.mColor = QColor(217, 163, 69);
                    zone.mColor.setAlpha(75);
                    zone.mXStart = tmaxRef;
                    zone.mXEnd = tmaxDisplay;
                    zone.mText = tr("Outside reference area");
                    mGraph->add_zone(zone);
                }

                QMap<double, double> curveG;
                QMap<double, double> curveG95Sup;
                QMap<double, double> curveG95Inf;

                /*
                 *  Define the first point, which is often not the first point of the ref curve
                 */

                if (tminDisplay>curve.mDataMean.firstKey() && tminDisplay<curve.mDataMean.lastKey()) {
                     const double v = curve.interpolate_mean(tminDisplay);
                     const double error = plugin->getRefErrorAt(date.mData, tminDisplay, mode) * 1.96;

                     curveG[tminDisplay] = v;
                     curveG95Sup[tminDisplay] = v + error;
                     curveG95Inf[tminDisplay] = v - error;

                }

                double t, tDisplay, error;
                for ( QMap<double, double>::const_iterator &&iPt = curve.mDataMean.cbegin();  iPt!=curve.mDataMean.cend(); ++iPt) {
                    t = iPt.key();
                    tDisplay = DateUtils::convertToAppSettingsFormat(t);

                    error = plugin->getRefErrorAt(date.mData, t, mode) * 1.96;

                    curveG[tDisplay] = iPt.value();
                    curveG95Sup[tDisplay] = iPt.value() + error;
                    curveG95Inf[tDisplay] = iPt.value() - error;

                }

                const GraphCurve &curveGEnv = shapeCurve(curveG95Inf, curveG95Sup, "G Env",
                                                 QColor(180, 180, 180), Qt::DashLine, QColor(180, 180, 180, 30), true);
                mGraph->add_curve(curveGEnv);

                GraphCurve graphCurveG = FunctionCurve(curveG, "G", Painting::mainColorDark );
                graphCurveG.mVisible = true;
                mGraph->add_curve(graphCurveG);
            }

            if (mode != DATE_GAUSS_MODE_NONE) {
                const double yMin = age - error * 4.;
                const double yMax = age + error * 4.;

                /* ----------------------------------------------
                 *  Measure curve
                 * ---------------------------------------------- */

                GraphCurve curveMeasure;
                curveMeasure.mName = "Measurement";
                curveMeasure.mPen.setColor(mMeasureColor);
                QColor curveColor(mMeasureColor);
                curveColor.setAlpha(50);
                curveMeasure.mBrush = curveColor;
                curveMeasure.mType = GraphCurve::CurveType::eVerticalQMap;

                /* 5000 pts are used on vertical measurement
                 * because the y scale auto adjusts depending on x zoom.
                 * => the visible part of the measurement may be very reduced ! */
                const double step = (yMax - yMin) / 4999.;
                QMap<double, double> measureCurve;
                measureCurve[yMin] = 0.;
                for (int i = 1; i< 4999; i++) {
                    const double y = yMin + i*step;
                    measureCurve[y] = exp(-0.5 * pow((y - age) / error, 2.));

                }
                measureCurve[yMax] = 0.;

                measureCurve = normalize_map(measureCurve);

                curveMeasure.mData = measureCurve;
                mGraph->add_curve(curveMeasure);


                /* ----------------------------------------------
                 *  Error on measure
                 * ---------------------------------------------- */

                GraphCurve curveMeasureAvg;
                curveMeasureAvg.mVisible = true;
                curveMeasureAvg.mName = "MeasureAvg";
                curveMeasureAvg.mPen.setColor(mMeasureColor);
                curveMeasureAvg.mPen.setStyle(Qt::SolidLine);
                curveMeasureAvg.mType = GraphCurve::CurveType::eHorizontalLine;

                GraphCurve curveMeasureSup;
                curveMeasureSup.mVisible = true;
                curveMeasureSup.mName = "MeasureSup";
                curveMeasureSup.mPen.setColor(mMeasureColor);
                curveMeasureSup.mPen.setStyle(Qt::DashLine);
                curveMeasureSup.mType = GraphCurve::CurveType::eHorizontalLine;

                GraphCurve curveMeasureInf;
                curveMeasureInf.mVisible = true;
                curveMeasureInf.mName = "MeasureInf";
                curveMeasureInf.mPen.setColor(mMeasureColor);
                curveMeasureInf.mPen.setStyle(Qt::DashLine);
                curveMeasureInf.mType = GraphCurve::CurveType::eHorizontalLine;

                curveMeasureAvg.mHorizontalValue = age;
                curveMeasureSup.mHorizontalValue = age + error;
                curveMeasureInf.mHorizontalValue = age - error;

                mGraph->add_curve(curveMeasureAvg);
                mGraph->add_curve(curveMeasureSup);
                mGraph->add_curve(curveMeasureInf);
            }
        }
    } else {
        
        double tminDisplay(mTminDisplay);
        double tmaxDisplay(mTmaxDisplay);

        const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
        const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);

        for (auto&& d : date.mSubDates ) {
            Date sd (d.toObject());

            if (!sd.isNull() && sd.mIsValid) {
                const double t3 = sd.getFormatedTminCalib();
                const double t4 = sd.getFormatedTmaxCalib();
    
                tminDisplay = qMin(t1, qMin(t2,t3));
                tmaxDisplay = qMax(t1, qMax(t2,t4));
            } else {
                tminDisplay = qMin(t1, t2);
                tmaxDisplay = qMax(t1, t2);
            }
        }
        GraphViewRefAbstract::drawSubDates(date.mSubDates, settings, tminDisplay, tmaxDisplay);
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
