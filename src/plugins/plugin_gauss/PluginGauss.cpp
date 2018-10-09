#include "PluginGauss.h"
#if USE_PLUGIN_GAUSS

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "PluginGaussForm.h"
#include "PluginGaussRefView.h"
#include "PluginGaussSettingsView.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>

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
long double PluginGauss::getLikelihood(const double& t, const QJsonObject& data)
{
    QPair<long double, long double > result = getLikelihoodArg(t, data);

    return expl(result.second) / sqrt(result.first);
}

QPair<long double, long double> PluginGauss::getLikelihoodArg(const double& t, const QJsonObject& data)
{

    const double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
    const double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
    const QString mode = data.value(DATE_GAUSS_MODE_STR).toString();

    //long double exponent;

    double refError = getRefErrorAt(data, t, mode);
    long double variance = refError * refError + error * error;

    double refValue;

    if (mode == DATE_GAUSS_MODE_CURVE) {
        const QString ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        refValue = getRefCurveValueAt(ref_curve, t);
    }
    else
        refValue = getRefValueAt(data, t);

    const long double exponent = -0.5l * powl((long double)(age - refValue), 2.l) / variance;

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

Date::DataMethod PluginGauss::getDataMethod() const
{
    return Date::eMHSymGaussAdapt;
}

QList<Date::DataMethod> PluginGauss::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHSymetric);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}

QString PluginGauss::getDateDesc(const Date* date) const
{
    Q_ASSERT(date);
    QString result;

    const QJsonObject &data = date->mData;

    const QString mode = data[DATE_GAUSS_MODE_STR].toString();


    result += QObject::tr("Age : %1  ±  %2").arg(QLocale().toString(data[DATE_GAUSS_AGE_STR].toDouble()), QLocale().toString(data[DATE_GAUSS_ERROR_STR].toDouble()));

    if (mode == DATE_GAUSS_MODE_NONE)
        result += " (No calibration)";

    else if (mode == DATE_GAUSS_MODE_EQ) {
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

        result += "; " + QObject::tr("Ref. curve") + " : g(t) = " + eq;
    }
    else if (mode == DATE_GAUSS_MODE_CURVE) {
        const QString ref_curve = data[DATE_GAUSS_CURVE_STR].toString();
        if (mRefCurves.contains(ref_curve) && !mRefCurves[ref_curve].mDataMean.isEmpty())
            result += "; " + tr("Ref. curve : %1").arg(ref_curve);
        else
            result += ", " + tr("ERROR ->  Ref. curve : %1").arg(ref_curve);

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

int PluginGauss::csvMinColumns() const{
    return csvColumns().count() - 3;
}

QJsonObject PluginGauss::fromCSV(const QStringList& list, const QLocale& csvLocale)
{
    QJsonObject json;
    if (list.size() >= csvMinColumns()) {
        json.insert(DATE_GAUSS_AGE_STR, csvLocale.toDouble(list.at(1)));
        double error = csvLocale.toDouble(list.at(2));
        if (error == 0)
            return QJsonObject();
        json.insert(DATE_GAUSS_ERROR_STR, error);
        
        if ( (list.at(3) == "equation") && (list.size() >= csvMinColumns() + 3) ) {
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_EQ));
            double a = csvLocale.toDouble(list.at(4));
            double b = csvLocale.toDouble(list.at(5));
            double c = csvLocale.toDouble(list.at(6));
            if ( (a==0) && (b== 0))  // this solution is forbiden
                return QJsonObject();

            else {
                json.insert(DATE_GAUSS_A_STR, a);
                json.insert(DATE_GAUSS_B_STR, b);
                json.insert(DATE_GAUSS_C_STR, c);
            }

        } else if (list.at(3) == "none" || list.at(3) == "")
            json.insert(DATE_GAUSS_MODE_STR, QString(DATE_GAUSS_MODE_NONE));

        else {
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
    QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
#endif
    QString calibPath = path + "/Calib/Gauss";
    return calibPath;
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
    
    QFile file(refFile.absoluteFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QLocale locale = QLocale(QLocale::English);
        QTextStream stream(&file);
        bool firstLine = true;
        while (!stream.atEnd())
        {
            QString line = stream.readLine();
            if (!isComment(line)) {
                QStringList values = line.split(",");
                if (values.size() >= 3) {
                    bool ok = true;
                    double t = locale.toDouble(values.at(0),&ok);
                    if(!ok)
                        continue;
                    double g = locale.toDouble(values.at(1),&ok);
                    if(!ok)
                        continue;
                    double e = locale.toDouble(values.at(2),&ok);
                    if(!ok)
                        continue;
                    
                    double gSup = g + 1.96 * e;
                    if(!ok)
                        continue;
                    double gInf = g - 1.96 * e;
                    if(!ok)
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
                }
            }
        }
        file.close();
        
        // invalid file ?
        if (!curve.mDataMean.isEmpty()) {
            curve.mTmin = curve.mDataMean.firstKey();
            curve.mTmax = curve.mDataMean.lastKey();
        }
    }
    return curve;
}

// Reference Values & Errors
double PluginGauss::getRefValueAt(const QJsonObject& data, const double& t)
{
    const QString mode = data.value(DATE_GAUSS_MODE_STR).toString();
    double v = 0;
    
    if (mode == DATE_GAUSS_MODE_NONE)
        v = t;
    else if (mode == DATE_GAUSS_MODE_EQ) {
        const double a = data.value(DATE_GAUSS_A_STR).toDouble();
        const double b = data.value(DATE_GAUSS_B_STR).toDouble();
        const double c = data.value(DATE_GAUSS_C_STR).toDouble();
        
        v = a * t * t + b * t + c;
    } else if (mode == DATE_GAUSS_MODE_CURVE) {
        const QString ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        v = getRefCurveValueAt(ref_curve, t);
    }
    return v;
}

double PluginGauss::getRefErrorAt(const QJsonObject& data, const double& t, const QString mode)
{
    //QString mode = data[DATE_GAUSS_MODE_STR].toString();
    double e (0.);
    
    if (mode == DATE_GAUSS_MODE_CURVE) {
        QString ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        e = getRefCurveErrorAt(ref_curve, t);
    }
    return e;
}

QPair<double,double> PluginGauss::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    double tmin (0.);
    double tmax (0.);
    const double k (5.);
    
    if (data.value(DATE_GAUSS_MODE_STR).toString() == DATE_GAUSS_MODE_CURVE) {
        QString ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();

        if (mRefCurves.contains(ref_curve) && !mRefCurves.value(ref_curve).mDataMean.isEmpty()) {
           tmin = mRefCurves.value(ref_curve).mTmin;
           tmax = mRefCurves.value(ref_curve).mTmax;
        }
        else
            qDebug() << "PluginGauss::getTminTmaxRefsCurve no ref curve";

    }
    else if (data.value(DATE_GAUSS_MODE_STR).toString() == DATE_GAUSS_MODE_NONE) {
        double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
        
        tmin = age - k * error;
        tmax = age + k * error;
    }
    else if (data[DATE_GAUSS_MODE_STR].toString() == DATE_GAUSS_MODE_EQ) {
        double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        double error = data.value(DATE_GAUSS_ERROR_STR).toDouble();
        
        double a = data.value(DATE_GAUSS_A_STR).toDouble();
        double b = data.value(DATE_GAUSS_B_STR).toDouble();
        double c = data.value(DATE_GAUSS_C_STR).toDouble();
        
        double v1 = age - k * error;
        double v2 = age + k * error;

        if (a == 0.) {
            if (b == 0.) {
                // Error!
            } else {
                const double t1 = (v1 - c) / b;
                const double t2 = (v2 - c) / b;
                tmin = qMin(t1, t2);
                tmax = qMax(t1, t2);
            }
        }
        else {
            double delta1 = b*b - 4*a*(c - v1);
            double delta2 = b*b - 4*a*(c - v2);
            
            bool hasDelta1 = false;
            
            if (delta1 > 0) {
                hasDelta1 = true;
                
                const double t11 = (- b - sqrt(delta1)) / (2 * a);
                const double t12 = (- b + sqrt(delta1)) / (2 * a);
                
                tmin = qMin(t11, t12);
                tmax = qMax(t11, t12);
            }
            if (delta2 > 0) {
                const double t21 = (- b - sqrt(delta2)) / (2 * a);
                const double t22 = (- b + sqrt(delta2)) / (2 * a);
                
                if (hasDelta1) {
                    tmin = qMin(qMin(t21, t22), tmin);
                    tmax = qMax(qMax(t21, t22), tmax);
                }
                else {
                    tmin = qMin(t21, t22);
                    tmax = qMax(t21, t22);
                }
            }
        }
    }
    return QPair<double,double>(tmin, tmax);
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
bool PluginGauss::isDateValid(const QJsonObject& data, const ProjectSettings& settings)
{
    QString mode = data.value(DATE_GAUSS_MODE_STR).toString();
    bool valid = true;
    long double v (0.);
    long double lastV (0.);
    if (mode == DATE_GAUSS_MODE_CURVE) {
         // controle valid solution (double)likelihood>0
        // remember likelihood type is long double
        const QString ref_curve = data.value(DATE_GAUSS_CURVE_STR).toString().toLower();
        const RefCurve& curve = mRefCurves.value(ref_curve);
        valid = false;
        double age = data.value(DATE_GAUSS_AGE_STR).toDouble();
        if (age>curve.mDataInfMin && age < curve.mDataSupMax)
            valid = true;

        else {
            double t = curve.mTmin;
            long double repartition (0.);
            while (valid==false && t<=curve.mTmax) {
                v = (double)getLikelihood(t,data);
                // we have to check this calculs
                //because the repartition can be smaller than the calibration
                if (lastV>0. && v>0.)
                    repartition += (long double) settings.mStep * (lastV + v) / 2.;

                lastV = v;

                valid = ( (double)repartition > 0.);
                t +=settings.mStep;
            }
        }
    }
    return valid;
}

#endif
