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

#include "DateUtils.h"

#include "QtUtilities.h"
#include "AppSettings.h"

#include <QLocale>


const double BASE_YEAR_BP = 1950.0;
const double BASE_YEAR_B2K = 2000.0;
const double KA_CONVERSION_FACTOR = -1.0 / 1e+3; // Coefficient pour eKa
const double MA_CONVERSION_FACTOR = -1.0 / 1e+06; // Coefficient pour eMa

double DateUtils::convertToFormat(const double valueToFormat, const FormatDate format)
{
    switch (format) {
        case eCalBP:
            return BASE_YEAR_BP - valueToFormat;
            break;
        case eCalB2K:
            return BASE_YEAR_B2K - valueToFormat;
            break;
        case eDatBP:
            return valueToFormat - BASE_YEAR_BP;
            break;
        case eDatB2K:
            return valueToFormat - BASE_YEAR_B2K;
            break;
        case eKa:
            return valueToFormat * KA_CONVERSION_FACTOR;
        break;
        case eMa:
            return valueToFormat * MA_CONVERSION_FACTOR;
        break;

        case eBCAD:
        case eBCECE:
        case eNumeric:
        default:
            return valueToFormat;
            break;
    }
}

double DateUtils::convertFromFormat(const double formatedValue, const FormatDate format)
{
    switch (format) {
        case eCalBP:
            return BASE_YEAR_BP - formatedValue;
            break;
        case eCalB2K:
            return 2000. - formatedValue;
            break;
        case eDatBP:
            return formatedValue + BASE_YEAR_BP;
            break;
        case eDatB2K:
            return formatedValue + BASE_YEAR_B2K;
            break;
        case eKa:
            return formatedValue / KA_CONVERSION_FACTOR;
        break;
        case eMa:
            return formatedValue / MA_CONVERSION_FACTOR;
        break;

        case eBCAD:
        case eBCECE:
        case eNumeric:
        default:
            return formatedValue;
            break;
    }
}

QString DateUtils::dateFormatToString(const FormatDate format)
{
    switch (format) {
        case eCalBP:
            return "Age Cal. BP"; // "Before Present"
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
            return "BC/AD"; // "Before Christ" "Anno Domini"
            break;
        case eBCECE:
            return "BCE/CE"; // "Before Common Era" "Common Era"
            break;
        case eKa:
            return "Age Ka"; // "Kiloannum"
            break;
        case eMa:
            return "Age Ma"; // "Megaannum"
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

double DateUtils::convertToAppSettingsFormat(const double &valueToFormat)
{
    return DateUtils::convertToFormat(valueToFormat, AppSettings::mFormatDate); //getAppSettingsFormat() = AppSettings::mFormatDate
}

QString DateUtils::convertFromAppSettingsFormatStr(const double formattedValue)
{
    return stringForLocal(convertFromAppSettingsFormat(formattedValue));
}

double DateUtils::convertFromAppSettingsFormat(const double &formattedValue)
{
    return DateUtils::convertFromFormat(formattedValue, AppSettings::mFormatDate);
}



std::map<double, double> DateUtils::convertMapToAppSettingsFormat(const std::map<double, double> &mapToFormat)
{
    std::map<double, double> mapResult;
    switch (AppSettings::mFormatDate) {
    case eCalBP:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(BASE_YEAR_BP - value.first, value.second);
        }
        break;
    case eCalB2K:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(BASE_YEAR_B2K - value.first, value.second);
        }
        break;
    case eDatBP:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(value.first - BASE_YEAR_BP, value.second);
        }
        break;
    case eDatB2K:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(value.first - BASE_YEAR_B2K, value.second);
        }
        break;
    case eKa:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(value.first * KA_CONVERSION_FACTOR, value.second);
        }
        break;
    case eMa:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(value.first * MA_CONVERSION_FACTOR, value.second);
        }
        break;

    case eBCAD:
    case eBCECE:
    case eNumeric:
        return mapToFormat;
        break;
    default:
        // Avertir que le format n'est pas pris en charge
        qDebug() << "[DateUtils::convertMapToAppSettingsFormat] " << "Warning: Unsupported format, returning original map.";
        return mapToFormat;
        break;
    }

    return mapResult;
}


std::map<double, double> DateUtils::convertMapFromAppSettingsFormat(const std::map<double, double> &mapToFormat)
{
    std::map<double, double> mapResult;
    switch (AppSettings::mFormatDate) {
    case eCalBP:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(BASE_YEAR_BP - value.first, value.second);
        }
        break;
    case eCalB2K:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(BASE_YEAR_B2K - value.first, value.second);
        }
        break;
    case eDatBP:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(value.first + BASE_YEAR_BP, value.second);
        }
        break;
    case eDatB2K:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(value.first + BASE_YEAR_B2K, value.second);
        }
        break;
    case eKa:
        for (const auto& value : mapToFormat) {
            mapResult.emplace(value.first / KA_CONVERSION_FACTOR, value.second);
        }
        break;
    case eMa:
        for (const auto& value : mapToFormat) {
            mapResult.emplace( value.first / MA_CONVERSION_FACTOR, value.second);
        }
        break;

    case eBCAD:
    case eBCECE:
    case eNumeric:
        return mapToFormat;
        break;
    default:
        //  Avertir que le format n'est pas pris en charge
        qDebug() << "[DateUtils::convertMapFromAppSettingsFormat] " << "Warning: Unsupported format, returning original map.";
        return std::map<double, double>(mapToFormat);
        break;
    }

    return mapResult;

}

bool DateUtils::is_date(const FormatDate format)
{
    switch (format) {
        case eCalBP:
        case eCalB2K:
        case eKa:
        case eMa:
            return false;
            break;

        case eDatBP:
        case eDatB2K:
        case eBCAD:
        case eBCECE:
        case eNumeric:
        default:
            return true;
            break;
    }
}

bool DateUtils::is_age(const FormatDate format)
{
    switch (format) {
        case eCalBP:
        case eCalB2K:
        case eKa:
        case eMa:
            return true;
            break;

        case eDatBP:
        case eDatB2K:
        case eBCAD:
        case eBCECE:
        case eNumeric:
        default:
            return false;
            break;
    }
}
