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

#if USE_PLUGIN_GAUSS
#include "PluginGauss.h"

#include "QtUtilities.h"
#include "PluginGaussForm.h"
#include "PluginGaussRefView.h"
#include "PluginGaussSettingsView.h"
#include "Generator.h"

#include <QJsonObject>
#include <QtWidgets>

#include <cstdlib>
#include <stdio.h>

const unsigned k_sigma = 5;

PluginGauss::PluginGauss()
{
    mColor = QColor(217, 37, 37);
    loadRefDatas();
}

PluginGauss::~PluginGauss()
{
    if (mRefGraph)
        delete mRefGraph;
}

// Likelihood
long double PluginGauss::getLikelihood(const double t, const QJsonObject &data)
{
    const QPair<long double, long double > result = getLikelihoodArg(t, data);

    return expl(result.second) / sqrt(result.first);
}

QPair<long double, long double> PluginGauss::getLikelihoodArg(const double t, const QJsonObject &data)
{
    // inherits the first data propeties as plug-in and method...

    const double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
    const double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
    const QString mode = data.value(DATE_GAUSS_MODE_STR).toString();

    double refError = 0.;

    double refValue;

    if (mode == DATE_GAUSS_MODE_CURVE) {
        const QString &ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        refValue = getRefCurveValueAt(ref_curve, t);
        refError = getRefCurveErrorAt(ref_curve, t);

    } else {
        refValue = getRefValueAt(data, t);
    }

    const long double variance = static_cast<long double>(refError * refError + error * error);
    const long double exponent = -0.5l * powl(static_cast<long double>(age - refValue), 2.l) / variance;

    return qMakePair(variance, exponent);
}
    
// Properties
QString PluginGauss::getName() const
{
    return QString("Gauss");
}

QIcon PluginGauss::getIcon() const
{
    return QIcon(":/gauss_w.png");
}

bool PluginGauss::doesCalibration() const
{
    return true;
}

bool PluginGauss::wiggleAllowed() const
{
    return true;
}

MHVariable::SamplerProposal PluginGauss::getDataMethod() const
{
    return MHVariable::eMHPrior;
}

QList<MHVariable::SamplerProposal> PluginGauss::allowedDataMethods() const
{
    QList<MHVariable::SamplerProposal> methods;
    methods.append(MHVariable::eMHPrior);
    methods.append(MHVariable::eInversion);
    methods.append(MHVariable::eMHAdaptGauss);
    return methods;
}

QString PluginGauss::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QString result;
    
    if (date->mOrigin == Date::eSingleDate) {

        const QJsonObject &data = date->mData;

        const QString mode = data[DATE_GAUSS_MODE_STR].toString();

        result += QObject::tr("Mean = %1  ±  %2").arg(QLocale().toString(data[DATE_GAUSS_AGE_STR].toDouble()), QLocale().toString(data[DATE_GAUSS_ERROR_STR].toDouble()));

        if (mode == DATE_GAUSS_MODE_NONE) {
            result += " (No calibration)";

        } else if (mode == DATE_GAUSS_MODE_EQ) {
            const double a = data[DATE_GAUSS_A_STR].toDouble();
            const double b = data[DATE_GAUSS_B_STR].toDouble();
            const double c = data[DATE_GAUSS_C_STR].toDouble();

            QString aStr;
            if (a != 0.) {
                if (a == -1.)
                    aStr += "-";
                else if (a != 1.)
                    aStr += QLocale().toString(a);

                aStr += "t²";
            }
            QString bStr;
            if (b != 0.) {
                if (b == -1.)
                    bStr += "-";
                else if (b != 1.)
                    bStr += QLocale().toString(b);

                bStr += "t";
            }
            QString cStr;
            if (c != 0.)
                cStr += QLocale().toString(c);

            QString eq = aStr;
            if (!eq.isEmpty() && !bStr.isEmpty())
                eq += " + ";

            eq += bStr;
            if (!eq.isEmpty() && !cStr.isEmpty())
                eq += " + ";

            eq += cStr;

            result += " " + QObject::tr("Ref. curve") + " : g(t) = " + eq;
        }
        else if (mode == DATE_GAUSS_MODE_CURVE) {
            const QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString().toLower();
            if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
                result += " " + tr("Ref. curve : %1").arg(ref_curve);
            else
                result += " " + tr("ERROR ->  Ref. curve : %1").arg(ref_curve);

        }
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

QString PluginGauss::getDateRefCurveName(const Date* date)
{
    Q_ASSERT(date);
    const QJsonObject &data = date->mData;

    const QString mode = data[DATE_GAUSS_MODE_STR].toString();

    if (mode == DATE_GAUSS_MODE_CURVE)
        return data[DATE_GAUSS_CURVE_STR].toString().toLower();

    else
        return QString();

}

// CSV
QString PluginGauss::csvHelp() const
{
    return "Calibration : g(t) = at^2 + bt + c\r";
}

QStringList PluginGauss::csvColumns() const
{
    QStringList cols;
    cols << "Data Name" << "Mean" << "Error (sd)" << "Ref. Curve" << "a" << "b" << "c";
    return cols;
}

qsizetype PluginGauss::csvMinColumns() const{
    return csvColumns().count() - 3;
}

QJsonObject PluginGauss::fromCSV(const QStringList& list, const QLocale& csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        json.insert(DATE_GAUSS_AGE_STR, csvLocale.toDouble(list.at(1)));
        const double error = csvLocale.toDouble(list.at(2));
        if (error == 0.)
            return QJsonObject();
        json.insert(DATE_GAUSS_ERROR_STR, error);

        if ( (list.at(3).toLower() == "equation") && (list.size() >= csvMinColumns() + 3) ) {
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_EQ));
            const double a = csvLocale.toDouble(list.at(4));
            const double b = csvLocale.toDouble(list.at(5));
            const double c = csvLocale.toDouble(list.at(6));
            if ( (a==0.) && (b== 0.))  {// this solution is forbiden
                return QJsonObject();

            } else {
                json.insert(DATE_GAUSS_A_STR, a);
                json.insert(DATE_GAUSS_B_STR, b);
                json.insert(DATE_GAUSS_C_STR, c);
            }

        } else if (list.at(3).toLower() == "none" || list.at(3) == "") {
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_NONE));

        } else {
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_CURVE));
            json.insert(DATE_GAUSS_CURVE_STR, list.at(3));
        }
    }
    return json;
}

QStringList PluginGauss::toCSV(const QJsonObject& data, const QLocale& csvLocale) const
{
    QStringList list;
    list << csvLocale.toString(data.value(DATE_GAUSS_AGE_STR).toDouble());
    list << csvLocale.toString(data.value(DATE_GAUSS_ERROR_STR).toDouble());

    if (data.value(DATE_GAUSS_MODE_STR).toString() == DATE_GAUSS_MODE_NONE){
        list << "none";

    } else if (data[DATE_GAUSS_MODE_STR].toString() == DATE_GAUSS_MODE_EQ){
        list << "equation";
        list << csvLocale.toString(data.value(DATE_GAUSS_A_STR).toDouble());
        list << csvLocale.toString(data.value(DATE_GAUSS_B_STR).toDouble());
        list << csvLocale.toString(data.value(DATE_GAUSS_C_STR).toDouble());

    }  else if (data.value(DATE_GAUSS_MODE_STR).toString() == DATE_GAUSS_MODE_CURVE)
        list << data.value(DATE_GAUSS_CURVE_STR).toString();

    return list;
}

// ------------------------------------------------------------------

// Reference Curves (files)
QString PluginGauss::getRefExt() const
{
    return "csv";
}

QString PluginGauss::getRefsPath() const
{
    qDebug()<< "[PluginGauss::getRefsPath()] " << AppPluginLibrary() + "/Calib/Gauss";
    return AppPluginLibrary() + "/Calib/Gauss";
}

/**
 * @brief PluginGauss::loadRefFile the reference curve must be in english CSV mode_t
 * it's mean decimal separator is dot and value separator is coma
 * example file.csv
 * -1500,10,5
 * -1000,1500,10
 * -0,1500,10
 * 500,-1500,8
 * 1500,20,2
 * @param refFile
 * @return
 */
RefCurve PluginGauss::loadRefFile(QFileInfo refFile)
{
    RefCurve curve;
    curve.mName = refFile.fileName().toLower();

   // QFile file(refFile.absoluteFilePath());

    FILE * pFile;
    pFile = fopen (refFile.absoluteFilePath().toLocal8Bit(),"r");
    double prev_t = -INFINITY;
    double delta_t = INFINITY;

    if (pFile != nullptr) {
    //if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QLocale locale = QLocale(QLocale::English);
     //   QTextStream stream(&file);

        bool firstLine = true;

        char ch = ' ';
        while (ch != EOF)  {

            QString line;
            // this code allows to open the MacOS file (QChar::LineFeed) and WindowsOS file (QChar::CarriageReturn)
            do {
                ch = fgetc (pFile);
                if (ch == QChar::LineFeed || ch == QChar::CarriageReturn)
                    break;
                line.append(ch);
            } while (ch != EOF);

            if (!isComment(line)) {
                QStringList values = line.split(",");
                if (values.size() > 2) {
                    bool ok = true;
                    const double t = locale.toDouble(values.at(0), &ok);
                    if (!ok)
                        continue;

                    delta_t = std::min(delta_t, abs(t-prev_t));

                    const double g = locale.toDouble(values.at(1), &ok);
                    if (!ok)
                        continue;

                    const double e = locale.toDouble(values.at(2), &ok);
                    if (!ok)
                        continue;

                    const double gSup = g + 1.96 * e;
                    if (!ok)
                        continue;

                    const double gInf = g - 1.96 * e;
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
        fclose(pFile);

        curve.mMinStep = delta_t;
        // invalid file ?
        if (!curve.mDataMean.isEmpty()) {
            curve.mTmin = curve.mDataMean.firstKey();
            curve.mTmax = curve.mDataMean.lastKey();
        }
    }
    return curve;
}

// Reference Values & Errors
long double PluginGauss::getRefValueAt(const QJsonObject &data, const double t)
{
    const QString &mode = data.value(DATE_GAUSS_MODE_STR).toString();

    if (mode == DATE_GAUSS_MODE_NONE) {
        return t;

    } else if (mode == DATE_GAUSS_MODE_EQ) {
        const long double a = data.value(DATE_GAUSS_A_STR).toDouble();
        const long double b = data.value(DATE_GAUSS_B_STR).toDouble();
        const long double c = data.value(DATE_GAUSS_C_STR).toDouble();

        return a * powl(std::abs(t), 2.l) + b * t + c;

    } else if (mode == DATE_GAUSS_MODE_CURVE) {
        const QString &ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        return getRefCurveValueAt(ref_curve, t);

    } else
        return 0.;
}

double PluginGauss::getRefErrorAt(const QJsonObject &data, const double t, const QString mode)
{
    if (mode == DATE_GAUSS_MODE_CURVE) {
        const QString &ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        return getRefCurveErrorAt(ref_curve, t);

    } else
        return 0.;
}

QPair<double, double> PluginGauss::getTminTmaxRefsCurve(const QJsonObject &data) const
{
    double tmin = 0.;
    double tmax = 0.;

    if (data.value(DATE_GAUSS_MODE_STR).toString() == DATE_GAUSS_MODE_CURVE) {
        const QString &ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
#ifdef DEBUG
        if (mRefCurves.contains(ref_curve) && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
#endif
           tmin = mRefCurves.value(ref_curve).mTmin;
           tmax = mRefCurves.value(ref_curve).mTmax;
#ifdef DEBUG

        } else
            qDebug() << "PluginGauss::getTminTmaxRefsCurve no ref curve";
#endif
    }
    else if (data.value(DATE_GAUSS_MODE_STR).toString() == DATE_GAUSS_MODE_NONE) {
        const double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        const double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();

        tmin = age - k_sigma * error;
        tmax = age + k_sigma * error;

    } else if (data[DATE_GAUSS_MODE_STR].toString() == DATE_GAUSS_MODE_EQ) {
        const double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        const double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();

        const double a = data.value(DATE_GAUSS_A_STR).toDouble();
        const double b = data.value(DATE_GAUSS_B_STR).toDouble();
        const double c = data.value(DATE_GAUSS_C_STR).toDouble();

        const double v1 = age - k_sigma * error;
        const double v2 = age + k_sigma * error;

        auto s1 = solve_quadratic(v1, a, b, c);
        auto s2 = solve_quadratic(v2, a, b, c);

        if (isinf(s1.first)) {
            if (isinf(s2.first)) {
                tmin = -INFINITY;
                tmax = INFINITY;
            } else {
                tmin = s2.first;
                tmax = s2.second;
            }
        } else if (isinf(s2.first)) {
            tmin = s1.first;
            tmax = s1.second;

        } else {
            tmin = std::min({s1.first, s2.first});
            tmax = std::max({s1.second, s2.second});
        }

    }
    return QPair<double, double>(tmin, tmax);
}

double PluginGauss::getMinStepRefsCurve(const QJsonObject &data) const
{
    int frac = 1001;
    const QString &ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
    const QString &mode = data.value(DATE_GAUSS_MODE_STR).toString();

    if (mode == DATE_GAUSS_MODE_NONE) {
        double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
        return  2 * error/ frac;

    } else if (mode == DATE_GAUSS_MODE_EQ) {
        double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();

        double a = data.value(DATE_GAUSS_A_STR).toDouble();
        double b = data.value(DATE_GAUSS_B_STR).toDouble();
        double c = data.value(DATE_GAUSS_C_STR).toDouble();

        double v1 = age - k_sigma * error;
        double v2 = age + k_sigma * error;

        const auto s1 = solve_quadratic(v1, a, b, c);
        const auto s2 = solve_quadratic(v2, a, b, c);

        if (isinf(s1.first)) {
            if (isinf(s2.first)) {
                return INFINITY;

            } else {
                return (s2.second - s2.first)/frac;
            }
        } else if (isinf(s2.first)) {
           return (s1.second - s1.first)/frac;

        } else {
            double sp1 = std::abs(s2.first - s1.first);
            double sp2 = std::abs(s2.second - s1.second);

            return std::min(sp1, sp2)/frac;
        }


    } else  if (mRefCurves.contains(ref_curve)  && !mRefCurves[ref_curve].mDataMean.isEmpty()) {
        int search_frac = k_sigma * 5;
        double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        double error = k_sigma * data.value(DATE_GAUSS_ERROR_STR).toDouble();

        std::vector<double> ordered_pts;
        ordered_pts.reserve(2 * search_frac + 1); // Pré-allocation pour éviter les réallocations


        const auto& refData = mRefCurves[ref_curve].mDataMean;

        // parcours de toute la courbe avec chaque fraction de la mesure
        for (int f = - search_frac; f<=search_frac; f++) {
            double value = age + error * static_cast<double>(f) / search_frac;
            auto prev_data = refData.cbegin();
            for ( ; prev_data != refData.cend(); prev_data++ ) {
                auto next_data = std::next(prev_data);

                double prev_value = prev_data.value();
                double next_value = next_data.value();

                // Détection du plateau : deux points consécutifs égaux
                if (prev_value == value && next_value == value) {
                    // Recherche du début et de la fin du plateau
                    auto plateau_start = prev_data;
                    auto plateau_end = next_data;

                    // Trouver le début du plateau
                    while (plateau_start != refData.cbegin() &&
                           std::prev(plateau_start).value() == value) {
                        --plateau_start;
                    }

                    // Trouver la fin du plateau
                    while (std::next(plateau_end) != refData.cend() &&
                           std::next(plateau_end).value() == value) {
                        ++plateau_end;
                    }

                    // Calculer la position moyenne du plateau
                    const auto y = (plateau_start.key() + plateau_end.key()) / 2.0;
                    ordered_pts.push_back(y);
                    break;
                }
                else if ((prev_value < value && value <= next_value) ||
                         (prev_value > value && value >= next_value)) {
                    // Interpolation uniquement quand on traverse strictement
                    const auto y = interpolate(value, prev_value, next_value,
                                               prev_data.key(), next_data.key());
                    ordered_pts.push_back(y);
                    break;  // On sort après avoir interpolé un point
                }

            }
        }
        // Suppression des doublons
        std::sort(ordered_pts.begin(), ordered_pts.end());
        ordered_pts.erase(std::unique(ordered_pts.begin(), ordered_pts.end()), ordered_pts.end());

        double min_dif = std::numeric_limits<double>::max();
        for (auto it = std::next(ordered_pts.begin()); it != ordered_pts.end(); ++it) {
            min_dif = std::min(min_dif, std::abs(*it - *std::prev(it)));
            if (min_dif==0)
                qDebug()<<"plugin gauss step=0";
        }

        return min_dif/3.;

    } else {

        return INFINITY;
    }
}

// ------------------------------------------------------------------

// Settings / Input Form / RefView
GraphViewRefAbstract* PluginGauss::getGraphViewRef()
{
   mRefGraph = new PluginGaussRefView();

   return mRefGraph;
}
void PluginGauss::deleteGraphViewRef(GraphViewRefAbstract* graph)
{
    if (graph)
        delete static_cast<PluginGaussRefView*>(graph);

    graph = nullptr;
    mRefGraph = nullptr;
}
PluginSettingsViewAbstract* PluginGauss::getSettingsView()
{
    return new PluginGaussSettingsView(this);
}

PluginFormAbstract* PluginGauss::getForm()
{
    PluginGaussForm* form = new PluginGaussForm(this);
    return form;
}

// ------------------------------------------------------------------

// Convert old project versions
QJsonObject PluginGauss::checkValuesCompatibility(const QJsonObject& values)
{
    QJsonObject result = values;
    if (!values.contains(DATE_GAUSS_MODE_STR)) {
        result.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_EQ));
    }
    //force type double
    result[DATE_GAUSS_AGE_STR] = result.value(DATE_GAUSS_AGE_STR).toDouble();
    result[DATE_GAUSS_ERROR_STR] = result.value(DATE_GAUSS_ERROR_STR).toDouble();

    result[DATE_GAUSS_A_STR] = result.value(DATE_GAUSS_A_STR).toDouble();
    result[DATE_GAUSS_B_STR] = result.value(DATE_GAUSS_B_STR).toDouble();
    result[DATE_GAUSS_C_STR] = result.value(DATE_GAUSS_C_STR).toDouble();
    result[DATE_GAUSS_CURVE_STR] = result.value(DATE_GAUSS_CURVE_STR).toString().toLower();
    return result;
}

// Date Validity
bool PluginGauss::isDateValid(const QJsonObject& data, const StudyPeriodSettings& settings)
{
    const QString mode = data.value(DATE_GAUSS_MODE_STR).toString();
    bool valid = true;
   /* const auto tminMax = getTminTmaxRefsCurve(data);
    const double new_step = (tminMax.second - tminMax.first)/5.;
    bool valid =  settings.mStep <= new_step;
    if (!valid) {
        QMessageBox message(QMessageBox::Critical,
                            qApp->applicationName() + " " + qApp->applicationVersion(),
                            QString("PluginGauss: invalid Age = %1 and Error = %2 \n Definition of the calibration curve insufficient, decrease the step below %3").arg( QString::number(data.value(DATE_GAUSS_AGE_STR).toDouble()),
                                                                                                                  QString::number(data.value(DATE_GAUSS_ERROR_STR).toDouble()),
                                                                                                                  QString::number(new_step)    ),
                            QMessageBox::Ok,
                            qApp->activeWindow());
        message.exec();
    }
*/

    long double v = 0.l;
    long double lastV = 0.l;
    if (mode == DATE_GAUSS_MODE_CURVE) {
         // controle valid solution (double)likelihood>0
        // remember likelihood type is long double
        const QString ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        const RefCurve& curve = mRefCurves.value(ref_curve);
        valid = false;
        double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        if (age > curve.mDataInfMin && age < curve.mDataSupMax)
            valid = true;

        else {
            double t = curve.mTmin;
            long double repartition = 0.l;
            while (valid == false && t <= curve.mTmax) {
                v = static_cast<long double> (getLikelihood(t, data));
                // we have to check this calculs
                //because the repartition can be smaller than the calibration
                if (lastV>0.l && v>0.l) {
                    repartition += static_cast<long double>(settings.mStep) * (lastV + v) / 2.l;

                }
                lastV = v;

                valid = ( double (repartition) > 0.);
                t += settings.mStep;
            }

        }
    }
    return valid;
}

// Combine / Split
bool PluginGauss::areDatesMergeable(const QJsonArray& )
{
    return true;
}
/**
 * @brief Combine several Gauss Age
 **/
QJsonObject PluginGauss::mergeDates(const QJsonArray& dates)
{
    QJsonObject result;
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
                    dWiggleMin = d.toObject().value(STATE_DATE_DELTA_AVERAGE).toDouble() - d.toObject().value(STATE_DATE_DELTA_ERROR).toDouble()*k_sigma;
                    dWiggleMax = d.toObject().value(STATE_DATE_DELTA_AVERAGE).toDouble() + d.toObject().value(STATE_DATE_DELTA_ERROR).toDouble()*k_sigma;
                    break;

            }

            min = std::max(tminTmax.first + dWiggleMin, min);
            max = std::min(tminTmax.second + dWiggleMax, max);
        }

        if ((max - min) <= 00.) {
            result["error"] = tr("Combine is not possible, not enough coincident densities; numeric issue");
            return result;
        }


        // inherits the first data propeties as plug-in and method...
        result = dates.at(0).toObject();
        result[STATE_NAME] = names.join(" | ");
        result[STATE_DATE_UUID] = QString::fromStdString( Generator::UUID());
        result[STATE_DATE_SUB_DATES] = dates;


        QJsonObject mergedData;

        mergedData[DATE_GAUSS_AGE_STR] = 1000.;
        mergedData[DATE_GAUSS_ERROR_STR] = 100.;
        mergedData[DATE_GAUSS_MODE_STR] = DATE_GAUSS_MODE_NONE;


        result[STATE_DATE_DATA] = mergedData;
        result[STATE_DATE_ORIGIN] = Date::eCombination;

        result[STATE_DATE_VALID] = true;
        result[STATE_DATE_DELTA_TYPE] = Date::eDeltaNone;


    } else
        result["error"] = tr("Combine needs at least 2 data !");

    return result;

}
#endif
