/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef DATEUTILS_H
#define DATEUTILS_H

#include <QString>
#include <QMap>

#include <map>

typedef double (*DateConversion)(const double &);

class DateUtils
{
public:
    enum FormatDate
    {
        eUnknown = -2,
        eNumeric = -1,
        eBCAD = 0,
        eCalBP = 1,
        eCalB2K = 2,
        eDatBP = 3,
        eDatB2K = 4,
        eBCECE = 5,
        eKa = 6,
        eMa = 7,
        eCustom = 8
    };

    static QString dateFormatToString(const FormatDate format);

    /**
     * @brief convert native values (classic BC/AD) to their prefered display date format (Cal B2k, ...)
     */
    static double convertToAppSettingsFormat(const double &valueToFormat);
    static QString convertToAppSettingsFormatStr(const double valueToFormat, const bool forCSV = false);

    static std::map<double, double> convertMapToAppSettingsFormat(const std::map<double, double> &mapToFormat);
    static std::map<double, double> convertMapFromAppSettingsFormat(const std::map<double, double> &mapToFormat);

    static double convertToFormat(const double valueToFormat, const FormatDate format);
    static double convertFromFormat(const double formattedValue, const FormatDate format);
    /**
     * @brief convert formatted values (Cal B2k, Cal BP, ...) to native value (classic BC/AD)
     */
    static double convertFromAppSettingsFormat(const double &formattedValue);
    inline QString convertFromAppSettingsFormatStr(const double formattedValue);

    static FormatDate getAppSettingsFormat();
    static QString getAppSettingsFormatStr();

    static bool is_date(const FormatDate format);
    static bool is_age(const FormatDate format);

};

#endif
