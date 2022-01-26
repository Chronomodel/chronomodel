/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "AppSettings.h"

#include <QSettings>
#include <QString>
#include <QLocale>
#include <QFontMetrics>

#include <QFile>

int AppSettings::mWidthUnit;
int AppSettings::mHeigthUnit;

QLocale::Language AppSettings::mLanguage;
QLocale::Country AppSettings::mCountry;

bool AppSettings::mAutoSave;
int AppSettings::mAutoSaveDelay;
bool AppSettings::mShowHelp;
QString AppSettings::mCSVCellSeparator;
QString AppSettings::mCSVDecSeparator;
bool AppSettings::mOpenLastProjectAtLaunch;
int AppSettings::mPixelRatio;
int AppSettings::mIconSize;
int AppSettings::mDpm;
int AppSettings::mImageQuality;
DateUtils::FormatDate AppSettings::mFormatDate;
int AppSettings::mPrecision;
int AppSettings::mNbSheet;

QString AppSettings:: mLastDir;
QString AppSettings::mLastFile;

QSize AppSettings::mLastSize;
QPoint AppSettings::mLastPosition;

AppSettings::AppSettings()
{
    QLocale newLoc(QLocale::system());
    AppSettings::mLanguage = newLoc.language();
    AppSettings::mCountry = newLoc.country();
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);

    AppSettings::readSettings();
    AppSettings::widthUnit();
    AppSettings::heigthUnit();

    if (newLoc.decimalPoint()==',') {
        AppSettings:: mCSVCellSeparator=";";
        AppSettings::mCSVDecSeparator=",";
    } else {
        AppSettings::mCSVCellSeparator=",";
        AppSettings::mCSVDecSeparator=".";
    }
}

/*
AppSettings::AppSettings(const AppSettings& s)
{
    copyFrom(s);
}
*/
/*
 *
AppSettings& AppSettings::operator=(const AppSettings& s)
{
    copyFrom(s);
    return *this;
}
*/

/*
 * On macOS and iOS, if the file format is NativeFormat, these files are used by default:

    $HOME/Library/Preferences/com.chronomodel.http:  www.ChronoModel.plist
    $HOME/Library/Preferences/CNRS.chronomodel.plist
    $HOME/Library/Preferences/com.yourcompany.chronomodel.plist

/Users/dufresne/Library/Preferences/com.chronomodel.http:  www.ChronoModel.plist

    $HOME/Library/Preferences/fr.CNRS.chronomodel.plist

    /Library/Preferences/com.MySoft.Star Runner.plist
    /Library/Preferences/com.MySoft.plist

On Windows, NativeFormat settings are stored in the following registry paths:

    HKEY_CURRENT_USER\Software\MySoft\Star Runner
    HKEY_CURRENT_USER\Software\MySoft\OrganizationDefaults
    HKEY_LOCAL_MACHINE\Software\MySoft\Star Runner
    HKEY_LOCAL_MACHINE\Software\MySoft\OrganizationDefaults

 *
 */


void AppSettings::readSettings()
{

    QSettings settings;
 //qDebug()<< settings.fileName();
 QFile file(settings.fileName());

#ifdef DEBUG
 if (file.exists())
     qDebug()<< settings.fileName() <<"exist";
else
       qDebug()<< settings.fileName() <<"n exist pas";
#endif

    settings.beginGroup("MainWindow");

    AppSettings::mLastPosition = settings.value("pos",  QPoint(200, 200)).toPoint();
    AppSettings::mLastSize = settings.value("size", QSize(400, 400)).toSize();

    settings.beginGroup("AppSettings");
    AppSettings::mLanguage = QLocale::Language (settings.value(APP_SETTINGS_STR_LANGUAGE, QLocale::system().language()).toInt());
    AppSettings::mCountry = QLocale::Country (settings.value(APP_SETTINGS_STR_COUNTRY, QLocale::system().language()).toInt());
    AppSettings::mIconSize = settings.value(APP_SETTINGS_STR_ICON_SIZE, APP_SETTINGS_DEFAULT_ICON_SIZE).toInt();

    AppSettings::mAutoSave = settings.value(APP_SETTINGS_STR_AUTO_SAVE, APP_SETTINGS_DEFAULT_AUTO_SAVE).toBool();
    AppSettings::mAutoSaveDelay = settings.value(APP_SETTINGS_STR_AUTO_SAVE_DELAY_SEC, APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC).toInt();
    AppSettings::mShowHelp = settings.value(APP_SETTINGS_STR_SHOW_HELP, APP_SETTINGS_DEFAULT_SHOW_HELP).toBool();
    AppSettings::mCSVCellSeparator = settings.value(APP_SETTINGS_STR_CELL_SEP, APP_SETTINGS_DEFAULT_CELL_SEP).toString();

    AppSettings::mCSVDecSeparator = settings.value(APP_SETTINGS_STR_DEC_SEP, APP_SETTINGS_DEFAULT_DEC_SEP).toString();
    AppSettings::mOpenLastProjectAtLaunch = settings.value(APP_SETTINGS_STR_OPEN_PROJ, APP_SETTINGS_DEFAULT_OPEN_PROJ).toBool();
    AppSettings::mPixelRatio = settings.value(APP_SETTINGS_STR_PIXELRATIO, APP_SETTINGS_DEFAULT_PIXELRATIO).toInt();

    AppSettings::mDpm = settings.value(APP_SETTINGS_STR_DPM, APP_SETTINGS_DEFAULT_DPM).toInt();
    AppSettings::mImageQuality = settings.value(APP_SETTINGS_STR_IMAGE_QUALITY, APP_SETTINGS_DEFAULT_IMAGE_QUALITY).toInt();
    AppSettings::mFormatDate = DateUtils::FormatDate (settings.value(APP_SETTINGS_STR_FORMATDATE, APP_SETTINGS_DEFAULT_FORMATDATE).toInt());
    AppSettings::mPrecision = settings.value(APP_SETTINGS_STR_PRECISION, APP_SETTINGS_DEFAULT_PRECISION).toInt();
    AppSettings::mNbSheet = settings.value(APP_SETTINGS_STR_SHEET, APP_SETTINGS_DEFAULT_SHEET).toInt();

    try {
        AppSettings::mLastDir = settings.value("last_project_dir", "").toString();
        AppSettings::mLastFile = settings.value("last_project_filename", "").toString();

    } catch (...) {
        AppSettings::mLastDir = "";
        AppSettings::mLastFile = "";
    }
    settings.endGroup();

}
void AppSettings::writeSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");

    settings.setValue("size", AppSettings::mLastSize);
    settings.setValue("pos", AppSettings::mLastPosition);

    settings.beginGroup("AppSettings");
    settings.setValue(APP_SETTINGS_STR_AUTO_SAVE, AppSettings::mAutoSave);
    settings.setValue(APP_SETTINGS_STR_AUTO_SAVE_DELAY_SEC, AppSettings::mAutoSaveDelay);

    settings.setValue(APP_SETTINGS_STR_SHOW_HELP, AppSettings::mShowHelp);
    settings.setValue(APP_SETTINGS_STR_CELL_SEP, AppSettings::mCSVCellSeparator);
    settings.setValue(APP_SETTINGS_STR_DEC_SEP, AppSettings::mCSVDecSeparator);
    settings.setValue(APP_SETTINGS_STR_OPEN_PROJ, AppSettings::mOpenLastProjectAtLaunch);
    settings.setValue(APP_SETTINGS_STR_PIXELRATIO, AppSettings::mPixelRatio);
    settings.setValue(APP_SETTINGS_STR_DPM, AppSettings::mDpm);
    settings.setValue(APP_SETTINGS_STR_IMAGE_QUALITY, AppSettings::mImageQuality);
    settings.setValue(APP_SETTINGS_STR_FORMATDATE, AppSettings::mFormatDate);
    settings.setValue(APP_SETTINGS_STR_PRECISION, AppSettings::mPrecision);
    settings.setValue(APP_SETTINGS_STR_SHEET, AppSettings::mNbSheet);

    settings.setValue("last_project_dir", AppSettings::mLastDir);
    settings.setValue("last_project_filename", AppSettings::mLastFile);

    settings.endGroup(); // Group AppSettings

    settings.endGroup();// Group MainWindows
}


AppSettings::~AppSettings()
{

}

 int AppSettings::widthUnit()
{
    return AppSettings::mWidthUnit;
}

 int AppSettings::heigthUnit()
 {
     return AppSettings::mHeigthUnit;
 }
