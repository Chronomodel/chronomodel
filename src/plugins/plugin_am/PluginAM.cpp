#include "PluginAM.h"
#if USE_PLUGIN_AM

#include "StdUtilities.h"
#include "QtUtilities.h"
#include "PluginAMForm.h"
#include "PluginAMRefView.h"
#include "PluginAMSettingsView.h"
#include "PluginAMLoop.h"
#include <cstdlib>
#include <iostream>
#include <QJsonObject>
#include <QtWidgets>


PluginAM::PluginAM()
{
    mColor = QColor(198,79,32);
    mMCMCSettings.restoreDefault();
    loadRefDatas();
}

PluginAM::~PluginAM()
{

}

#pragma mark Likelihood
long double PluginAM::getLikelihoodForMode(const double& t, const QJsonObject& data, const QString& mode)
{
    // ----------------------------------
    //  Read values
    // ----------------------------------
    long double i = (long double) data.value(DATE_AM_I).toDouble();
    long double d = (long double) data.value(DATE_AM_D).toDouble();
    long double f = (long double) data.value(DATE_AM_F).toDouble();
    
    long double alpha95 = (long double) data.value(DATE_AM_ALPHA_95).toDouble();
    long double sigmaF = (long double) data.value(DATE_AM_SIGMA_F).toDouble();
    
    QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
    QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
    QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
    
    // ---------------------------------------
    //  Get likelihood for modes I, D, F
    // ---------------------------------------
    long double m = 0;
    long double s = 0;
    long double g = 0;
    long double ge = 0;
  
    if(mode == DATE_AM_MODE_I)
    {
        m = i;
        s = alpha95 / 2.448l;
        g = getRefCurveValueAt(curveI, t);
        ge = getRefCurveErrorAt(curveI, t);
    }
    else if(mode == DATE_AM_MODE_D)
    {
        m = d;
        s = alpha95 / (2.448l * cosl(i * M_PI / 180.l));
        g = getRefCurveValueAt(curveD, t);
        ge = getRefCurveErrorAt(curveD, t);
    }
    else if(mode == DATE_AM_MODE_F)
    {
        m = f;
        s = sigmaF;
        g = getRefCurveValueAt(curveF, t);
        ge = getRefCurveErrorAt(curveF, t);
    }
    long double v = s * s + ge * ge; // variance
    long double w = 1 / v;
    long double e = -0.5l * w * (m - g) * (m - g); // exponent
    
    return expl(e) / sqrt(v);
}

long double PluginAM::getLikelihood(const double& t, const QJsonObject& data)
{
    // ----------------------------------
    //  Read values
    // ----------------------------------
    QString mode = data.value(DATE_AM_MODE).toString();
    
    long double i = (long double) data.value(DATE_AM_I).toDouble();
    long double d = (long double) data.value(DATE_AM_D).toDouble();
    long double f = (long double) data.value(DATE_AM_F).toDouble();
    
    long double alpha95 = (long double) data.value(DATE_AM_ALPHA_95).toDouble();
    long double sigmaF = (long double) data.value(DATE_AM_SIGMA_F).toDouble();
    
    QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
    QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
    QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
    
    // ---------------------------------------
    //  Get likelihood for modes I, D, F
    // ---------------------------------------
    if(mode == DATE_AM_MODE_I || mode == DATE_AM_MODE_D || mode == DATE_AM_MODE_F)
    {
        return getLikelihoodForMode(t, data, mode);
    }
    // ---------------------------------------
    //  Get likelihood for modes ID, IF, IDF
    // ---------------------------------------
    else if(mode == DATE_AM_MODE_ID || mode == DATE_AM_MODE_IF || mode == DATE_AM_MODE_IDF)
    {
        if(mode == DATE_AM_MODE_ID)
        {
            long double mi = i;
            long double si = alpha95 / 2.448l;
            long double gi = getRefCurveValueAt(curveI, t);
            long double gei = getRefCurveErrorAt(curveI, t);
            long double soi = si;
            long double vi = si * si + gei * gei + soi * soi; // variance
            long double wi = 1 / vi;
            long double ei = -0.5l * wi * (mi - gi) * (mi - gi); // exponent
            
            long double md = d;
            long double sd = (2.448l * cosl(i * M_PI / 180.l));
            long double gd = getRefCurveValueAt(curveD, t);
            long double ged = getRefCurveErrorAt(curveD, t);
            long double sod = 0;
            long double vd = sd * sd + ged * ged + sod * sod; // variance
            long double wd = 1 / vd;
            long double ed = -0.5l * wd * (md - gd) * (md - gd); // exponent
            
            return expl(ei) / sqrt(vi) + expl(ed) / sqrt(vd);
        }
        if(mode == DATE_AM_MODE_IF)
        {
            long double mi = i;
            long double si = alpha95 / 2.448l;
            long double gi = getRefCurveValueAt(curveI, t);
            long double gei = getRefCurveErrorAt(curveI, t);
            long double soi = 0;
            long double vi = si * si + gei * gei + soi * soi; // variance
            long double wi = 1 / vi;
            long double ei = -0.5l * wi * (mi - gi) * (mi - gi); // exponent
            
            long double mf = f;
            long double sf = sigmaF;
            long double gf = getRefCurveValueAt(curveF, t);
            long double gef = getRefCurveErrorAt(curveF, t);
            long double sof = 0;
            long double vf = sf * sf + gef * gef + sof * sof; // variance
            long double wf = 1 / vf;
            long double ef = -0.5l * wf * (mf - gf) * (mf - gf); // exponent
            
            return expl(ei) / sqrt(vi) + expl(ef) / sqrt(vf);
        }
        if(mode == DATE_AM_MODE_IDF)
        {
            long double mi = i;
            long double si = alpha95 / 2.448l;
            long double gi = getRefCurveValueAt(curveI, t);
            long double gei = getRefCurveErrorAt(curveI, t);
            long double soi = 0;
            long double vi = si * si + gei * gei + soi * soi; // variance
            long double wi = 1 / vi;
            long double ei = -0.5l * wi * (mi - gi) * (mi - gi); // exponent
            
            long double md = d;
            long double sd = (2.448l * cosl(i * M_PI / 180.l));
            long double gd = getRefCurveValueAt(curveD, t);
            long double ged = getRefCurveErrorAt(curveD, t);
            long double sod = 0;
            long double vd = sd * sd + ged * ged + sod * sod; // variance
            long double wd = 1 / vd;
            long double ed = -0.5l * wd * (md - gd) * (md - gd); // exponent
            
            long double mf = f;
            long double sf = sigmaF;
            long double gf = getRefCurveValueAt(curveF, t);
            long double gef = getRefCurveErrorAt(curveF, t);
            long double sof = 0;
            long double vf = sf * sf + gef * gef + sof * sof; // variance
            long double wf = 1 / vf;
            long double ef = -0.5l * wf * (mf - gf) * (mf - gf); // exponent
            
            return expl(ei) / sqrt(vi) + expl(ed) / sqrt(vd) + expl(ef) / sqrt(vf);
        }
    }
    return 0;
}

QPair<long double, long double> PluginAM::getLikelihoodArg(const double& t, const QJsonObject& data)
{
    // ----------------------------------
    //  Read values
    // ----------------------------------
    QString mode = data.value(DATE_AM_MODE).toString();
    
    long double i = (long double) data.value(DATE_AM_I).toDouble();
    long double d = (long double) data.value(DATE_AM_D).toDouble();
    long double f = (long double) data.value(DATE_AM_F).toDouble();
    
    long double alpha95 = (long double) data.value(DATE_AM_ALPHA_95).toDouble();
    long double sigmaF = (long double) data.value(DATE_AM_SIGMA_F).toDouble();
    
    QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
    QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
    QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
    
    // ---------------------------------------
    //  Get likelihood for modes I, D, F
    // ---------------------------------------
    if(mode == DATE_AM_MODE_I || mode == DATE_AM_MODE_D || mode == DATE_AM_MODE_F)
    {
        long double m = 0;
        long double s = 0;
        long double g = 0;
        long double ge = 0;
        
        if(mode == DATE_AM_MODE_I)
        {
            m = i;
            s = alpha95 / 2.448l;
            g = getRefCurveValueAt(curveI, t);
            ge = getRefCurveErrorAt(curveI, t);
            
        }
        else if(mode == DATE_AM_MODE_D)
        {
            m = d;
            s = alpha95 / (2.448l * cosl(i * M_PI / 180.l));
            g = getRefCurveValueAt(curveD, t);
            ge = getRefCurveErrorAt(curveD, t);
        }
        else if(mode == DATE_AM_MODE_F)
        {
            m = f;
            s = sigmaF;
            g = getRefCurveValueAt(curveF, t);
            ge = getRefCurveErrorAt(curveF, t);
        }
        long double v = s * s + ge * ge; // variance
        long double w = 1 / v;
        long double e = -0.5l * w * (m - g) * (m - g); // exponent
        
        return QPair<long double, long double>(v, e);
    }
    else
    {
        // Impossible to return a QPair when doing combinations !!
        // The LikelihoodArg idea is to be changed in all plugins to be ready for MCMC enabled plugins.
        return QPair<long double, long double>(0, 0);
    }
}


#pragma mark Properties
QString PluginAM::getName() const
{
    return QString("AM");
}
QIcon PluginAM::getIcon() const
{
    return QIcon(":/AM_w.png");
}
bool PluginAM::doesCalibration() const
{
    return true;
}
bool PluginAM::wiggleAllowed() const
{
    return false;
}
Date::DataMethod PluginAM::getDataMethod() const
{
    return Date::eInversion;
}
QList<Date::DataMethod> PluginAM::allowedDataMethods() const
{
    QList<Date::DataMethod> methods;
    methods.append(Date::eMHSymetric);
    methods.append(Date::eInversion);
    methods.append(Date::eMHSymGaussAdapt);
    return methods;
}

QString PluginAM::getDateDesc(const Date* date) const
{
    QString result;
    if(date)
    {
        QJsonObject data = date->mData;
        
        // ----------------------------------
        //  Read values
        // ----------------------------------
        QString mode = data.value(DATE_AM_MODE).toString();
        
        double i = data.value(DATE_AM_I).toDouble();
        double d = data.value(DATE_AM_D).toDouble();
        double f = data.value(DATE_AM_F).toDouble();
        
        double alpha95 = data.value(DATE_AM_ALPHA_95).toDouble();
        double sigmaF = data.value(DATE_AM_SIGMA_F).toDouble();
        
        QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
        QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
        QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
        
        // ----------------------------------
        //  Build description string
        // ----------------------------------
        QLocale locale = QLocale();
        
        if(mode == DATE_AM_MODE_I)
        {
            result += QObject::tr("mode : I");
            result += ", I = " + locale.toString(i);
            result += ", " + QObject::tr("Alpha95") + " : " + locale.toString(alpha95);
            result += ", " + tr("Curve I") + " : " + curveI;
        }
        else if(mode == DATE_AM_MODE_D)
        {
            result += QObject::tr("mode : D");
            result += ", I = " + locale.toString(i);
            result += ", D = " + locale.toString(d);
            result += ", " + QObject::tr("Alpha95") + " : " + locale.toString(alpha95);
            result += ", " + tr("Curve D") + " : " + curveD;
        }
        else if(mode == DATE_AM_MODE_F)
        {
            result += QObject::tr("mode : F");
            result += ", F = " + locale.toString(f);
            result += ", " + QObject::tr("Sigma F") + " : " + locale.toString(sigmaF);
            result += ", " + tr("Curve F") + " : " + curveF;
        }
        else if(mode == DATE_AM_MODE_ID)
        {
            result += QObject::tr("Mode : ID");
            result += ", I = " + locale.toString(i);
            result += ", D = " + locale.toString(d);
            result += ", " + QObject::tr("Alpha95") + " : " + locale.toString(alpha95);
            result += ", " + tr("Curve I") + " : " + curveI;
            result += ", " + tr("Curve D") + " : " + curveD;
        }
        else if(mode == DATE_AM_MODE_IF)
        {
            result += QObject::tr("Mode : IF");
            result += ", I = " + locale.toString(i);
            result += ", F = " + locale.toString(f);
            result += ", " + QObject::tr("Alpha95") + " : " + locale.toString(alpha95);
            result += ", " + QObject::tr("Sigma F") + " : " + locale.toString(sigmaF);
            result += ", " + tr("Curve I") + " : " + curveI;
            result += ", " + tr("Curve F") + " : " + curveF;
        }
        else if(mode == DATE_AM_MODE_IDF)
        {
            result += QObject::tr("Mode : IDF");
            result += ", I = " + locale.toString(i);
            result += ", D = " + locale.toString(d);
            result += ", F = " + locale.toString(f);
            result += ", " + QObject::tr("Alpha95") + " : " + locale.toString(alpha95);
            result += ", " + QObject::tr("Sigma F") + " : " + locale.toString(sigmaF);
            result += ", " + tr("Curve I") + " : " + curveI;
            result += ", " + tr("Curve D") + " : " + curveD;
            result += ", " + tr("Curve F") + " : " + curveF;
        }
    }
    return result;
}

Date::CalibrationType PluginAM::getDateCalibrationType(const QJsonObject& data)
{
    QString mode = data.value(DATE_AM_MODE).toString();
    
    if(mode == DATE_AM_MODE_ID || mode == DATE_AM_MODE_IF || mode == DATE_AM_MODE_IDF){
        return Date::eMCMC;
    }
    else{
        return Date::eStandard;
    }
}

MCMCLoop* PluginAM::createMCMCLoopForDate(const Date* date, const ProjectSettings& settings)
{
    if(date)
    {
        QJsonObject data = date->mData;
        QString mode = data.value(DATE_AM_MODE).toString();
        if(mode == DATE_AM_MODE_ID || mode == DATE_AM_MODE_IF || mode == DATE_AM_MODE_IDF){
            return new PluginAMLoop(date, settings);
        }
    }
    return 0;
}

#pragma mark CSV
QStringList PluginAM::csvColumns() const
{
    QStringList cols;
    cols << "Name"
        << "mode"
        << "I"
        << "D"
        << "F"
        << "Alpha 95"
        << "Sigma F"
        << "Curve I"
        << "Curve D"
        << "Curve F";
    return cols;
}


PluginFormAbstract* PluginAM::getForm()
{
    PluginAMForm* form = new PluginAMForm(this);
    return form;
}

#pragma mark Convert old project versions
QJsonObject PluginAM::checkValuesCompatibility(const QJsonObject& values)
{
    QJsonObject result = values;

    // example :
    // result[DATE_AM_I] = result.value(DATE_AM_I).toDouble();
    
    return result;
}

QJsonObject PluginAM::fromCSV(const QStringList& list,const QLocale &csvLocale)
{
    QJsonObject json;
    if(list.size() >= csvMinColumns())
    {
        json.insert(DATE_AM_MODE, list.at(1));
        json.insert(DATE_AM_I, csvLocale.toDouble(list.at(2)));
        json.insert(DATE_AM_D, csvLocale.toDouble(list.at(3)));
        json.insert(DATE_AM_F, csvLocale.toDouble(list.at(4)));
        json.insert(DATE_AM_ALPHA_95, csvLocale.toDouble(list.at(5)));
        json.insert(DATE_AM_SIGMA_F, csvLocale.toDouble(list.at(6)));
        json.insert(DATE_AM_CURVE_I, list.at(7));
        json.insert(DATE_AM_CURVE_D, list.at(8));
        json.insert(DATE_AM_CURVE_F, list.at(9));
    }
    return json;
}

QStringList PluginAM::toCSV(const QJsonObject& data, const QLocale& csvLocale) const
{
    QStringList list;
    list << data.value(DATE_AM_MODE).toString();
    list << csvLocale.toString(data.value(DATE_AM_I).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_D).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_F).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_ALPHA_95).toDouble());
    list << csvLocale.toString(data.value(DATE_AM_SIGMA_F).toDouble());
    list << data.value(DATE_AM_CURVE_I).toString();
    list << data.value(DATE_AM_CURVE_D).toString();
    list << data.value(DATE_AM_CURVE_F).toString();
    return list;
}

#pragma mark Reference curves (files)
QString PluginAM::getRefExt() const
{
    return "ref";
}

QString PluginAM::getRefsPath() const
{
    QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
#endif
    QString calibPath = path + "/Calib/AM";
    return calibPath;
}

RefCurve PluginAM::loadRefFile(QFileInfo refFile)
{
    RefCurve curve;
    curve.mName = refFile.fileName().toLower();
    
    QFile file(refFile.absoluteFilePath());
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QLocale locale = QLocale(QLocale::English);
        QTextStream stream(&file);
        bool firstLine = true;
        
        while(!stream.atEnd())
        {
            QString line = stream.readLine();

            if(line.contains("reference", Qt::CaseInsensitive) && line.contains("point", Qt::CaseInsensitive))
            {
                // TODO : start loading points
                break;
            }
            bool ok;
            if(!isComment(line))
            {
                QStringList values = line.split(",");
                if(values.size() >= 3)
                {
                    int t = locale.toInt(values.at(0),&ok);

                    double g = locale.toDouble(values.at(1),&ok);
                    if(!ok) continue;
                    double e = locale.toDouble(values.at(2),&ok);
                    if(!ok) continue;
                    
                    double gSup = g + 1.96f * e;
                    if(!ok) continue;
                    double gInf = g - 1.96f * e;
                    if(!ok) continue;
                    
                    curve.mDataMean[t] = g;
                    curve.mDataError[t] = e;
                    curve.mDataSup[t] = gSup;
                    curve.mDataInf[t] = gInf;
                    
                    if(firstLine)
                    {
                        curve.mDataMeanMin = g;
                        curve.mDataMeanMax = g;
                        
                        curve.mDataErrorMin = e;
                        curve.mDataErrorMax = e;
                        
                        curve.mDataSupMin = gSup;
                        curve.mDataSupMax = gSup;
                        
                        curve.mDataInfMin = gInf;
                        curve.mDataInfMax = gInf;
                    }
                    else
                    {
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
        if(!curve.mDataMean.isEmpty())
        {
            curve.mTmin = curve.mDataMean.firstKey();
            curve.mTmax = curve.mDataMean.lastKey();
        }
    }
    return curve;
}

#pragma mark Reference Values & Errors
QPair<double,double> PluginAM::getTminTmaxRefsCurve(const QJsonObject& data) const
{
    // ----------------------------------
    //  Quel est le sens de cette fonction dans le cas du plugin AM ??
    //  On a 3 types des courbes de référence, pour I, D et F.
    //  Trouver les min/max pour chaque type a du sens, mais les min/max pour tous les types combinés est absurde...
    //  ...à moins que ce ne soit utilisé que pour l'affichage des graphs !
    // ----------------------------------
    
    QPair<double,double> bounds = qMakePair<double,double>(0, 0);
    
    // ----------------------------------
    //  Read values
    // ----------------------------------
    QString mode = data.value(DATE_AM_MODE).toString();
    QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
    QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
    QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
    
    // ----------------------------------
    //  Read values
    // ----------------------------------
    if(mode == DATE_AM_MODE_I){
        bounds = getTminTmaxRefCurve(curveI);
    }
    else if(mode == DATE_AM_MODE_D){
        bounds = getTminTmaxRefCurve(curveD);
    }
    else if(mode == DATE_AM_MODE_F){
        bounds = getTminTmaxRefCurve(curveF);
    }
    else if(mode == DATE_AM_MODE_ID){
        QPair<double,double> boundsI = getTminTmaxRefCurve(curveI);
        QPair<double,double> boundsD = getTminTmaxRefCurve(curveD);
        bounds = qMakePair<double,double>(qMin(boundsI.first, boundsD.first),
                                          qMax(boundsI.second, boundsD.second));
    }
    else if(mode == DATE_AM_MODE_IF){
        QPair<double,double> boundsI = getTminTmaxRefCurve(curveI);
        QPair<double,double> boundsF = getTminTmaxRefCurve(curveF);
        bounds = qMakePair<double,double>(qMin(boundsI.first, boundsF.first),
                                          qMax(boundsI.second, boundsF.second));
    }
    else if(mode == DATE_AM_MODE_IDF){
        QPair<double,double> boundsI = getTminTmaxRefCurve(curveI);
        QPair<double,double> boundsD = getTminTmaxRefCurve(curveD);
        QPair<double,double> boundsF = getTminTmaxRefCurve(curveF);
        bounds = qMakePair<double,double>(qMin(boundsI.first, qMin(boundsD.first, boundsF.first)),
                                          qMax(boundsI.second, qMin(boundsD.second, boundsF.second)));
    }
    
    return bounds;
}

QPair<double,double> PluginAM::getTminTmaxRefCurve(const QString& curveName) const
{
    double tmin = 0;
    double tmax = 0;

    if(mRefCurves.contains(curveName) && !mRefCurves.value(curveName).mDataMean.isEmpty())
    {
        tmin = mRefCurves.value(curveName).mTmin;
        tmax = mRefCurves.value(curveName).mTmax;
    }
    return qMakePair<double,double>(tmin,tmax);
}

#pragma mark Settings / Input Form / RefView
GraphViewRefAbstract* PluginAM::getGraphViewRef()
{
    if(mRefGraph) delete mRefGraph;
    mRefGraph = new PluginAMRefView();
    return mRefGraph;
}

PluginSettingsViewAbstract* PluginAM::getSettingsView()
{
    return new PluginAMSettingsView(this);
}

#pragma mark Date validity
bool PluginAM::isDateValid(const QJsonObject& data, const ProjectSettings& settings)
{
    // ----------------------------------
    //  Read values
    // ----------------------------------
    QString mode = data.value(DATE_AM_MODE).toString();
    
    long double i = (long double) data.value(DATE_AM_I).toDouble();
    long double d = (long double) data.value(DATE_AM_D).toDouble();
    long double f = (long double) data.value(DATE_AM_F).toDouble();
    
    QString curveI = data.value(DATE_AM_CURVE_I).toString().toLower();
    QString curveD = data.value(DATE_AM_CURVE_D).toString().toLower();
    QString curveF = data.value(DATE_AM_CURVE_F).toString().toLower();
    
    // ----------------------------------
    //  Check if valid
    // ----------------------------------
    bool valid = true;
    
    if(mode == DATE_AM_MODE_I || mode == DATE_AM_MODE_ID || mode == DATE_AM_MODE_IF || mode == DATE_AM_MODE_IDF) {
        valid &= isCurveValid(data, curveI, i, settings.mStep);
    }
    if(mode == DATE_AM_MODE_D || mode == DATE_AM_MODE_ID || mode == DATE_AM_MODE_IDF) {
        valid &= isCurveValid(data, curveD, d, settings.mStep);
    }
    if(mode == DATE_AM_MODE_F || mode == DATE_AM_MODE_IF || mode == DATE_AM_MODE_IDF) {
        valid &= isCurveValid(data, curveF, f, settings.mStep);
    }
    return valid;
}

bool PluginAM::isCurveValid(const QJsonObject& data, const QString& curveName, const double& mesure, const double& step)
{
    // controle valid solution (double)likelihood>0
    // remember likelihood type is long double
    const RefCurve& curve = mRefCurves.value(curveName);
    bool valid = false;
    
    if(mesure > curve.mDataInfMin && mesure < curve.mDataSupMax){
        valid = true;
    }
    else {
        double t = curve.mTmin;
        long double repartition = 0;
        long double v = 0;
        long double lastV = 0;
        
        while(valid == false && t <= curve.mTmax){
            v = (double)getLikelihood(t, data);
            // we have to check this calculs because the repartition can be smaller than the calibration
            if (lastV>0 && v>0) {
                repartition += (long double) step * (lastV + v) / 2.;
            }
            lastV = v;
            
            valid = ( (double)repartition > 0);
            t += step;
        }
    }
    return valid;
}


#endif
