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

#include "Plugin14CRefView.h"
#if USE_PLUGIN_14C

#include "Plugin14C.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"

#include <QtWidgets>

Plugin14CRefView::Plugin14CRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(98, 113, 123, 240);
    mGraph = new GraphView(this);
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("age");
    mGraph->autoAdjustYScale(true);
    setMouseTracking(true);
    mGraph->setMouseTracking(true);
}

Plugin14CRefView::~Plugin14CRefView()
{

}

void Plugin14CRefView::setDate(const Date &date, const StudyPeriodSettings& settings)
{
     GraphViewRefAbstract::setDate(date, settings);

     if (date.mOrigin == Date::eSingleDate) {

         double tminDisplay_formated_effective;
         double tmaxDisplay_formated_effective;

         const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
         const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);

         if (!date.isNull() && date.mIsValid) {
             const double t3 = date.getFormatedTminCalib();
             const double t4 = date.getFormatedTmaxCalib();

             tminDisplay_formated_effective = qMin(t1, qMin(t2, t3));
             tmaxDisplay_formated_effective = qMax(t1, qMax(t2, t4));

         } else {
             tminDisplay_formated_effective = qMin(t1, t2);
             tmaxDisplay_formated_effective = qMax(t1, t2);
         }


         mGraph->setRangeX(tminDisplay_formated_effective, tmaxDisplay_formated_effective);
         mGraph->setCurrentX(tminDisplay_formated_effective, tmaxDisplay_formated_effective);

         mGraph->removeAllCurves();
         mGraph->remove_all_zones();
         mGraph->clearInfos();
         mGraph->showInfos(true);
         mGraph->setFormatFunctX(nullptr);

         if (!date.isNull())  {
             double age = date.mData.value(DATE_14C_AGE_STR).toDouble();
             double error = date.mData.value(DATE_14C_ERROR_STR).toDouble();
             const double delta_r = date.mData.value(DATE_14C_DELTA_R_STR).toDouble();
             const double delta_r_error = date.mData.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();
             const QString ref_curve = date.mData.value(DATE_14C_REF_CURVE_STR).toString().toLower();

             /* ----------------------------------------------
              *  Reference curve
              * ---------------------------------------------- */

            const double tminRef_formated = date.getFormatedTminRefCurve();
            const double tmaxRef_formated = date.getFormatedTmaxRefCurve();

            Plugin14C* plugin = static_cast<Plugin14C* >(date.mPlugin);
            const RefCurve& curve = plugin->mRefCurves.value(ref_curve);

            if (curve.mDataMean.isEmpty()) {
                GraphZone zone;
                zone.mColor = Qt::gray;
                zone.mColor.setAlpha(75);
                zone.mXStart = tminDisplay_formated_effective;
                zone.mXEnd = tmaxDisplay_formated_effective;
                zone.mText = tr("No reference data");
                mGraph->add_zone(zone);
                return;
            }

            if (tminDisplay_formated_effective < tminRef_formated) {
                GraphZone zone;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mXStart = tminDisplay_formated_effective;
                zone.mXEnd = tminRef_formated;
                zone.mText = tr("Outside reference area");
                mGraph->add_zone(zone);
            }

            if (tmaxRef_formated < tmaxDisplay_formated_effective) {
                GraphZone zone;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mXStart = tmaxRef_formated;
                zone.mXEnd = tmaxDisplay_formated_effective;
                zone.mText = tr("Outside reference area");
                mGraph->add_zone(zone);
            }


            QMap<double, double> curveG;
            QMap<double, double> curveG95Sup;
            QMap<double, double> curveG95Inf;

             /*
              * We need to skim the real map to fit with the real value of the calibration curve
              * The graphView function does the interpolation between the real point
              */
             for ( QMap<double, double>::const_iterator &&iPt = curve.mDataMean.cbegin();  iPt!=curve.mDataMean.cend(); ++iPt) {
                 const double t (iPt.key());
                 const double tDisplay = DateUtils::convertToAppSettingsFormat(t);

                 const double error = plugin->getRefErrorAt(date.mData, t) * 1.96;

                 curveG[tDisplay] = iPt.value();
                 curveG95Sup[tDisplay] = iPt.value() + error;
                 curveG95Inf[tDisplay] = iPt.value() - error;

             }
             mGraph->setRangeX(tminDisplay_formated_effective, tmaxDisplay_formated_effective);
             mGraph->setCurrentX(tminDisplay_formated_effective, tmaxDisplay_formated_effective);

             GraphCurve graphCurveG = FunctionCurve(curveG, "G", Painting::mainColorDark );
             graphCurveG.mVisible = true;
             mGraph->add_curve(graphCurveG);


             const GraphCurve &curveGEnv = shapeCurve(curveG95Inf, curveG95Sup, "G Env",
                                              QColor(180, 180, 180), Qt::DashLine, QColor(180, 180, 180, 30), true);
             mGraph->add_curve(curveGEnv);

             /* ----------------------------------------------
              *  Measure curve
              * ---------------------------------------------- */

            double yMin = age - error * 3.;
            double yMax = age + error * 3.;
            GraphCurve curveMeasure;
            curveMeasure.mVisible = true;
            curveMeasure.mName = "Measurement";

            QColor penColor(mMeasureColor);
            QColor brushColor(mMeasureColor);

             // Lower opacity in case of delta r not null
             if (delta_r != 0. && delta_r_error != 0.) {
                 penColor.setAlpha(100);
                 brushColor.setAlpha(15);

             } else {
                 penColor.setAlpha(255);
                 brushColor.setAlpha(50);
             }
             curveMeasure.mPen = penColor;
             curveMeasure.mBrush = brushColor;
             curveMeasure.mType = GraphCurve::CurveType::eVerticalQMap;
             //curveMeasure.mIsVertical = true;
             //curveMeasure.mIsHisto = false;

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

             // Infos to write :
             //QString info = tr("Age BP : %1  ± %2").arg(locale().toString(age), locale().toString(error));

             /* ----------------------------------------------
              *  Delta R curve
              * ---------------------------------------------- */
             if (delta_r != 0. && delta_r_error != 0.) {
                 // Apply reservoir effect
                 age = (age - delta_r);
                 error = sqrt(error * error + delta_r_error * delta_r_error);

                 yMin = qMin(yMin, age - error * 3.);
                 yMax = qMax(yMax, age + error * 3.);

                 GraphCurve curveDeltaR;
                 curveDeltaR.mVisible = true;
                 curveDeltaR.mName = "Delta R";

                 penColor = mMeasureColor;
                 brushColor = mMeasureColor;
                 brushColor.setAlpha(50);

                 curveDeltaR.mPen = penColor;
                 curveDeltaR.mBrush = brushColor;

                 curveDeltaR.mType = GraphCurve::CurveType::eVerticalQMap;

                 /* 5000 pts are used on vertical measurement
                  * because the y scale auto adjusts depending on x zoom.
                  * => the visible part of the measure may be very reduced ! */

                 QMap<double, double> deltaRCurve;

                 deltaRCurve[yMin] = 0.;
                 for (int i = 0; i< 4999; i++) {
                     double y = yMin + i*step;
                     deltaRCurve[y] = exp(-0.5 * pow((age - y) / error, 2.));
                 }
                 deltaRCurve[yMax] = 0.;

                 deltaRCurve = normalize_map(deltaRCurve);
                 curveDeltaR.mData = deltaRCurve;
                 mGraph->add_curve(curveDeltaR);

                // info += tr(", ΔR : %1  ± %2").arg(locale().toString(delta_r), locale().toString(delta_r_error));
             }

             /* ----------------------------------------------
         *  Sub-dates curves (combination)
         * ---------------------------------------------- */

             for (auto && subDate: date.mSubDates) {
                 QJsonObject d = subDate.toObject();

                 GraphCurve curveSubMeasure;
                 curveSubMeasure.mVisible = true;
                 curveSubMeasure.mName = "Sub-Measurement : " + d.value(STATE_NAME).toString();// QString::number(i);

                 double sub_age = d.value(STATE_DATE_DATA).toObject().value(DATE_14C_AGE_STR).toDouble();
                 double sub_error = d.value(STATE_DATE_DATA).toObject().value(DATE_14C_ERROR_STR).toDouble();
                 double sub_delta_r = d.value(STATE_DATE_DATA).toObject().value(DATE_14C_DELTA_R_STR).toDouble();
                 double sub_delta_r_error = d.value(STATE_DATE_DATA).toObject().value(DATE_14C_DELTA_R_ERROR_STR).toDouble();

                 // Apply reservoir effect
                 sub_age = (sub_age - sub_delta_r);
                 sub_error = sqrt(sub_error * sub_error + sub_delta_r_error * sub_delta_r_error);

                 yMin = qMin(yMin, sub_age - sub_error * 3);
                 yMax = qMax(yMax, sub_age + sub_error * 3);

                 QColor penColor = QColor(167, 126, 73);
                 QColor brushColor = QColor(167, 126, 73);
                 penColor.setAlpha(255);
                 brushColor.setAlpha(50);

                 curveSubMeasure.mPen = penColor;
                 curveSubMeasure.mBrush = brushColor;
                 curveSubMeasure.mType = GraphCurve::CurveType::eVerticalQMap;


                 /* 5000 pts are used on vertical measurement
                  * because the y scale auto adjusts depending on x zoom.
                  * => the visible part of the measurement may be very reduced ! */

                 QMap<double, double> subCurve;
                 subCurve.insert(yMin, 0);
                 for (int i = 0; i< 4999; i++) {
                     double y = yMin + i*step;
                     double v = exp(-0.5 * pow((sub_age - y) / sub_error, 2.));
                     subCurve.insert(y, v);
                 }
                 subCurve.insert(yMax, 0);
                 subCurve = normalize_map(subCurve);
                 curveSubMeasure.mData = subCurve;
                 mGraph->add_curve(curveSubMeasure);
             }

             /* ----------------------------------------------
              *  Error on measure (horizontal lines)
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

             //Y scale and RangeY are define in graphView::zommX()

         }

     } else {

           double tminDisplay(mTminDisplay);
           double tmaxDisplay(mTmaxDisplay);

           const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
           const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);

           for (auto&& d : date.mSubDates ) {
               QJsonObject subDate = d.toObject();

               Date sd (subDate);

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

void Plugin14CRefView::zoomX(const double min, const double max)
{
    mGraph->zoomX(min, max);
}

void Plugin14CRefView::setMarginRight(const int margin)
{
    mGraph->setMarginRight(margin);
}

void Plugin14CRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(rect());
}

#endif
