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

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "DateUtils.h"

#include <qsystemdetection.h>
#include <QFont>
#include <QLocale>

#define APP_SETTINGS_DEFAULT_ICON_SIZE 3

#define APP_SETTINGS_DEFAULT_AUTO_SAVE true
#define APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC 300
#define APP_SETTINGS_DEFAULT_SHOW_HELP false
#define APP_SETTINGS_DEFAULT_CELL_SEP ","
#define APP_SETTINGS_DEFAULT_DEC_SEP "."
#define APP_SETTINGS_DEFAULT_OPEN_PROJ false
#define APP_SETTINGS_DEFAULT_PIXELRATIO 1
#define APP_SETTINGS_DEFAULT_DPM 96
#define APP_SETTINGS_DEFAULT_IMAGE_QUALITY 100
#define APP_SETTINGS_DEFAULT_FORMATDATE DateUtils::eBCAD
#define APP_SETTINGS_DEFAULT_PRECISION 0
#define APP_SETTINGS_DEFAULT_SHEET 50

#define APP_SETTINGS_STR_LANGUAGE "language"
#define APP_SETTINGS_STR_COUNTRY "country"
#define APP_SETTINGS_STR_ICON_SIZE "icon_size"

#define APP_SETTINGS_STR_AUTO_SAVE "auto_save_enabled"
#define APP_SETTINGS_STR_AUTO_SAVE_DELAY_SEC "auto_save_delay"
#define APP_SETTINGS_STR_SHOW_HELP "show_help"
#define APP_SETTINGS_STR_CELL_SEP "csv_cell_sep"
#define APP_SETTINGS_STR_DEC_SEP "csv_dec_sep"
#define APP_SETTINGS_STR_OPEN_PROJ "auto_open_project"
#define APP_SETTINGS_STR_PIXELRATIO "pixel_ratio"
#define APP_SETTINGS_STR_DPM "dpm"
#define APP_SETTINGS_STR_IMAGE_QUALITY "image_quality"
#define APP_SETTINGS_STR_FORMATDATE "format_date"
#define APP_SETTINGS_STR_PRECISION "precision"
#define APP_SETTINGS_STR_SHEET "sheet"


class AppSettings
{
public:
    AppSettings();
    virtual ~AppSettings();

    static void readSettings();
    static void writeSettings();

    static int widthUnit();
    static int heigthUnit();

    static void setWidthUnit(int &width) {mWidthUnit = width;}
    static void setHeigthUnit(int &heigth) {mHeigthUnit = heigth;}

    static QLocale::Language mLanguage;
#if QT_DEPRECATED_SINCE(6, 6)
    static QLocale::Territory mCountry;
#else
    static QLocale::Country mCountry;
#endif
    static bool mAutoSave;
    static int mAutoSaveDelay;
    static bool mShowHelp;
    static QString mCSVCellSeparator;
    static QString mCSVDecSeparator;
    static bool mOpenLastProjectAtLaunch;
    static int mPixelRatio;
    static int mIconSize;
    static int mDpm;
    static int mImageQuality;
    static  DateUtils::FormatDate mFormatDate;
    static int mPrecision;
    static int mNbSheet;

    static QString mLastDir;
    static QString mLastFile;

    static QSize mLastSize;
    static QPoint mLastPosition;

private:
    static int mWidthUnit;
    static int mHeigthUnit;
};

#endif
