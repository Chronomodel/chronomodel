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

#include "PluginMagRefView.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"

#include <QtWidgets>

PluginMagRefView::PluginMagRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(98, 113, 123, 240);
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

            const double incl = date.mData.value(DATE_AM_INC_STR).toDouble();
            const double decl = date.mData.value(DATE_AM_DEC_STR).toDouble();

            const double alpha95 = date.mData.value(DATE_AM_ALPHA95_STR).toDouble();
            const double field = date.mData.value(DATE_AM_FIELD_STR).toDouble();
            const double error_f = date.mData.value(DATE_AM_ERROR_F_STR).toDouble();

            const ProcessTypeAM pta = static_cast<ProcessTypeAM> (date.mData.value(DATE_AM_PROCESS_TYPE_STR).toInt());

            QString ref_curve;
            switch (pta) {
            case eInc:
                ref_curve = date.mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
                break;
            case eDec:
                ref_curve = date.mData.value(DATE_AM_REF_CURVED_STR).toString().toLower();
                break;
            case eField:
                ref_curve = date.mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();
                break;
          /*  case eID:
                mIDRadio->setChecked(true);
                break;
            case eIF:
                mIFRadio->setChecked(true);
                break;
            case eIDF:
                mIDFRadio->setChecked(true);
                break;
                */
            default:
                break;
            }

            if (ref_curve == "")
                return;

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
                mGraph->add_zone(zone);
                return;
            }

            if (tminDisplay < tminRef){
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
            QMap<double, double> curveG95Sup;
            QMap<double, double> curveG95Inf;


            for ( QMap<double, double>::const_iterator &&iPt = curve.mDataMean.cbegin();  iPt!=curve.mDataMean.cend(); ++iPt) {
                const double t (iPt.key());
                const double tDisplay = DateUtils::convertToAppSettingsFormat(t);

                const double error = plugin->getRefErrorAt(date.mData, t) * 1.96;

                curveG[tDisplay] = iPt.value();
                curveG95Sup[tDisplay] = iPt.value() + error;
                curveG95Inf[tDisplay] = iPt.value() - error;

                if (tDisplay>tminDisplay && tDisplay<tmaxDisplay) {
                   yMin = qMin(yMin, curveG95Inf.value(tDisplay));
                   yMax = qMax(yMax, curveG95Sup.value(tDisplay));
                }
            }

            mGraph->setRangeX(tminDisplay,tmaxDisplay);
            mGraph->setCurrentX(tminDisplay, tmaxDisplay);

            const GraphCurve &graphCurveG = FunctionCurve(curveG, "G", Painting::mainColorDark );
            mGraph->add_curve(graphCurveG);

            const GraphCurve &curveGEnv = shapeCurve(curveG95Inf, curveG95Sup, "G Env",
                                             QColor(180, 180, 180), Qt::DashLine, QColor(180, 180, 180, 30));
            mGraph->add_curve(curveGEnv);
            /* ----------------------------------------------
             *  Measure curve
             * ---------------------------------------------- */
            double error (0.);
            double avg (0.);

            switch (pta) {
            case eInc:
                avg = incl;
                error = alpha95 / 2.448;
                break;
            case eDec:
                avg = decl;
                error = alpha95 / (2.448 * cos(incl * M_PI / 180.));
                break;
            case eField:
                avg = field;
                error = error_f;
                break;
          /*  case eID:
                mIDRadio->setChecked(true);
                break;
            case eIF:
                mIFRadio->setChecked(true);
                break;
            case eIDF:
                mIDFRadio->setChecked(true);
                break;
                */
            default:
                break;
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

            curveMeasure.mType = GraphCurve::CurveType::eVerticalQMap;

            /* 5000 pts are used on vertical measurement
             * because the y scale auto adjusts depending on x zoom.
             * => the visible part of the measurement may be very reduced !
             */
            const double step = (yMax - yMin) / 4999.;
            QMap<double,double> measureCurve;

            measureCurve[yMin] = 0.;
            for (int i = 1; i< 4999; i++) {
                const double y = yMin + i*step;
                measureCurve[y] = exp(-0.5 * pow((y - avg) / error, 2.));

            }
            measureCurve[yMax] = 0.;


            measureCurve = normalize_map(measureCurve);
            curveMeasure.mData = measureCurve;
            mGraph->add_curve(curveMeasure);


            // ----------------------------------------------
            //  Error on measurement
            // ----------------------------------------------

            GraphCurve curveMeasureAvg;
            curveMeasureAvg.mName = "MeasureAvg";
            curveMeasureAvg.mPen.setColor(mMeasureColor);
            curveMeasureAvg.mPen.setStyle(Qt::SolidLine);
            curveMeasureAvg.mType = GraphCurve::CurveType::eHorizontalLine;

            GraphCurve curveMeasureSup;
            curveMeasureSup.mName = "MeasureSup";
            curveMeasureSup.mPen.setColor(mMeasureColor);
            curveMeasureSup.mPen.setStyle(Qt::DashLine);
            curveMeasureSup.mType = GraphCurve::CurveType::eHorizontalLine;

            GraphCurve curveMeasureInf;
            curveMeasureInf.mName = "MeasureInf";
            curveMeasureInf.mPen.setColor(mMeasureColor);
            curveMeasureInf.mPen.setStyle(Qt::DashLine);
            curveMeasureInf.mType = GraphCurve::CurveType::eHorizontalLine;

            curveMeasureAvg.mHorizontalValue = avg;
            curveMeasureSup.mHorizontalValue = avg + error;
            curveMeasureInf.mHorizontalValue = avg - error;

            mGraph->add_curve(curveMeasureAvg);
            mGraph->add_curve(curveMeasureSup);
            mGraph->add_curve(curveMeasureInf);
            mGraph->setFormatFunctY(nullptr);

            // Y scale and RangeY are define in graphView::zommX()

        }
    } else { // eCombineDate

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
