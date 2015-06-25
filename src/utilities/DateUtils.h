#ifndef DATEUTILS_H
#define DATEUTILS_H

#include <QString>


class DateUtils{
public:
    enum FormatDate{
        eBCAD = 0,
        eCalBP = 1,
        eCalB2K = 2
    };
    static double convertToFormat(const double valueToFormat, const FormatDate format);
    static double convertFromFormat(const double formattedValue, const FormatDate format);
    
    static QString formatString(const FormatDate format);
    static QString dateToString(const double date, const int precision = 0);
    
    static double convertToAppSettingsFormat(const double valueToFormat);
    static QString convertToAppSettingsFormatStr(const double valueToFormat);
    
    static double convertFromAppSettingsFormat(const double formattedValue);
    static QString convertFromAppSettingsFormatStr(const double formattedValue);
    
    static QString getAppSettingsFormat();
};

#endif

