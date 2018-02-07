#ifndef DATEUTILS_H
#define DATEUTILS_H

#include <QString>


class DateUtils
{
public:
    enum FormatDate
    {
        eBCECE = -3,
        eUnknown = -2,
        eNumeric = -1,
        eBCAD = 0,
        eCalBP = 1,
        eCalB2K = 2,
        eDatBP = 3,
        eDatB2K = 4,
        eKa = 6,
        eMa = 7
    };
    static double convertToFormat(const double &valueToFormat, const FormatDate &format);
    static double convertFromFormat(const double &formattedValue, const FormatDate &format);
    
    static QString formatString(const FormatDate format);
    //static QString dateToString(const double date);
    //static QString dateToString(const double date, int precision);
    
    /** 
     * @brief convert native values (classic BC/AD) to their prefered display date format (Cal B2k, ...)
     */
    static double convertToAppSettingsFormat(const double &valueToFormat);
    static QString convertToAppSettingsFormatStr(const double valueToFormat, const bool forCSV = false);
    static QMap<double, double> convertMapToAppSettingsFormat(const QMap<double,double> &mapToFormat);

    /**
     * @brief convert formatted values (Cal B2k, Cal BP, ...) to native value (classic BC/AD)
     */
    static double convertFromAppSettingsFormat(const double &formattedValue);
    static QString convertFromAppSettingsFormatStr(const double formattedValue);
    

    static FormatDate getAppSettingsFormat();
    static QString getAppSettingsFormatStr();

};

#endif

