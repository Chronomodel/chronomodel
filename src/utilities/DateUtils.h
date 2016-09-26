#ifndef DATEUTILS_H
#define DATEUTILS_H

#include <QString>


class DateUtils{
public:
    enum FormatDate{
        eUnknown = -2,
        eNumeric = -1,
        eBCAD = 0,
        eCalBP = 1,
        eCalB2K = 2,
        eDatBP = 3,
        eDatB2K = 4,
    };
    static float convertToFormat(const float valueToFormat, const FormatDate format);
    static float convertFromFormat(const float formattedValue, const FormatDate format);
    
    static QString formatString(const FormatDate format);
    static QString dateToString(const float date);
    static QString dateToString(const float date, int precision);
    
    /** 
     * @brief convert native values (classic BC/AD) to their prefered display date format (Cal B2k, ...)
     */
    static float convertToAppSettingsFormat(const float valueToFormat);
    static QString convertToAppSettingsFormatStr(const float valueToFormat);

    /**
     * @brief convert formatted values (Cal B2k, Cal BP, ...) to native value (classic BC/AD)
     */
    static float convertFromAppSettingsFormat(const float formattedValue);
    static QString convertFromAppSettingsFormatStr(const float formattedValue);
    

    static FormatDate getAppSettingsFormat();
    static QString getAppSettingsFormatStr();

};

#endif

