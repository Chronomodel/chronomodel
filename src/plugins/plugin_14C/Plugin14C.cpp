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

#include "Plugin14C.h"
#if USE_PLUGIN_14C

#include "QtUtilities.h"
#include "Plugin14CForm.h"
#include "Plugin14CRefView.h"
#include "Plugin14CSettingsView.h"
#include "Generator.h"

#include <cstdlib>
#include <QJsonObject>
#include <QtWidgets>


Plugin14C::Plugin14C()
{
    mColor = QColor(47, 46, 68);
    mRefGraph = nullptr;
    loadRefDatas();
}

Plugin14C::~Plugin14C()
{
    if (mRefGraph)
        delete mRefGraph;
}

// Likelihood
QPair<long double, long double> Plugin14C::getLikelihoodArg(const double t, const QJsonObject &data)
{
    double age = data.value(DATE_14C_AGE_STR).toDouble();
    double error = data.value(DATE_14C_ERROR_STR).toDouble();
    double delta_r = data.value(DATE_14C_DELTA_R_STR).toDouble();
    double delta_r_error = data.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();

    // Apply reservoir effect
    age = (age - delta_r);
    error = sqrt(error * error + delta_r_error * delta_r_error);

    long double refValue = (long double) getRefValueAt(data, t);
    long double refError = (long double) getRefErrorAt(data, t);

    long double variance = refError * refError + error * error;
    long double exponent = -0.5l * powl((long double)(age - refValue), 2.l) / variance;
    return qMakePair(variance, exponent);
}

long double Plugin14C::getLikelihood(const double t, const QJsonObject &data)
{
    QPair<long double, long double > result = getLikelihoodArg(t, data);
    long double back = expl(result.second) / sqrtl(result.first) ;
    return back;
}

// Properties
QString Plugin14C::getName() const
{
    return QString("14C");
}

QIcon Plugin14C::getIcon() const
{
    return QIcon(":/14C_w.png");
}

bool Plugin14C::doesCalibration() const
{
    return true;
}

bool Plugin14C::wiggleAllowed() const
{
    return true;
}

MHVariable::SamplerProposal Plugin14C::getDataMethod() const
{
    return MHVariable::eInversion;
}

QList<MHVariable::SamplerProposal> Plugin14C::allowedDataMethods() const
{
    QList<MHVariable::SamplerProposal> methods;
    methods.append(MHVariable::eMHPrior);
    methods.append(MHVariable::eInversion);
    methods.append(MHVariable::eMHAdaptGauss);
    return methods;
}

QString Plugin14C::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QLocale locale = QLocale();
    QString result;

    const QJsonObject data = date->mData;

    const double age = data.value(DATE_14C_AGE_STR).toDouble();
    const double error = data.value(DATE_14C_ERROR_STR).toDouble();
    const double delta_r = data.value(DATE_14C_DELTA_R_STR).toDouble();
    const double delta_r_error = data.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();
    const QString ref_curve = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();

    result += QObject::tr("Age = %1  ± %2").arg(locale.toString(age), locale.toString(error));

    if (delta_r != 0. || delta_r_error != 0.)
        result += ": " + QObject::tr("ΔR = %1 ± %2").arg(locale.toString(delta_r), locale.toString(delta_r_error));


    if (mRefCurves.contains(ref_curve) && !mRefCurves.value(ref_curve).mDataMean.isEmpty())
        result += ": " + tr("Ref. curve = %1").arg(ref_curve);
    else
        result += ": " + tr("ERROR -> Ref. curve : %1").arg(ref_curve);


    return result;
}

QString Plugin14C::getDateRefCurveName(const Date* date)
{
    Q_ASSERT(date);
    const QJsonObject data = date->mData;

    return  data.value(DATE_14C_REF_CURVE_STR).toString().toLower();
}

// CSV
QStringList Plugin14C::csvColumns() const
{
    QStringList cols;
    cols << "Data Name" << "Age" << "Error (sd)" << "Ref. curve" << "ΔR" << "ΔR Error";
    return cols;
}

qsizetype Plugin14C::csvMinColumns() const
{
    return csvColumns().count() - 2;
}

QJsonObject Plugin14C::fromCSV(const QStringList& list, const QLocale &csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        double error = csvLocale.toDouble(list.at(2));
        if (error == 0.)
            return json;

        json.insert(DATE_14C_AGE_STR, csvLocale.toDouble(list.at(1)));
        json.insert(DATE_14C_ERROR_STR, error);
        json.insert(DATE_14C_REF_CURVE_STR, list.at(3).toLower());

        // These columns are nor mandatory in the CSV file so check if they exist :
        json.insert(DATE_14C_DELTA_R_STR, (list.size() > 4) ? csvLocale.toDouble(list.at(4)) : 0.f);
        json.insert(DATE_14C_DELTA_R_ERROR_STR, (list.size() > 5) ? csvLocale.toDouble(list.at(5)) : 0.f);

    }
    return json;
}

QStringList Plugin14C::toCSV(const QJsonObject& data, const QLocale& csvLocale) const
{
    QStringList list;
    list << csvLocale.toString(data.value(DATE_14C_AGE_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_14C_ERROR_STR).toDouble());
    list << data.value(DATE_14C_REF_CURVE_STR).toString();
    list << csvLocale.toString(data.value(DATE_14C_DELTA_R_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_14C_DELTA_R_ERROR_STR).toDouble());
    return list;
}

// ------------------------------------------------------------------

// Ref curves (files)
QString Plugin14C::getRefExt() const
{
    return "14c";
}

QString Plugin14C::getRefsPath() const
{
    return AppPluginLibrary() + "/Calib/14C";
}

RefCurve Plugin14C::loadRefFile(QFileInfo refFile)
{
    RefCurve curve;
    curve.mName = refFile.fileName().toLower();

    // Default time in BP, data in 14C age, data column is 1
    bool timeInBP (true);
    //bool timeInBC (false);
    bool dataInF14C (false);
    bool dataInAge (true);
    int ageColumn (1);
    int F14Column (0);
    char dataSep (',');
    
    QFile file(refFile.absoluteFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QLocale locale = QLocale(QLocale::English);
        QTextStream stream(&file);
        bool firstLine = true;
        double prev_t = -INFINITY;
        double delta_t = INFINITY;

        while (!stream.atEnd()) {
            QString line = stream.readLine();

            if (isComment(line)) {
                
                
                if (line.contains("BC", Qt::CaseInsensitive) || line.contains("BP", Qt::CaseInsensitive) || line.contains("Year", Qt::CaseInsensitive) ||  line.contains("age", Qt::CaseInsensitive) || line.contains("F14", Qt::CaseInsensitive) ) {
                    if (line.contains(',')) {
                        dataSep = ',';
                    } else {
                       dataSep = char(9);
                    }
                }
                QStringList values = line.split(dataSep);
                
                if (values.at(0).contains("BP", Qt::CaseInsensitive) ) {
                    timeInBP = true;
                    
                } else if (values.at(0).contains("BC", Qt::CaseInsensitive)  || values.at(0).contains("year", Qt::CaseInsensitive)) {
                    timeInBP = false;
                }
                
                if (line.contains("F14", Qt::CaseInsensitive)) {
                    dataInF14C = true;
                    dataInAge = false;
         
                    for (int i = 0; i < values.size(); ++i) {
                        if ( values[i].contains("F14", Qt::CaseInsensitive)) {
                            F14Column = i;
                            continue;
                        }
                    }
                } else if (line.contains("age", Qt::CaseInsensitive)) {
                    dataInF14C = false;
                    dataInAge = true;
  
                    for (int i = 0; i < values.size(); ++i) {
                        if ( values[i].contains("age", Qt::CaseInsensitive)) {
                            ageColumn = i;
                            continue;
                        }
                    }
               }
             
                
               
            }
            
            int dataColumn(1);
            if (dataInF14C)
                dataColumn = F14Column;
            else if (dataInAge)
                dataColumn = ageColumn;
       
            
            if (!isComment(line)) {
                QStringList values = line.split(dataSep);

                if (values.size() >= 3) {
                    bool ok = true;

                    double t = locale.toDouble(values.at(0), &ok);
                    if (!ok)
                        continue;
                    if (timeInBP)
                        t = 1950 - t;

                    delta_t = std::min(delta_t, abs(t-prev_t));
                    // must convert all in 14C
                    double g = locale.toDouble(values.at(dataColumn), &ok);
                    if(!ok)
                        continue;
                    
                    double e = locale.toDouble(values.at(dataColumn+1), &ok);
                    if (!ok)
                        continue;
                    if (dataInF14C) {// data is BP Age
                        e = errF14CtoErrCRA(e, g);
                        g = F14CtoCRA(g);
                    }

                    double gSup = g + 1.96 * e;
                    if(!ok)
                        continue;
                    double gInf = g - 1.96 * e;
                    if (!ok)
                        continue;

                    curve.mDataMean[t] = g;
                    curve.mDataError[t] = e;
                    curve.mDataSup[t] = gSup;
                    curve.mDataInf[t] = gInf;

                    if (firstLine) {
                        curve.mDataMeanMin = g;
                        curve.mDataMeanMax = g;

                        curve.mDataErrorMin = e;
                        curve.mDataErrorMax = e;

                        curve.mDataSupMin = gSup;
                        curve.mDataSupMax = gSup;

                        curve.mDataInfMin = gInf;
                        curve.mDataInfMax = gInf;

                    } else {
                        curve.mDataMeanMin = qMin(curve.mDataMeanMin, g);
                        curve.mDataMeanMax = qMax(curve.mDataMeanMax, g);

                        curve.mDataErrorMin = qMin(curve.mDataErrorMin, e);
                        curve.mDataErrorMax = qMax(curve.mDataErrorMax, e);

                        curve.mDataSupMin = qMin(curve.mDataSupMin, gSup);
                        curve.mDataSupMax = qMax(curve.mDataSupMax, gSup);

                        curve.mDataInfMin = qMin(curve.mDataInfMin, gInf);
                        curve.mDataInfMax = qMax(curve.mDataInfMax, gInf);
                    }
                    firstLine = false;
                    prev_t = t;
                }
            }
        }
        file.close();

        curve.mMinStep = delta_t;
        // invalid file ?
        if (!curve.mDataMean.isEmpty()) {
            curve.mTmin = curve.mDataMean.firstKey();
            curve.mTmax = curve.mDataMean.lastKey();
        }
    }
    return curve;
}

// References Values & Errors
double Plugin14C::getRefValueAt(const QJsonObject& data, const double& t)
{
    const QString curveName = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();
    return getRefCurveValueAt(curveName, t);
}

double Plugin14C::getRefErrorAt(const QJsonObject& data, const double& t)
{
    const QString curveName = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();
    return getRefCurveErrorAt(curveName, t);
}

QPair<double,double> Plugin14C::getTminTmaxRefsCurve(const QJsonObject &data) const
{
    double tmin (-INFINITY);
    double tmax (INFINITY);
    const QString ref_curve = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();

    if (mRefCurves.contains(ref_curve)  && !mRefCurves[ref_curve].mDataMean.isEmpty()) {
       tmin = mRefCurves.value(ref_curve).mTmin;
       tmax = mRefCurves.value(ref_curve).mTmax;
    }
    return QPair<double, double>(tmin, tmax);
}

double Plugin14C::getMinStepRefsCurve(const QJsonObject &data) const
{
    const QString ref_curve = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();

    if (mRefCurves.contains(ref_curve)  && !mRefCurves[ref_curve].mDataMean.isEmpty()) {
       return mRefCurves.value(ref_curve).mMinStep;

    } else {
       return INFINITY;
    }
}

//Settings / Input Form / RefView
GraphViewRefAbstract* Plugin14C::getGraphViewRef()
{
    mRefGraph = new Plugin14CRefView();
    return mRefGraph;
}
void Plugin14C::deleteGraphViewRef(GraphViewRefAbstract* graph)  {

    if (graph)
        delete static_cast<Plugin14CRefView*>(graph);

    graph = nullptr;
    mRefGraph = nullptr;
}
PluginSettingsViewAbstract* Plugin14C::getSettingsView()
{
    return new Plugin14CSettingsView(this);
}

PluginFormAbstract* Plugin14C::getForm()
{
    Plugin14CForm* form = new Plugin14CForm(this);
    return form;
}

// Convert old project versions
QJsonObject Plugin14C::checkValuesCompatibility(const QJsonObject& values)
{
    QJsonObject result = values;

    if (result.find(DATE_14C_DELTA_R_STR) == result.end())
        result[DATE_14C_DELTA_R_STR] = 0.0;

    if (result.find(DATE_14C_DELTA_R_ERROR_STR) == result.end())
        result[DATE_14C_DELTA_R_ERROR_STR] = 0.0;

    // must be a double
    result[DATE_14C_DELTA_R_STR] = result.value(DATE_14C_DELTA_R_STR).toDouble() ;
    result[DATE_14C_DELTA_R_ERROR_STR] = result.value(DATE_14C_DELTA_R_ERROR_STR).toDouble() ;

    // Force curve name to lower case :
    result[DATE_14C_REF_CURVE_STR] = result.value(DATE_14C_REF_CURVE_STR).toString().toLower();

    return result;
}

// Date Validity
bool Plugin14C::isDateValid(const QJsonObject& data, const StudyPeriodSettings& settings)
{
    const QString ref_curve = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();
    bool valid = false;
    if (!mRefCurves.contains(ref_curve)) {
        qDebug()<<"in Plugin14C::isDateValid() unkowned curve"<<ref_curve;
        valid = false;
    } else {
        // controle valid solution (double)likelihood>0
        // remember likelihood type is long double
        const RefCurve& curve = mRefCurves.value(ref_curve);
        valid = false;
        double age = data.value(DATE_14C_AGE_STR).toDouble();
        const double delta_r = data.value(DATE_14C_DELTA_R_STR).toDouble();

        // Apply reservoir effect
        age = (age - delta_r);

        if (age>curve.mDataInfMin && age < curve.mDataSupMax)
            valid = true;

        else {
            double t = curve.mTmin;
            long double repartition (0.);
            long double v (0.);
            long double lastV (0.);
            while (valid==false && t<=curve.mTmax) {
                v = (double)getLikelihood(t,data);
                // we have to check this calculs
                //because the repartition can be smaller than the calibration
                if (lastV>0 && v>0)
                    repartition += (long double) settings.mStep * (lastV + v) / 2.;

                lastV = v;

                valid = ( (double)repartition > 0);
                t +=settings.mStep;
            }
        }
    }
    return valid;
}

// Grouped Actions
QList<QHash<QString, QVariant>> Plugin14C::getGroupedActions()
{
    QList<QHash<QString, QVariant>> result;

    QHash<QString, QVariant> groupedAction;
    groupedAction.insert("pluginId", getId());
    groupedAction.insert("title", tr("Selected Events with 14C: Change Ref. Curves"));
    groupedAction.insert("label", tr("Change 14C Ref. Curves for all 14C data in selected events :"));
    groupedAction.insert("inputType", "combo");
    groupedAction.insert("items", getRefsNames());
    groupedAction.insert("valueKey", DATE_14C_REF_CURVE_STR);

    result.append(groupedAction);
    return result;
}

// Combine / Split
bool Plugin14C::areDatesMergeable(const QJsonArray& dates)
{
    QString refCurve;
    for (int i=0; i<dates.size(); ++i) {
        QJsonObject date = dates.at(i).toObject();
        QJsonObject data = date.value(STATE_DATE_DATA).toObject();
        QString curve = data.value(DATE_14C_REF_CURVE_STR).toString().toLower();

        if (refCurve.isEmpty())
            refCurve = curve;

        else if (refCurve != curve)
            return false;
    }
    return true;
}

/**
 * @brief Combine several 14C Age
 **/
QJsonObject Plugin14C::mergeDates(const QJsonArray& dates)
{

    QJsonObject result;
    if (dates.size() > 1) {

        QStringList names;
        QJsonObject mergedData;

        // inherits the first data propeties as plug-in and method...

        // test wiggle existence and create the name
        bool withWiggle (false);
        for (int i = 0; i<dates.size(); ++i) {
            QJsonObject date = dates.at(i).toObject();
            const QJsonObject dateData = date.value(STATE_DATE_DATA).toObject();
            withWiggle = withWiggle || (dateData.value(STATE_DATE_DELTA_TYPE).toInt() != Date::eDeltaNone);

            names.append(dates.at(i).toObject().value(STATE_NAME).toString());
        }

        result = dates.at(0).toObject();
        result[STATE_NAME] = names.join(" | ");
        result[STATE_DATE_UUID] = QString::fromStdString( Generator::UUID());
        result[STATE_DATE_SUB_DATES] = dates;


        if (withWiggle) {

            mergedData[DATE_14C_AGE_STR] = 1000.;
            mergedData[DATE_14C_ERROR_STR] = 100.;
            mergedData[DATE_14C_DELTA_R_STR] = 0.;
            mergedData[DATE_14C_DELTA_R_ERROR_STR] = 0.;
            mergedData[DATE_14C_REF_CURVE_STR] = "" ;

            result[STATE_DATE_DATA] = mergedData;
            result[STATE_DATE_ORIGIN] = Date::eCombination;

            result[STATE_DATE_VALID] = true;
            result[STATE_DATE_DELTA_TYPE] = Date::eDeltaNone;


        } else {
            // Verify all dates have the same ref curve :
            const QJsonObject firstDate = dates.at(0).toObject();
            const  QJsonObject firstDateData = firstDate.value(STATE_DATE_DATA).toObject();
            QString firstCurve = firstDateData.value(DATE_14C_REF_CURVE_STR).toString().toLower();

            for (int i(1); i<dates.size(); ++i) {
                QJsonObject date = dates.at(i).toObject();
                const QJsonObject dateData = date.value(STATE_DATE_DATA).toObject();
                const QString curve = dateData.value(DATE_14C_REF_CURVE_STR).toString().toLower();

                if (firstCurve != curve) {
                    result["error"] = tr("All combined data must use the same reference curve !");
                    return result;
                }
            }

            //double sum_vi = 0.;
            double sum_mi_vi = 0.;
            double sum_1_vi = 0.;

            for (int i = 0; i<dates.size(); ++i) {
                const QJsonObject date = dates.at(i).toObject();
                const QJsonObject data = date.value(STATE_DATE_DATA).toObject();

                const double a = data.value(DATE_14C_AGE_STR).toDouble();
                const double e = data.value(DATE_14C_ERROR_STR).toDouble();
                const double r = data.value(DATE_14C_DELTA_R_STR).toDouble();
                const double re = data.value(DATE_14C_DELTA_R_ERROR_STR).toDouble();

                // Reservoir effet
                const double m = a - r;
                const double v = e * e + re * re;

                //sum_vi += v;
                sum_mi_vi += m/v;
                sum_1_vi += 1/v;
            }

            mergedData[DATE_14C_AGE_STR] = sum_mi_vi / sum_1_vi;
            mergedData[DATE_14C_ERROR_STR] = sqrt(1 / sum_1_vi);
            mergedData[DATE_14C_DELTA_R_STR] = 0.;
            mergedData[DATE_14C_DELTA_R_ERROR_STR] = 0.;
            mergedData[DATE_14C_REF_CURVE_STR] = firstCurve ;

            result[STATE_DATE_ORIGIN] = Date::eSingleDate;
            result[STATE_DATE_DATA] = mergedData;

      }
    } else
        result["error"] = tr("Combine needs at least 2 data !");

    return result;

}


#endif
