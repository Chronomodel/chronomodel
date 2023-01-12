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

#include "PluginF14CRefView.h"
#if USE_PLUGIN_F14C

#include "PluginF14C.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"

#include <QtWidgets>

PluginF14CRefView::PluginF14CRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(Qt::black);
    mGraph = new GraphView(this);
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eAllTicks);
    //mGraph->setRendering(GraphView::eHD);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("F14C");
    mGraph->autoAdjustYScale(true);
    setMouseTracking(true);
    mGraph->setMouseTracking(true);
}

PluginF14CRefView::~PluginF14CRefView()
{

}

void PluginF14CRefView::setDate(const Date& date, const ProjectSettings& settings)
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
         mGraph->removeAllZones();
         mGraph->clearInfos();
         mGraph->showInfos(true);
         mGraph->setFormatFunctX(nullptr);

         if (!date.isNull())  {
             double age = date.mData.value(DATE_F14C_FRACTION_STR).toDouble();
             double error = date.mData.value(DATE_F14C_ERROR_STR).toDouble();
             const QString ref_curve = date.mData.value(DATE_F14C_REF_CURVE_STR).toString().toLower();

             /* ----------------------------------------------
              *  Reference curve
              * ---------------------------------------------- */

             const double tminRef = date.getFormatedTminRefCurve();
             const double tmaxRef = date.getFormatedTmaxRefCurve();

             PluginF14C* plugin = static_cast<PluginF14C* >(date.mPlugin);
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

             if (tminDisplay < tminRef) {
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
             double yMax (yMin);

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


             /* ----------------------------------------------
              *  Measure curve
              * ---------------------------------------------- */
             yMin = qMin(yMin, age - error * 3);
             yMax = qMax(yMax, age + error * 3);

             GraphCurve curveMeasure;
             curveMeasure.mName = "Measurement";

             QColor penColor(mMeasureColor);
             QColor brushColor(mMeasureColor);

             // Lower opacity in case of delta r not null
             /*    if (delta_r != 0. && delta_r_error != 0.) {
            penColor.setAlpha(100);
            brushColor.setAlpha(15);

        } else {
 */           penColor.setAlpha(255);
             brushColor.setAlpha(50);
             //       }
             curveMeasure.mPen = penColor;
             curveMeasure.mBrush = brushColor;
             curveMeasure.mType = GraphCurve::CurveType::eVerticalQMap;

             /* 5000 pts are used on vertical measurement
              * because the y scale auto adjusts depending on x zoom.
              * => the visible part of the measurement may be very reduced ! */
             const double step = (yMax - yMin) / 5000.;
             QMap<double, double> measureCurve;

             measureCurve[yMin] = 0.;
             for (int i = 1; i<5000; i++) {
                 double y = yMin + i*step;
                 measureCurve[y] = exp(-0.5 * pow((y - age) / error, 2.));

             }
             measureCurve[yMax] = 0.;
             measureCurve = normalize_map(measureCurve);
             curveMeasure.mData = measureCurve;
             mGraph->addCurve(curveMeasure);


             /* ----------------------------------------------
              *  Sub-dates curves (combination)
              * ---------------------------------------------- */

             for (auto && subDate: date.mSubDates) {
                 QJsonObject d = subDate.toObject();
                 GraphCurve curveSubMeasure;
                 curveSubMeasure.mName = "Sub-Measurement : " + d.value(STATE_NAME).toString();// QString::number(i);

                 double sub_age = d.value(STATE_DATE_DATA).toObject().value(DATE_F14C_FRACTION_STR).toDouble();
                 double sub_error = d.value(STATE_DATE_DATA).toObject().value(DATE_F14C_ERROR_STR).toDouble();

                 yMin = qMin(yMin, sub_age - sub_error * 3);
                 yMax = qMax(yMax, sub_age + sub_error * 3);

                 QColor penColor = QColor(167, 126, 73);
                 QColor brushColor = QColor(167, 126, 73);
                 penColor.setAlpha(255);
                 brushColor.setAlpha(50);

                 curveSubMeasure.mPen = penColor;
                 curveSubMeasure.mBrush = brushColor;
                 curveSubMeasure.mType = GraphCurve::CurveType::eVerticalQMap;

                 const double step = (yMax - yMin) / 1000.;
                 QMap<double, double> subCurve;
                 for (double t = yMin; t<yMax; t += step) {
                     const double v = exp(-0.5 * pow((sub_age - t) / sub_error, 2.));
                     subCurve.insert(t, v);
                 }
                 subCurve = normalize_map(subCurve);
                 curveSubMeasure.mData = subCurve;
                 mGraph->addCurve(curveSubMeasure);
             }



             /* ----------------------------------------------
              *  Error on measure (horizontal lines)
              * ---------------------------------------------- */

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

             curveMeasureAvg.mHorizontalValue = age;
             curveMeasureSup.mHorizontalValue = age + error;
             curveMeasureInf.mHorizontalValue = age - error;

             mGraph->addCurve(curveMeasureAvg);
             mGraph->addCurve(curveMeasureSup);
             mGraph->addCurve(curveMeasureInf);

             //Y scale and RangeY are define in graphView::zommX()

         }
   } else {
       
       const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
       const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);
       
       double tminDisplay = t1;
       double tmaxDisplay = t1;

         for (auto&&d : date.mSubDates ) {
             Date sd (d.toObject());

             if (!date.isNull() && date.mIsValid) {
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

void PluginF14CRefView::zoomX(const double min, const double max)
{
    mGraph->zoomX(min, max);
}

void PluginF14CRefView::setMarginRight(const int margin)
{
    mGraph->setMarginRight(margin);
}

void PluginF14CRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(rect());
}

#endif
