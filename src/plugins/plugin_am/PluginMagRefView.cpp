/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE
    Komlan NOUKPOAPE

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
#include "Generator.h"

#include "fftw3.h"

#include <QtWidgets>

const int nb_iter = 100; // iter MCMC
const int Ns = 20; // integration shrinkage

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

void PluginMagRefView::setDate(const Date &date, const StudyPeriodSettings &settings)
{
    GraphViewRefAbstract::setDate(date, settings);

    if (date.mOrigin == Date::eSingleDate) {

        double tminDisplay;
        double tmaxDisplay;
        const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
        const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);

        PluginMag* plugin = static_cast<PluginMag*>(date.mPlugin);
        const QString &ref_curve_I = date.mData.value(DATE_AM_REF_CURVEI_STR).toString().toLower();
        const QString &ref_curve_D = date.mData.value(DATE_AM_REF_CURVED_STR).toString().toLower();
        const QString &ref_curve_F = date.mData.value(DATE_AM_REF_CURVEF_STR).toString().toLower();


        RefCurve curve;

        if (!date.isNull() && date.mIsValid) {
            const double t3 = date.getFormatedTminCalib();
            const double t4 = date.getFormatedTmaxCalib();

            tminDisplay = std::min({t1, t2, t3});
            tmaxDisplay = std::max({t1, t2, t4});

        } else {
            tminDisplay = std::min(t1, t2);
            tmaxDisplay = std::max(t1, t2);
        }

        mGraph->setRangeX(tminDisplay, tmaxDisplay);
        mGraph->setCurrentX(tminDisplay, tmaxDisplay);

        mGraph->removeAllCurves();
        mGraph->remove_all_zones();
        mGraph->clearInfos();
        mGraph->showInfos(true);
        mGraph->setFormatFunctX(nullptr);

        double MCMC_mean;
        if (!date.isNull()) {

            const double incl = date.mData.value(DATE_AM_INC_STR).toDouble();
            const double decl = date.mData.value(DATE_AM_DEC_STR).toDouble();

            const double alpha95 = date.mData.value(DATE_AM_ALPHA95_STR).toDouble();
            const double field = date.mData.value(DATE_AM_FIELD_STR).toDouble();
            const double error_f = date.mData.value(DATE_AM_ERROR_F_STR).toDouble();

            const ProcessTypeAM pta = static_cast<ProcessTypeAM> (date.mData.value(DATE_AM_PROCESS_TYPE_STR).toInt());

            switch (pta) {
                case eInc:
                    curve = plugin->mRefCurves.value(ref_curve_I);
                    break;
                case eDec:
                    curve = plugin->mRefCurves.value(ref_curve_D);
                    break;
                case eField:
                    curve = plugin->mRefCurves.value(ref_curve_F);
                    break;

                // Combinaison
                case eID:
                    curve = combine_curve_ID(incl, decl, alpha95, plugin->mRefCurves.value(ref_curve_I), plugin->mRefCurves.value(ref_curve_D), MCMC_mean);
                    break;
                case eIF:
                    curve = combine_curve_IF(incl, alpha95, field, error_f, plugin->mRefCurves.value(ref_curve_I), plugin->mRefCurves.value(ref_curve_F), MCMC_mean);
                    break;
                case eIDF:
                    curve = combine_curve_IDF(incl, decl, alpha95, field, error_f, plugin->mRefCurves.value(ref_curve_I), plugin->mRefCurves.value(ref_curve_D), plugin->mRefCurves.value(ref_curve_F), MCMC_mean);
                    break;

                default:
                    break;
            }

            /* ----------------------------------------------
             *  Reference curve
             * ---------------------------------------------- */

            double tminRef = date.getFormatedTminRefCurve();
            double tmaxRef = date.getFormatedTmaxRefCurve();

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

            for (auto [t, value] : curve.mDataMean.asKeyValueRange()) {
                const double tDisplay = DateUtils::convertToAppSettingsFormat(t);

                const double error = curve.interpolate_error(t) * 1.96;

                curveG[tDisplay] = value;
                curveG95Sup[tDisplay] = value + error;
                curveG95Inf[tDisplay] = value - error;

                if (tDisplay>tminDisplay && tDisplay<tmaxDisplay) {
                   yMin = qMin(yMin, curveG95Inf.value(tDisplay));
                   yMax = qMax(yMax, curveG95Sup.value(tDisplay));
                }
            }

            mGraph->setRangeX(tminDisplay,tmaxDisplay);
            mGraph->setCurrentX(tminDisplay, tmaxDisplay);

            GraphCurve graphCurveG = FunctionCurve(curveG, "G", Painting::mainColorDark );
            graphCurveG.mVisible = true;
            mGraph->add_curve(graphCurveG);

            const GraphCurve &curveGEnv = shapeCurve(curveG95Inf, curveG95Sup, "G Env",
                                             QColor(180, 180, 180), Qt::DashLine, QColor(180, 180, 180, 30), true);
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

                case eID:
                case eIF:
                case eIDF:
                    avg = MCMC_mean;
                    error = 0.;
                    break;
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
            if (pta==eInc || pta==eDec || pta==eField) {
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
                curveMeasure.mVisible = true;
                mGraph->add_curve(curveMeasure);
            }


            // ----------------------------------------------
            //  Error on measurement
            // ----------------------------------------------

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

            switch (pta) {
                case eInc:
                case eDec:
                case eField:
                    curveMeasureAvg.mHorizontalValue = avg;
                    curveMeasureSup.mHorizontalValue = avg + error;
                    curveMeasureInf.mHorizontalValue = avg - error;

                    mGraph->add_curve(curveMeasureAvg);
                    mGraph->add_curve(curveMeasureSup);
                    mGraph->add_curve(curveMeasureInf);
                break;
                case eID:
                case eIF:
                case eIDF:
                    curveMeasureAvg.mHorizontalValue = MCMC_mean;
                    curveMeasureInf.mHorizontalValue = 0.;

                    mGraph->add_curve(curveMeasureAvg);
                    mGraph->add_curve(curveMeasureInf);
                    mGraph->setMinimumY(0.);
                 break;
                default:
                break;
            }


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

void PluginMagRefView::resizeEvent(QResizeEvent* )
{
    mGraph->setGeometry(rect());
}

RefCurve PluginMagRefView::combine_curve_ID(double incl,  double decl, const double alpha95, const RefCurve &curve_I, const RefCurve &curve_D, double &mean_date) const
{
    RefCurve result;

    // common interval
    const double min_ti = std::max(curve_I.mTmin, curve_D.mTmin);
    const double max_ti = std::min(curve_I.mTmax, curve_D.mTmax);

    if (min_ti>= max_ti)
        return result;

    double min_step = std::max(curve_I.mMinStep, curve_D.mMinStep);

    int nb_pts = ceil((max_ti - min_ti)/min_step);
    // step correction
    min_step = (max_ti - min_ti) / nb_pts;
    // add a cell because nb_pts = (max_ti - min_ti)/min_step) +1
    ++nb_pts;

    incl = incl *rad;
    decl = decl *rad;
    const double s0xy = alpha95/140;

    double mean = 0.;
    double variance = 0.;
    double previousMean = 0.;
    double previousVariance = 0.;

    mean_date = 0.;
    double previousMean_date = 0.;
    int iter_date = 0;

    for (int i_pts = 0; i_pts < nb_pts; ++i_pts) {
        const double t = min_ti + i_pts*min_step;

        const double incl_t = curve_I.interpolate_mean(t) * rad;
        const double decl_t = curve_D.interpolate_mean(t) *rad ;

        const double x_t = sin(incl) * cos(incl_t) * cos(decl_t - decl) - cos(incl)*sin(incl_t);
        const double y_t = cos(incl_t)*sin(decl_t - decl);

        const double Sxy_t = curve_I.interpolate_error(t) * 2.448 / 140 ;


        for (int iter = 0; iter < nb_iter; ++iter) {
            // random sampling around the point to be dated
            const double xID = Generator::gaussByBoxMuller(0, s0xy);
            const double yID = Generator::gaussByBoxMuller(0, s0xy);

            // Calculation for ref point
            const double xID1 = Generator::gaussByBoxMuller(0, s0xy);
            const double yID1 = Generator::gaussByBoxMuller(0, s0xy);

            const double d0 = sqrt(pow(xID-xID1 ,2) + pow(yID-yID1 ,2)) / rad;
            ++iter_date;
            previousMean_date = std::move(mean_date);
            mean_date = previousMean_date + (d0 - previousMean_date) / iter_date;

            // random sampling around the point on the curve
            const double xID_t = Generator::gaussByBoxMuller(x_t, Sxy_t);
            const double yID_t = Generator::gaussByBoxMuller(y_t, Sxy_t);

            const double d = sqrt(pow(xID-xID_t ,2) + pow(yID-yID_t ,2)) / rad;

            previousMean = std::move(mean);
            previousVariance = std::move(variance);
            mean = previousMean + (d - previousMean) / (iter+1);
            variance = previousVariance + ( d - previousMean)*( d - mean);

        }
        variance /= nb_iter;

        result.mDataMean[t] = mean;
        result.mDataError[t] = sqrt(variance);
    }
    result.mMinStep = min_step;
    result.mTmin = min_ti;
    result.mTmax = max_ti;

    result.mDataMean = gaussian_filter(result.mDataMean, (max_ti-min_ti)/1000.);
    result.mDataError = gaussian_filter(result.mDataError, (max_ti-min_ti)/1000.);

    return result;
}

RefCurve PluginMagRefView::combine_curve_IF(double incl, const double alpha95, const double field, const double error_f, const RefCurve &curve_I, const RefCurve &curve_F, double &mean_date) const
{
    RefCurve result;

    // common interval
    const double min_ti = std::max(curve_I.mTmin, curve_F.mTmin);
    const double max_ti = std::min(curve_I.mTmax, curve_F.mTmax);

    if (min_ti>= max_ti)
        return result;

    double min_step = std::max(curve_I.mMinStep, curve_F.mMinStep);

    int nb_pts = ceil((max_ti - min_ti)/min_step);
    // step correction
    min_step = (max_ti - min_ti) / nb_pts;
    // add a cell because nb_pts = (max_ti - min_ti)/min_step) +1
    ++nb_pts;

    const double sI = alpha95/2.448;

    // Calculation of s0IF
    const double s0IF = 0.5*(pow(error_f, 2) + pow(sI*field*rad , 2.));

    double w = 0.;

    std::vector<double> sIF;
    for (int i = 0; i < Ns; ++i) {
        w += 1./(double)Ns;
        sIF.push_back(s0IF * ((1-w)/w));
    }


    mean_date = 0.;
    double previousMean_date = 0.;
    int iter_date = 0;
    for (int i_pts = 0; i_pts < nb_pts; ++i_pts) {

        double mean = 0.;
        double variance = 0.;
        double previousMean = 0.;
        double previousVariance = 0.;

        const double t = min_ti + i_pts*min_step;

        const double I_t = curve_I.interpolate_mean(t);
        const double F_t = curve_F.interpolate_mean(t);

        const double sI_t  = curve_I.interpolate_error(t);
        const double sF_t  = curve_F.interpolate_error(t);



        for (int iter = 0; iter < nb_iter; ++iter) {
            // random sampling around the point to be dated
            const double I = Generator::gaussByBoxMuller(incl, sI);
            const double F = Generator::gaussByBoxMuller(field, error_f);

            // Calculation for ref point
            const double I0 = Generator::gaussByBoxMuller(incl, sI);
            const double F0 = Generator::gaussByBoxMuller(field, error_f);

            // random sampling around the point on the curve
            const double I0_t = Generator::gaussByBoxMuller(I_t, sI_t);
            const double F0_t = Generator::gaussByBoxMuller(F_t, sF_t);


            double p1 = 0;
            double p = 0;
            for (auto sif : sIF) {
                const double sF11 = 2.*pow(error_f, 2) + sif;
                const double sI11 = 2.*pow(sI, 2) + (sif / pow(F, 2.));

                const double pIF11 = pow(F-F0, 2.)/sF11 + pow(I-I0, 2.)/sI11;
                p1 += exp(-0.5*pIF11)/(sqrt(sF11) * sqrt(sI11)) ;

                // Curve mean
                const double sF1 = pow(error_f, 2) + sif + pow(sF_t, 2.);
                const double sI1 = pow(sI, 2) + (sif / pow(F, 2.))  + pow(sI_t, 2.);

                const double pIF1 = pow(F-F0_t, 2.)/sF1 + pow((I-I0_t), 2.)/sI1;
                p += exp(-0.5*pIF1)/(sqrt(sF1) * sqrt(sI1)) ;
            }

            const double d0 = sqrt(-log(p1/(nb_pts*Ns)));

            ++iter_date;
            previousMean_date = std::move(mean_date);
            mean_date = previousMean_date + (d0 - previousMean_date) / iter_date;

            const double d = sqrt(-log(p/(nb_pts*Ns)));

            previousMean = std::move(mean);
            previousVariance = std::move(variance);
            mean = previousMean + (d - previousMean) / (iter+1);
            variance = previousVariance + ( d - previousMean)*( d - mean);

        }
        variance /= nb_iter;

        result.mDataMean[t] = mean;
        result.mDataError[t] = sqrt(variance);
    }


    result.mDataMean = gaussian_filter(result.mDataMean, (max_ti-min_ti)/1000.);
    result.mDataError = gaussian_filter(result.mDataError, (max_ti-min_ti)/1000.);

    result.mMinStep = min_step;
    result.mTmin = min_ti;
    result.mTmax = max_ti;

    return result;
}


RefCurve PluginMagRefView::combine_curve_IDF(double incl, double decl, const double alpha95, double field, const double error_f, const RefCurve &curve_I, const RefCurve &curve_D, const RefCurve &curve_F, double &mean_date) const
{
    RefCurve result;// common interval

    const double min_ti = std::max({curve_I.mTmin, curve_D.mTmin, curve_F.mTmin});
    const double max_ti = std::min({curve_I.mTmax, curve_D.mTmax, curve_F.mTmax});

    if (min_ti>= max_ti)
        return result;

    double min_step = std::max({curve_I.mMinStep, curve_D.mMinStep, curve_F.mMinStep});

    int nb_pts = ceil((max_ti - min_ti)/min_step);
    // step correction
    min_step = (max_ti - min_ti) / nb_pts;
    // add a cell because nb_pts = (max_ti - min_ti)/min_step) +1
    ++nb_pts;

    const double sI = alpha95/2.448;
    const double sD = alpha95/(2.448*cos(incl*rad));

    const double s0IDF = (pow(error_f, 2.) + 2*pow(sI*field, 2.)) /3.;


    double w = 0.;

    std::vector<double> sIDF;
    for (int i = 0; i < Ns; ++i) {
        w += 1./(double)Ns;
        sIDF.push_back(s0IDF * ((1-w)/w));
    }

    mean_date = 0.; // calculation on the whole curve
    double previousMean_date = 0.;
    int iter_date = 0;

    for (int i_pts = 0; i_pts < nb_pts; ++i_pts) {
        double mean = 0.;
        double variance = 0.;
        double previousMean = 0.;
        double previousVariance = 0.;

        const double t = min_ti + i_pts*min_step;

        const double I_t = curve_I.interpolate_mean(t);
        const double D_t = curve_D.interpolate_mean(t);
        const double F_t = curve_F.interpolate_mean(t);

        const double sI_t  = curve_I.interpolate_error(t);
        const double sD_t  = curve_D.interpolate_error(t);
        const double sF_t  = curve_F.interpolate_error(t);

        for (int iter = 0; iter < nb_iter; ++iter) {

            // random sampling around the point to be dated
            const double I = Generator::gaussByBoxMuller(incl, sI);
            const double D = Generator::gaussByBoxMuller(decl, sD);
            const double F = Generator::gaussByBoxMuller(field, error_f);

            // Calculation for ref point
            const double I0 = Generator::gaussByBoxMuller(incl, sI);
            const double D0 = Generator::gaussByBoxMuller(decl, sD);
            const double F0 = Generator::gaussByBoxMuller(field, error_f);

            // random sampling around the point on the curve
            const double I0_t = Generator::gaussByBoxMuller(I_t, sI_t);
            const double D0_t = Generator::gaussByBoxMuller(D_t, sD_t);
            const double F0_t = Generator::gaussByBoxMuller(F_t, sF_t);

            double p = 0;
            double p1 = 0;
            for (auto sidf : sIDF) {
                const double sF11 = 2.*pow(error_f, 2.) + sidf;
                const double sI11 = 2.*pow(sI, 2.) + (sidf / pow(field, 2.));
                const double sD11 = 2.*pow(sD, 2.) + (sidf / (pow(field * cos(I*rad), 2.)));

                const double pIDF11 = pow(F-F0, 2.)/sF11 + pow(I-I0, 2.)/sI11 + pow(D-D0, 2.)/sD11;
                p1 += exp(-0.5*pIDF11)/(sqrt(sF11) * sqrt(sI11) * sqrt(sD11));

                //Curve mean
                const double sF1 = pow(error_f, 2.) + sidf + pow(sF_t, 2.);
                const double sI1 = pow(sI, 2.) + (sidf / pow(field, 2.)) + pow(sI_t, 2.);
                const double sD1 = pow(sD, 2.) + (sidf / (pow(field * cos(incl*rad), 2.))) + pow(sD_t, 2.);

                const double pIDF1 = pow(F-F0_t, 2.)/sF1 + (pow(I-I0_t, 2.)/sI1 + pow(D-D0_t, 2.)/sD1);
                p += exp(-0.5*pIDF1)/(sqrt(sF1) * sqrt(sI1) * sqrt(sD1)) ;
            }

            const double d0 = sqrt(-log(p1/(nb_pts*Ns)));

            ++iter_date;
            previousMean_date = std::move(mean_date);
            mean_date = previousMean_date + (d0 - previousMean_date) / iter_date;

            const double d = sqrt(-log(p/(nb_pts*Ns)));

            previousMean = std::move(mean);
            previousVariance = std::move(variance);
            mean = previousMean + (d - previousMean) / (iter+1);
            variance = previousVariance + ( d - previousMean)*( d - mean);

        }
        variance /= nb_iter;

        result.mDataMean[t] = mean;
        result.mDataError[t] = sqrt(variance);
    }

    result.mDataMean = gaussian_filter(result.mDataMean, (max_ti-min_ti)/1000.);
    result.mDataError = gaussian_filter(result.mDataError, (max_ti-min_ti)/1000.);

    result.mMinStep = min_step;
    result.mTmin = min_ti;
    result.mTmax = max_ti;

    return result;
}

/**
 * @brief gaussian_filter, we assume a uniform step between values.
 * @param map
 * @param sigma, of the gaussian
 * @return
 */
QMap<double, double> gaussian_filter(QMap<double, double> &map, const double sigma)
{
    std::vector<double> curve_input;

    const double min = map.firstKey();
    const double max = map.lastKey();

    const double step = (max - min) / (map.size()-1);

    for (auto [key, value] : map.asKeyValueRange()) {
        curve_input.push_back(value);
    }

    /* ----- FFT -----
        http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        https://jperalta.wordpress.com/2006/12/12/using-fftw3/
    */



    //qDebug() <<"filtre Gaussian";
    //  data
    const int inputSize = curve_input.size();

    const double sigma_filter = sigma * step;

    const int gaussSize = std::max(inputSize, int(3*sigma));
    const int paddingSize = 2*gaussSize;

    const int N = gaussSize + 2*paddingSize;
    //const int NComplex = 2* (N/2)+1;

    const int NComplex =  (N/2)+1;

    // https://www.fftw.org/fftw3_doc/Real_002ddata-DFT-Array-Format.html

    double *inputReal;
    inputReal = new double [N];


    fftw_complex *inputComplex;
    inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);


    // we could use std::copy
    for (int i  = 0; i< paddingSize; i++) {
        inputReal[i] = curve_input[0];//0.;
    }
    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = curve_input[i];
    }
    for (int i ( inputSize+paddingSize); i< N; i++) {
        inputReal[i] = curve_input[inputSize-1];//0.;
    }
    fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);

    fftw_execute(plan_input);

    for (int i = 0; i < NComplex; ++i) {
        const double s =  M_PI * (double)i / (double)NComplex;
        const double factor = exp(-2. * pow(s * sigma_filter, 2.));
        if (isnan(factor)) {
            qDebug()<<"gaussian filter"<< s << " isnan";
        }
        inputComplex[i][0] *= factor;
        inputComplex[i][1] *= factor;

    }


    double *outputReal;
    outputReal = new double [2* (N/2)+1];//;[N];


    fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, inputComplex, outputReal, FFTW_ESTIMATE);
    fftw_execute(plan_output);

    QMap<double, double> results;
    for ( int i = 0; i < inputSize; i++) {
        const double t = min + i* step;
        results[t] = outputReal[i + paddingSize]/N;
#ifdef DEBUG
        if (isnan(results[t])) {
            qDebug()<<"gaussian filter"<<t<< " isnan";
        }
#endif
    }

    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_output);
    fftw_free(inputComplex);

    delete [] inputReal;
    delete [] outputReal;

    fftw_cleanup();
    return results;
}

/**
 * @brief low_pass_filter, Since the signal exhibits a discontinuity,
 * the filtered signal exhibits a significant disturbance at the extremities.
 * (see Hanning filter)
 * @param map
 * @param Tc, Cut-off period, equal to 1/(cut-off frequency)
 * @return
 */

QMap<double, double> low_pass_filter(QMap<double, double> &map, const double Tc)
{
    const double min = map.firstKey();
    const double max = map.lastKey();

    const double step = (max - min) / (map.size()-1);

    /*for (auto [key, value] : map.asKeyValueRange()) {
        //cout << key << ": " << value << Qt::endl;
        curve_input.push_back(value);
    }*/

    /* ----- FFT -----
     http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
     https://jperalta.wordpress.com/2006/12/12/using-fftw3/
    */
    qDebug() <<"[low_pass_filter] Tc ="<<Tc;

    const int inputSize = map.size();
    const int paddingSize = 1*inputSize;

    const int N = inputSize + 2*paddingSize;

    const int NComplex = 2* (N/2)+1;

    double *inputReal;
    inputReal = new double [N];

    fftw_complex *inputComplex;
    inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);

    for (int i  = 0; i< paddingSize; i++) {
        inputReal[i] = 0;
    }
    auto begin_map = map.begin();
    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = begin_map++.value();
    }
    for (int i = inputSize+paddingSize; i< N; i++) {
        inputReal[i] = 0;
    }

    fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);
    fftw_execute(plan_input);

    const int n_cut = 3*(max-min)/(Tc *step); //3 = 2*padding +inputSize = 2*(1*inputSize) + inputsizeNComplex *
    //qDebug()<<"n_cut"<<n_cut << "NComplex" << NComplex;

    for (int i = 0; i < NComplex; ++i) {

        if (i < n_cut) {
            inputComplex[i][0] *= 1;
            inputComplex[i][1] *= 1;

        } else {
            inputComplex[i][0] *= 0;
            inputComplex[i][1] *= 0;
        }
    }


    double *outputReal;
    outputReal = new double [N];

    fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, inputComplex, outputReal, FFTW_ESTIMATE);
    fftw_execute(plan_output);

    /*for ( int i = 0; i < N ; i++) {
        const double t = min - paddingSize* step + i* step;
        results[t]= outputReal[i]/N;
    }*/

    QMap<double, double> results;
    for ( int i = 0; i < inputSize; i++) {
        const double t = min + i* step;
        results[t] = outputReal[i + paddingSize]/N;
    }

    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_output);
    fftw_free(inputComplex);
    delete [] inputReal;
    delete [] outputReal;
    fftw_cleanup();

    return results;
}


/*  function w = hanning(varargin)
%   HANNING   Hanning window.
%   HANNING(N) returns the N-point symmetric Hanning window in a column
%   vector.  Note that the first and last zero-weighted window samples
%   are not included.
%
%   HANNING(N,'symmetric') returns the same result as HANNING(N).
%
%   HANNING(N,'periodic') returns the N-point periodic Hanning window,
%   and includes the first zero-weighted window sample.
%
%   NOTE: Use the HANN function to get a Hanning window which has the
%          first and last zero-weighted samples.ep
    itype = 1 --> periodic
    itype = 0 --> symmetric
    default itype=0 (symmetric)

    Copyright 1988-2004 The MathWorks, Inc.
%   $Revision: 1.11.4.3 $  $Date: 2007/12/14 15:05:04 $
*/

// https://en.wikipedia.org/wiki/Window_function#A_list_of_window_functions

double *hanning(int L, int N_fft)
{

/*
    for (i=0; i<N/2; i++) { // triangle Barttle
        w[i] = 4.*(double)i/(double)N;
    }
    for (i=N/2; i<N; i++) {
        w[i] = w[N-i];
    }
*/


    /*
     for (i=0; i<N; i++) { // Uniform, gives the average
        w[i] = 1./(double(N));
    }*/

    double *input;
    input = new double [N_fft];

    const int start_L = (N_fft-L)/2;
    const int middle = N_fft/2;

    int i  = 0;
    for (; i< start_L; i++) {
        input[i] = 0;
    }
    for (; i<= middle; i++) {
        const double z = 2.*M_PI*(double)(i-start_L)/(double)L;
        //input[i] = 0.35875 - 0.48829*cos(z) + 0.14128*cos(2*z) + 0.01168*cos(3*z); // Blackman–Harris window
        input[i] = 0.5 - 0.5*cos(z);
    }

    for (; i<= (start_L + L); i++) {
        input[i] = input[2*middle-i];
    }
    for (; i< N_fft; i++) {
        input[i] = 0;
    }


    return std::move(input);
}

// See how to use the hamming TF directly
// https://www.wolframalpha.com/input?i=FT+0.53836+-+0.46164*cos(z)
double *hamming(int L, int N_fft)
{
    double *input;
    input = new double [N_fft];

    const int start_L = (N_fft-L)/2;
    const int middle = N_fft/2;

    int i  = 0;
    for (; i< start_L; i++) {
        input[i] = 0;
    }
    for (; i<= middle; i++) {
        const double z = 2.*M_PI*(double)(i-start_L)/(double)L;
        input[i] = 0.53836 - 0.46164*cos(z);
    }
    for (; i<= (start_L + L); i++) {
        input[i] = input[2*middle-i];
    }
    for (; i< N_fft; i++) {
        input[i] = 0;
    }

    return std::move(input);
}

// https://en.wikipedia.org/wiki/Window_function#Hann_and_Hamming_windows
QMap<double, double> window_filter(QMap<double, double> &map, const double L)
{

    std::vector<double> curve_input;
    QMap<double, double> results;

    const double min = map.firstKey();
    const double max = map.lastKey();

    const double step = (max - min) / (map.size()-1);

    for (auto [key, value] : map.asKeyValueRange()) {
        //cout << key << ": " << value << Qt::endl;
        curve_input.push_back(value);
    }

    /* ----- FFT -----
        http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        https://jperalta.wordpress.com/2006/12/12/using-fftw3/
    */
    qDebug() <<"filtre hanning";
    //  data
    const int inputSize = curve_input.size();

    const int paddingSize = 2*inputSize;

    const int N = inputSize + 1*paddingSize;
    const int NComplex = 2* (N/2)+1;

    double *inputReal;
    inputReal = new double [N];

    fftw_complex *inputComplex;
    inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);


    // we could use std::copy
    for (int i  = 0; i< paddingSize; i++) {
        inputReal[i] = 0;
    }
    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = curve_input[i];
    }
    for (int i = inputSize+paddingSize; i< N; i++) {
        inputReal[i] = 0;
    }
    fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);
    fftw_execute(plan_input);

    double* hannReal = hamming(L*step, N);
    double somHann = 0.;
    for (int i= 0; i<N; ++i) {
        somHann += hannReal[i];
    }
    fftw_complex *hannComplex;
    hannComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);
    fftw_plan plan_hann = fftw_plan_dft_r2c_1d(N, hannReal, hannComplex, FFTW_ESTIMATE);
    fftw_execute(plan_hann);

    double *outputReal;
    outputReal = new double [N];

    fftw_complex *outputComplex;
    outputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);

    for (int i = 0; i<NComplex; ++i) {
        outputComplex[i][0] = hannComplex[i][0] * inputComplex[i][0] - hannComplex[i][1] * inputComplex[i][1];
        outputComplex[i][1] = hannComplex[i][0] * inputComplex[i][1] + hannComplex[i][1] * inputComplex[i][0];
    }

    fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, outputComplex, outputReal, FFTW_ESTIMATE);
    fftw_execute(plan_output);

    const int shift_padding = (inputSize/2)*step;
    //qDebug()<<"debut datat inputSize="<<inputSize<<" shift_padding="<< shift_padding<< "N="<<N<< " somHanning"<<somHann;


    for ( int i = 0; i < inputSize ; i++) {
        const double t = min + i* step;
        results[t] = outputReal[i+shift_padding]/(N*somHann);
#ifdef DEBUG
        if (isnan(results[t])) {
            qDebug()<<"window filter"<<t<< " isnan";
        }
#endif
    }
    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_hann);
    fftw_destroy_plan(plan_output);

    fftw_free(inputComplex);

    delete [] inputReal;
    delete [] outputReal;
    delete [] hannReal;

    fftw_cleanup();
    return results;
}



#endif
