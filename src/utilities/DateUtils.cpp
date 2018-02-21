#include "DateUtils.h"
#include "MainWindow.h"
#include "QtUtilities.h"

#include <cmath>
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

QMap<double, double> DateUtils::convertMapToAppSettingsFormat(const QMap<double, double> &mapToFormat)
{
   QMap<double, double> mapResult;
   for (QMap<double, double>::const_iterator value = mapToFormat.cbegin(); value!= mapToFormat.cend(); ++value)
       mapResult.insert(convertToAppSettingsFormat(value.key()), value.value());

   return mapResult;
}
