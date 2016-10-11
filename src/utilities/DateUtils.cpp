#include "DateUtils.h"
#include "MainWindow.h"
#include "QtUtilities.h"
#include <cmath>
#include <QLocale>


float DateUtils::convertToFormat(const float valueToFormat, const FormatDate format)
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
        case eBCAD:
        case eNumeric:
        default:
            return valueToFormat;
            break;
    }
}
float DateUtils::convertFromFormat(const float formattedValue, const FormatDate format)
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
        case eBCAD:
        case eNumeric:
        default:
            return formattedValue;
            break;
    }
}
QString DateUtils::formatString(const FormatDate format)
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
        case eNumeric:
        default:
            return "";
            break;
    }
}

QString DateUtils::dateToString(const float date)
{
    return formatValueToAppSettingsPrecision(date);
}

QString DateUtils::dateToString(const float date, int precision)
{
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    char fmt = 'f';
    if (date>250000)
        fmt = 'G';

    if (std::fabs(date)<1E-10)
        return "0";

    else
        return locale.toString(date, fmt, precision);
}

DateUtils::FormatDate DateUtils::getAppSettingsFormat()
{
    return MainWindow::getInstance()->getAppSettings().mFormatDate;
}

QString DateUtils::getAppSettingsFormatStr()
{
    return formatString(getAppSettingsFormat());
}


QString DateUtils::convertToAppSettingsFormatStr(const float valueToFormat)
{
    return dateToString(convertToAppSettingsFormat(valueToFormat));
}

float DateUtils::convertToAppSettingsFormat(const float valueToFormat)
{
    return DateUtils::convertToFormat(valueToFormat, getAppSettingsFormat());
}

QString DateUtils::convertFromAppSettingsFormatStr(const float formattedValue)
{
    return dateToString(convertFromAppSettingsFormat(formattedValue));
}

float DateUtils::convertFromAppSettingsFormat(const float formattedValue)
{
    return DateUtils::convertFromFormat(formattedValue, getAppSettingsFormat());
}
