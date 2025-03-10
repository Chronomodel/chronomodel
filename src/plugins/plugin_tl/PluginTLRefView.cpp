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

#include "PluginTLRefView.h"
#if USE_PLUGIN_TL

#include "PluginTL.h"
#include "GraphView.h"
#include "Painting.h"
#include "StdUtilities.h"

#include <QtWidgets>

PluginTLRefView::PluginTLRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(98, 113, 123, 240);
    mGraph = new GraphView(this);

    mGraph->setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
    mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    //mGraph->setRendering(GraphView::eHD);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("age");
    mGraph->autoAdjustYScale(true);

}

PluginTLRefView::~PluginTLRefView()
{

}

void PluginTLRefView::setDate(const Date& date, const StudyPeriodSettings& settings)
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
                double age = date.mData.value(DATE_TL_AGE_STR).toDouble();
                double error = date.mData.value(DATE_TL_ERROR_STR).toDouble();
                double ref_year = date.mData.value(DATE_TL_REF_YEAR_STR).toDouble();

                /* ----------------------------------------------
                 *  Reference curve
                 * ---------------------------------------------- */
                GraphCurve curve;
                curve.mVisible = true;
                curve.mName = "Reference";
                curve.mPen.setColor(Painting::mainColorDark);
                //curve.mIsHisto = false;
                QMap<double, double> refCurve;

                refCurve[tminDisplay] = ref_year - DateUtils::convertFromAppSettingsFormat(tminDisplay);
                refCurve[tmaxDisplay] = ref_year - DateUtils::convertFromAppSettingsFormat(tmaxDisplay);
                curve.mData =refCurve;
                //curve.mType = GraphCurve::CurveType::eDensityData;
                mGraph->add_curve(curve);

                // ----------------------------------------------

                double yMin = map_min(curve.mData).value();
                double yMax = map_max(curve.mData).value();

                yMin = qMin(yMin, age - error * 3);
                yMax = qMax(yMax, age + error * 3);

                 // Y scale and RangeY are define in graphView::zommX()


                /* ----------------------------------------------
                 *  Measure curve
                 * ---------------------------------------------- */

                GraphCurve curveMeasure;
                curveMeasure.mVisible = true;
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
                QMap<double,double> measureCurve;

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
                curveMeasure.mType = GraphCurve::CurveType::eHorizontalLine;

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

            // Y scale and RangeY are define in graphView::zommX()

        }
    } else {
        
        double tminDisplay (mTminDisplay);
        double tmaxDisplay (mTmaxDisplay);

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

void PluginTLRefView::zoomX(const double min, const double max)
{
    mGraph->zoomX(min, max);
}
void PluginTLRefView::setMarginRight(const int margin)
{
    mGraph->setMarginRight(margin);
}
void PluginTLRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(0, 0, width(), height());
}

#endif
