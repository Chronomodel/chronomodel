/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "DateUtils.h"
#include "MainWindow.h"
#include "QtUtilities.h"

#include <cmath>
#include <algorithm>
#include <QLocale>

double DateUtils::convertToFormat(const double &valueToFormat, const FormatDate &format)
{
    switch (format) {
        case eCalBP:
            return 1950. - valueToFormat;
            break;
        case eCalB2K:
            return 2000. - valueToFormat;
            break;
        case eDatBP:
            return valueToFormat - 1950.;
            break;
        case eDatB2K:
            return valueToFormat - 2000.;
            break;
        case eKa:
            return (2. - valueToFormat/1e+03);
        break;
        case eMa:
            return (- valueToFormat/1e+06);
        break;

        case eBCAD:
        case eBCECE:
        case eNumeric:
        default:
            return valueToFormat;
            break;
    }
}
double DateUtils::convertFromFormat(const double &formattedValue, const FormatDate &format)
{
    switch (format) {
        case eCalBP:
            return 1950. - formattedValue;
            break;
        case eCalB2K:
            return 2000. - formattedValue;
            break;
        case eDatBP:
            return formattedValue + 1950.;
            break;
        case eDatB2K:
            return formattedValue + 2000.;
            break;
        case eKa:
            return (2. - formattedValue)*1e+03;
        break;
        case eMa:
            return (- formattedValue*1e+06);
        break;

        case eBCAD:
        case eBCECE:
        case eNumeric:
        default:
            return formattedValue;
            break;
    }
}

QString DateUtils::dateFormatToString(const FormatDate format)
{
    switch (format) {
        case eCalBP:
            return "Age Cal. BP";
            break;
        case eCalB2K:
            return "Age Cal. B2K";
            break;
        case eDatBP:
            return "Date Cal. BP";
            break;
        case eDatB2K:
            return "Date Cal. B2K";
            break;
        case eBCAD:
            return "BC/AD";
            break;
        case eBCECE:
            return "BCE/CE";
            break;
        case eKa:
            return "Age Ka";
            break;
        case eMa:
            return "Age Ma";
            break;
        case eNumeric:
        default:
            return "";
            break;
    }
}

DateUtils::FormatDate DateUtils::getAppSettingsFormat()
{
    return AppSettings::mFormatDate;
}

QString DateUtils::getAppSettingsFormatStr()
{
    return dateFormatToString(getAppSettingsFormat());
}


QString DateUtils::convertToAppSettingsFormatStr(const double valueToFormat, const bool forCSV)
{
    if (forCSV)
         return stringForCSV(convertToAppSettingsFormat(valueToFormat));
    else
         return stringForLocal(convertToAppSettingsFormat(valueToFormat));
}

double DateUtils::convertToAppSettingsFormat(const double& valueToFormat)
{
    return DateUtils::convertToFormat(valueToFormat, getAppSettingsFormat());
}

QString DateUtils::convertFromAppSettingsFormatStr(const double formattedValue)
{
    return stringForLocal(convertFromAppSettingsFormat(formattedValue));
}

double DateUtils::convertFromAppSettingsFormat(const double &formattedValue)
{
    return DateUtils::convertFromFormat(formattedValue, getAppSettingsFormat());
}

double  toCalBP(double toConvert)   /* Définit une fonction. */
{
    return (1950. -toConvert);
}

double  toCalB2K(double toConvert)
{
    return (2000. -toConvert);
}
double  toDatBP(double toConvert)
{
    return (toConvert - 1950.);
}
double  toDatB2K(double toConvert)
{
    return (toConvert -2000.);
}
double  toKa(double toConvert)
{
    return (2. - toConvert/1e+03);
}
double  toMa(double toConvert)
{
    return (- toConvert/1e+06);
}
QMap<double, double> DateUtils::convertMapToAppSettingsFormat( QMap<double, double> &mapToFormat)
{
    

    double (*pf)( double );  /* Déclare un pointeur de fonction. */

    DateUtils::FormatDate FrmDate = getAppSettingsFormat();
    switch (FrmDate) {
        case eCalBP:
            //return 1950. - valueToFormat;
            pf = &toCalBP;
            break;
        case eCalB2K:
            //return 2000. - valueToFormat;
            pf= &toCalB2K;
            break;
        case eDatBP:
            //return valueToFormat - 1950.;
            pf = &toDatBP;
            break;
        case eDatB2K:
            //return valueToFormat - 2000.;
            pf = &toDatB2K;
            break;
        case eKa:
            //return (2. - valueToFormat/1e+03);
            pf =&toKa;
        break;
        case eMa:
            //return (- valueToFormat/1e+06);
            pf = &toMa;
        break;

        case eBCAD:
        case eBCECE:
        case eNumeric:
        default:
            return QMap<double, double> ( mapToFormat);
            break;
    }
    
    QMap<double, double> mapResult;
    for (QMap<double, double>::const_iterator value = mapToFormat.cbegin(); value!= mapToFormat.cend(); ++value)
       mapResult.insert((*pf)(value.key()), value.value());

   return mapResult;
}
