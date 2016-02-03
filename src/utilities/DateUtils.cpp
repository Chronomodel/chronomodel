#include "DateUtils.h"
#include "MainWindow.h"
#include "QtUtilities.h"
#include <cmath>
#include <QLocale>


double DateUtils::convertToFormat(const double valueToFormat, const FormatDate format)
{
    switch (format) {
        case eCalBP:
            return 1950. - valueToFormat;
            break;
        case eCalB2K:
            return 2000. - valueToFormat;
            break;
        case eBCAD:
        case eNumeric:
        default:
            return valueToFormat;
            break;
    }
}
double DateUtils::convertFromFormat(const double formattedValue, const FormatDate format)
{
    switch (format) {
        case eCalBP:
            return 1950. - formattedValue;
            break;
        case eCalB2K:
            return 2000. - formattedValue;
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
            return "Cal BP";
            break;
        case eCalB2K:
            return "Cal B2K";
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

QString DateUtils::dateToString(const double date)
{
    return formatValueToAppSettingsPrecision(date);
}

QString DateUtils::dateToString(const double date, int precision)
{
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    char fmt = 'f';
    if (date>250000){
        fmt = 'G';
    }
    if (std::fabs(date)<1E-10) {
        return "0";
    }
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


QString DateUtils::convertToAppSettingsFormatStr(const double valueToFormat)
{
    return dateToString(convertToAppSettingsFormat(valueToFormat));
}

double DateUtils::convertToAppSettingsFormat(const double valueToFormat)
{
    return DateUtils::convertToFormat(valueToFormat, getAppSettingsFormat());
}

QString DateUtils::convertFromAppSettingsFormatStr(const double formattedValue)
{
    return dateToString(convertFromAppSettingsFormat(formattedValue));
}

double DateUtils::convertFromAppSettingsFormat(const double formattedValue)
{
    return DateUtils::convertFromFormat(formattedValue, getAppSettingsFormat());
}



