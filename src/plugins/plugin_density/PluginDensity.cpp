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

#if USE_PLUGIN_DENSITY
#include "PluginDensity.h"

#include "QtUtilities.h"
#include "PluginDensityForm.h"
#include "PluginDensityRefView.h"
#include "PluginDensitySettingsView.h"
#include "Generator.h"

#include <QJsonObject>
#include <QtWidgets>

#include <cstdlib>
#include <stdio.h>

PluginDensity::PluginDensity()
{
    mColor = QColor(217, 37, 37);
    loadRefDatas();
}

PluginDensity::~PluginDensity()
{
    if (mRefGraph)
        delete mRefGraph;
}

// Likelihood
long double PluginDensity::getLikelihood(const double t, const QJsonObject &data)
{
    const QString curveName = data.value(DATE_DENSITY_CURVE_STR).toString().toLower();
    if (mRefCurves.constFind(curveName) != mRefCurves.constEnd()) {
        const RefCurve &curve = mRefCurves.value(curveName);
        return curve.interpolate_mean(t);
    } else {
        return 0.;
    }

}

QPair<long double, long double> PluginDensity::getLikelihoodArg(const double t, const QJsonObject &data)
{
    const double value = getLikelihood(t, data);
    return qMakePair(1., log(value));
}
    
// Properties
QString PluginDensity::getName() const
{
    return QString("Density");
}

QIcon PluginDensity::getIcon() const
{
    return QIcon(":/density_w.png");
}

bool PluginDensity::doesCalibration() const
{
    return true;
}

bool PluginDensity::wiggleAllowed() const
{
    return true;
}

MHVariable::SamplerProposal PluginDensity::getDataMethod() const
{
    return MHVariable::eInversion;
}

QList<MHVariable::SamplerProposal> PluginDensity::allowedDataMethods() const
{
    QList<MHVariable::SamplerProposal> methods;
    methods.append(MHVariable::eMHPrior);
    methods.append(MHVariable::eInversion);
    methods.append(MHVariable::eMHAdaptGauss);
    return methods;
}

QString PluginDensity::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QString result;

    if (date->mOrigin == Date::eSingleDate) {

        const QJsonObject &data = date->mData;

        const QString ref_curve = data[DATE_DENSITY_CURVE_STR].toString().toLower();
        if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
            result += " " + tr("Ref. curve : %1").arg(ref_curve);
        else
            result += " " + tr("ERROR ->  Ref. curve : %1").arg(ref_curve);


    } else {
        result = "Combine (";
        QStringList datesDesc;
        for (auto&& d: date->mSubDates) {
            Date subDate (d.toObject() );
            datesDesc.append(getDateDesc(&subDate));
        }
        result += datesDesc.join(" | ") + " )" ;
        
    }
    return result;

}

QString PluginDensity::getDateRefCurveName(const Date* date)
{
    Q_ASSERT(date);
    const QJsonObject &data = date->mData;

    return data[DATE_DENSITY_CURVE_STR].toString().toLower();

}

// CSV
QString PluginDensity::csvHelp() const
{
    return "CSV file name";
}

QStringList PluginDensity::csvColumns() const
{
    QStringList cols;
    cols << "Data Name"  << "File name" ;
    return cols;
}

qsizetype PluginDensity::csvMinColumns() const{
    return csvColumns().count() - 2;
}

/**
 * @brief PluginDensity::fromCSV
 * @param list
 * @param csvLocale
 * @return
 * @example
 * No name; Density; New Data; gate [20 :: 40].csv;none
 * Density; Density; lognorm; lognorm(3 :: 2) _0to20 50pts.csv;none
 */
QJsonObject PluginDensity::fromCSV(const QStringList &list, const QLocale &csvLocale)
{
    (void) csvLocale;
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        json.insert(DATE_DENSITY_CURVE_STR, list.at(1));
    }

    return json;
}

QStringList PluginDensity::toCSV(const QJsonObject &data, const QLocale &csvLocale) const
{
    (void) csvLocale;
    QStringList list;
    list << data.value(DATE_DENSITY_CURVE_STR).toString();

    return list;
}

// ------------------------------------------------------------------

// Density Curves (files)
QString PluginDensity::getRefExt() const
{
    return "csv";
}

QString PluginDensity::getRefsPath() const
{
    qDebug()<< "[PluginDensity::getRefsPath()] " << AppPluginLibrary() + "/Calib/Density";
    return AppPluginLibrary() + "/Calib/Density";
}

/**
 * @brief PluginDENSITY::loadRefFile the reference curve must be in english CSV mode_t
 * it's mean decimal separator is dot and value separator is coma
 * @param refFile
 * @return
 *
 *
 * tmin
 * tmax
 * values in column, the step is define with the number of values step = (tmax-tmin)/nbre + 1
 */
RefCurve PluginDensity::loadRefFile(QFileInfo refFile)
{
    RefCurve curve;
    curve.mName = refFile.fileName().toLower();

    std::vector<double> data_curve;

    FILE * pFile;
    pFile = fopen (refFile.absoluteFilePath().toLocal8Bit(),"r");

    if (pFile != nullptr) {

        QLocale locale = QLocale(QLocale::English);

        char ch = ' ';
        while (ch != EOF)  {

            QString line;
            // this code allows to open the MacOS file (QChar::LineFeed) and WindowsOS file (QChar::CarriageReturn)
            do {
                ch = fgetc (pFile);
                if (ch == QChar::LineFeed || ch == QChar::CarriageReturn)
                    break;
                if (ch != EOF)
                    line.append(ch);

            } while (ch != EOF);

            if (!isComment(line)) {
                QStringList values = line.split(",");
                //if (values.size() > 2) {
                    bool ok = true;
                    const double v = locale.toDouble(values.at(0), &ok);
                    if (!ok)
                        continue;


                    data_curve.push_back(v);

              //  }
            }
        }
        fclose(pFile);

        // insert values in curve

        double tmin = data_curve.at(0);
        double tmax = data_curve.at(1);
        if (tmin>tmax)
            std::swap(tmin, tmax);

        curve.mTmin = tmin;
        curve.mTmax = tmax;

        const double step = (tmax-tmin)/(data_curve.size()-3);

        for (size_t i = 2; i<data_curve.size(); i++) {
            const double t = step*(i-2) + tmin;
            curve.mDataMean[t] = data_curve.at(i);
        }
        curve.mMinStep = step;

        qDebug()<<"[pluginDensity::loadRefFile]:: normalization";
        curve.mDataMean = equal_areas(curve.mDataMean, 1.);
    }

    curve.mDataMeanMin = 0.;
    curve.mDataMeanMax = 0.;

    curve.mDataErrorMin = 0.;
    curve.mDataErrorMax = 0.;

    curve.mDataSupMin = 0.;
    curve.mDataSupMax = 0.;

    curve.mDataInfMin = 0.;
    curve.mDataInfMax = 0.;

    return curve;
}

double PluginDensity::getMinStepRefsCurve(const QJsonObject &data) const
{
    const QString ref_curve = data.value(DATE_DENSITY_CURVE_STR).toString().toLower();

    if (mRefCurves.contains(ref_curve)  && !mRefCurves[ref_curve].mDataMean.isEmpty()) {
        return mRefCurves.value(ref_curve).mMinStep;

    } else {
        return INFINITY;
    }
}
// Reference Values & Errors
double PluginDensity::getRefValueAt(const QJsonObject &data, const double &t)
{
    (void)  data;
    (void) t;
    return 0.;
}

double PluginDensity::getRefErrorAt(const QJsonObject &data, const double &t, const QString mode)
{
    (void)  data;
    (void) t;
    (void) mode;
    return 0.;
}

QPair<double, double> PluginDensity::getTminTmaxRefsCurve(const QJsonObject &data) const
{
    double tmin = 0.;
    double tmax = 0.;


    const QString ref_curve = data.value(DATE_DENSITY_CURVE_STR).toString().toLower();
#ifdef DEBUG
    if (mRefCurves.contains(ref_curve) && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
#endif
        tmin = mRefCurves.value(ref_curve).mTmin;
        tmax = mRefCurves.value(ref_curve).mTmax;
#ifdef DEBUG

    } else
            qDebug() << "[PluginDENSITY::getTminTmaxRefsCurve] no ref curve";
#endif

    return QPair<double,double>(tmin, tmax);
}

// ------------------------------------------------------------------

// Settings / Input Form / RefView
GraphViewRefAbstract* PluginDensity::getGraphViewRef()
{
   mRefGraph = new PluginDensityRefView();

   return mRefGraph;
}
void PluginDensity::deleteGraphViewRef(GraphViewRefAbstract* graph)
{
    if (graph)
        delete static_cast<PluginDensityRefView*>(graph);

    graph = nullptr;
    mRefGraph = nullptr;
}

PluginSettingsViewAbstract* PluginDensity::getSettingsView()
{
    return new PluginDensitySettingsView(this);
}

PluginFormAbstract* PluginDensity::getForm()
{
    PluginDensityForm* form = new PluginDensityForm(this);
    return form;
}

// ------------------------------------------------------------------

// Convert old project versions
QJsonObject PluginDensity::checkValuesCompatibility(const QJsonObject &values)
{
    QJsonObject result = values;
    result[DATE_DENSITY_CURVE_STR] = result.value(DATE_DENSITY_CURVE_STR).toString().toLower();

    return result;
}

// Date Validity
bool PluginDensity::isDateValid(const QJsonObject &, const StudyPeriodSettings &)
{
    return true;
}

// Combine / Split
bool PluginDensity::areDatesMergeable(const QJsonArray& )
{
    return true;
}

/**
 * @brief Combine several CSV densities
 **/
QJsonObject PluginDensity::mergeDates(const QJsonArray &dates)
{
    QJsonObject result;
    const double k = 10.;
    if (dates.size() > 1) {

        QStringList names;

        // ----------
        double min (-INFINITY);
        double max (+INFINITY);

        // il faut ajouter le décallage induit par le wiggle si il existe dWiggleMin, dWiggleMax
        double dWiggleMin = 0.;
        double dWiggleMax = 0.;
        for ( auto && d : dates ) {
            names.append(d.toObject().value(STATE_NAME).toString());
            QPair<double, double> tminTmax = getTminTmaxRefsCurve(d.toObject().value(STATE_DATE_DATA).toObject());
            switch (d.toObject().value(STATE_DATE_DELTA_TYPE).toInt()) {
            case Date::eDeltaNone:
                dWiggleMin = 0.;
                dWiggleMax = 0.;
                break;
            case Date::eDeltaFixed:
                dWiggleMin = d.toObject().value(STATE_DATE_DELTA_FIXED).toDouble();
                dWiggleMax = d.toObject().value(STATE_DATE_DELTA_FIXED).toDouble();
                break;
            case Date::eDeltaRange:
                dWiggleMin = d.toObject().value(STATE_DATE_DELTA_MIN).toDouble();
                dWiggleMax = d.toObject().value(STATE_DATE_DELTA_MAX).toDouble();
                break;
            case Date::eDeltaGaussian:
                dWiggleMin = d.toObject().value(STATE_DATE_DELTA_AVERAGE).toDouble() - d.toObject().value(STATE_DATE_DELTA_ERROR).toDouble()*k;
                dWiggleMax = d.toObject().value(STATE_DATE_DELTA_AVERAGE).toDouble() + d.toObject().value(STATE_DATE_DELTA_ERROR).toDouble()*k;
                break;

            }

            min = std::max(tminTmax.first + dWiggleMin, min);
            max = std::min(tminTmax.second + dWiggleMax, max);
        }

        if ((max - min) <= 1.) {
            result["error"] = tr("Combine is not possible, not enough coincident densities; numeric issue");
            return result;
        }


        // inherits the first data propeties as plug-in and method...
        result = dates.at(0).toObject();
        result[STATE_NAME] = names.join(" | ");
        result[STATE_DATE_UUID] = QString::fromStdString( Generator::UUID());
        result[STATE_DATE_SUB_DATES] = dates;


        QJsonObject mergedData;

        mergedData[DATE_DENSITY_CURVE_STR] = DATE_DENSITY_CURVE_STR;


        result[STATE_DATE_DATA] = mergedData;
        result[STATE_DATE_ORIGIN] = Date::eCombination;

        result[STATE_DATE_VALID] = true;
        result[STATE_DATE_DELTA_TYPE] = Date::eDeltaNone;



    } else
        result["error"] = tr("Combine needs at least 2 data !");

    return result;

}
#endif
