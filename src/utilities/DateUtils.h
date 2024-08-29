/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#ifndef DATEUTILS_H
#define DATEUTILS_H

#include <QString>
#include <QMap>

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
        eMa = 7
    };

    static QString dateFormatToString(const FormatDate format);

    /**
     * @brief convert native values (classic BC/AD) to their prefered display date format (Cal B2k, ...)
     */
    static double convertToAppSettingsFormat(const double &valueToFormat);
    static QString convertToAppSettingsFormatStr(const double valueToFormat, const bool forCSV = false);
    static QMap<double, double> convertMapToAppSettingsFormat(const QMap<double, double> &mapToFormat);

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
