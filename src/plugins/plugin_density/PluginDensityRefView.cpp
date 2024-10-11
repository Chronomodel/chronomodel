/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include "PluginDensityRefView.h"
#if USE_PLUGIN_DENSITY

#include "PluginDensity.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"

#include <QtWidgets>

PluginDensityRefView::PluginDensityRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(98, 113, 123, 240);
    mGraph = new GraphView(this);

    mGraph->setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
    mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->autoAdjustYScale(true);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("x");

}

PluginDensityRefView::~PluginDensityRefView()
{

}

void PluginDensityRefView::setDate(const Date& date, const StudyPeriodSettings& settings)
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

            //const QString mode = date.mData.value(DATE_GAUSS_MODE_STR).toString();
            const QString ref_curve = date.mData.value(DATE_DENSITY_CURVE_STR).toString();

            /* ----------------------------------------------
             *  Reference curve
             * ---------------------------------------------- */

            const double tminRef = date.getFormatedTminRefCurve();
            const double tmaxRef = date.getFormatedTmaxRefCurve();

            PluginDensity* plugin = static_cast<PluginDensity*> (date.mPlugin);

            const RefCurve& curve = plugin->mRefCurves.value(ref_curve);

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

            const double t0 = DateUtils::convertFromAppSettingsFormat(qMax(tminDisplay, tminRef));
            double yMin = plugin->getRefValueAt(date.mData, t0);
            double yMax = yMin;

            QMap<double, double> curveG;


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


                curveG[tminDisplay] = v;


                yMin = qMin(yMin, v);
                yMax = qMax(yMax, v);
            }

            double t, tDisplay;
            for ( QMap<double, double>::const_iterator &&iPt = curve.mDataMean.cbegin();  iPt!=curve.mDataMean.cend(); ++iPt) {
                t = iPt.key();
                tDisplay = DateUtils::convertToAppSettingsFormat(t);

                curveG[tDisplay] = iPt.value();

                if (tDisplay>=tminDisplay && tDisplay<=tmaxDisplay) {
                    yMin = qMin(yMin, iPt.value());
                    yMax = qMax(yMax, iPt.value());
                }

            }


            const GraphCurve &graphCurveG = FunctionCurve(curveG, "G", Painting::mainColorDark );
            mGraph->add_curve(graphCurveG);
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

void PluginDensityRefView::zoomX(const double min, const double max)
{
    mGraph->zoomX(min, max);
}

void PluginDensityRefView::setMarginRight(const int margin)
{
    mGraph->setMarginRight(margin);
}

void PluginDensityRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(0, 0, width(), height());
}

#endif
