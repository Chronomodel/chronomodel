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

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "DateUtils.h"
#include "Event.h"
#include "Painting.h"

#include <qsystemdetection.h>
#include <QFont>
#include <QLocale>

#define APP_SETTINGS_DEFAULT_ICON_SIZE 3
#define APP_SETTINGS_DEFAULT_MAPCOLOR 9

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
#define APP_SETTINGS_STR_MAPCOLOR "map_palette"

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
#define APP_SETTINGS_STR_FORMATDATE_CUSTOM "format_date_custom"
#define APP_SETTINGS_DEFAULT_FORMATDATE_CUSTOM "Years"

#define APP_SETTINGS_STR_PRECISION "precision"
#define APP_SETTINGS_STR_SHEET "sheet"

#define APP_SETTINGS_STR_EVENTMODEL "event_model"
#define APP_SETTINGS_DEFAULT_EVENTMODEL 1

class AppSettings
{
public:
    AppSettings();
    virtual ~AppSettings();

    static void readSettings();
    static void writeSettings();

    static void setWidthUnit(int &width) {mWidthUnit = width;}
    static void setHeigthUnit(int &heigth) {mHeigthUnit = heigth;}

    static int widthUnit()
    {
        return mWidthUnit;
    }

    static int heigthUnit()
    {
        return mHeigthUnit;
    }

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
    static QString mFormatDateCustom;

    static int mPrecision;
    static int mNbSheet;

    static EventModelType mEventModel;

    static QString mLastDir;
    static QString mLastFile;

    static QSize mLastSize;
    static QPoint mLastPosition;

    static bool mIsSaved;

    static ColorPalette mMapPalette;

private:
    static int mWidthUnit;
    static int mHeigthUnit;
};

#endif
