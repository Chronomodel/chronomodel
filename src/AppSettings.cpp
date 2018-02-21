#include "AppSettings.h"
#include <QSettings>
#include <QString>
#include <QLocale>
#include <QFontMetrics>

int AppSettings::mWidthUnit;
int AppSettings::mHeigthUnit;
QFont AppSettings::mFont;
int AppSettings::mFontDescent;
int AppSettings::mButtonWidth;

 QLocale::Language AppSettings::mLanguage;
 QLocale::Country AppSettings::mCountry;

 bool AppSettings::mAutoSave;
 int AppSettings::mAutoSaveDelay;
 bool AppSettings::mShowHelp;
 QString AppSettings:: mCSVCellSeparator;
  QString AppSettings::mCSVDecSeparator;
  bool AppSettings::mOpenLastProjectAtLaunch;
 short AppSettings:: mPixelRatio;
 short AppSettings:: mDpm;
 short AppSettings::mImageQuality;
 DateUtils::FormatDate AppSettings::mFormatDate;
 int AppSettings::mPrecision;
 int AppSettings::mNbSheet;
 QString AppSettings::mFontFamily;
  int  AppSettings::mFontPointSize;

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

void AppSettings::readSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");

    AppSettings::mLastPosition = settings.value("pos",  QPoint(200, 200)).toPoint();
    AppSettings::mLastSize = settings.value("size", QSize(400, 400)).toSize();

    settings.beginGroup("AppSettings");
    AppSettings::mLanguage = (QLocale::Language) settings.value(APP_SETTINGS_STR_LANGUAGE, QLocale::system().language()).toInt();
    AppSettings::mCountry = (QLocale::Country) settings.value(APP_SETTINGS_STR_COUNTRY, QLocale::system().language()).toInt();
    AppSettings::mFontFamily =  settings.value(APP_SETTINGS_STR_FONT_FAMILY, APP_SETTINGS_DEFAULT_FONT_FAMILY).toString();
    AppSettings::mFontPointSize = settings.value(APP_SETTINGS_STR_FONT_SIZE, APP_SETTINGS_DEFAULT_FONT_SIZE).toInt();

    if (AppSettings::mFontFamily != "")
         AppSettings::setFont(QFont(AppSettings::mFontFamily, AppSettings::mFontPointSize));
    else
        AppSettings::setFont( QFont(APP_SETTINGS_DEFAULT_FONT_FAMILY, APP_SETTINGS_DEFAULT_FONT_SIZE));


    AppSettings::mAutoSave = settings.value(APP_SETTINGS_STR_AUTO_SAVE, APP_SETTINGS_DEFAULT_AUTO_SAVE).toBool();
    AppSettings::mAutoSaveDelay = settings.value(APP_SETTINGS_STR_AUTO_SAVE_DELAY_SEC, APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC).toInt();
    AppSettings::mShowHelp = settings.value(APP_SETTINGS_STR_SHOW_HELP, APP_SETTINGS_DEFAULT_SHOW_HELP).toBool();
    AppSettings::mCSVCellSeparator = settings.value(APP_SETTINGS_STR_CELL_SEP, APP_SETTINGS_DEFAULT_CELL_SEP).toString();
    AppSettings:: mCSVDecSeparator = settings.value(APP_SETTINGS_STR_DEC_SEP, APP_SETTINGS_DEFAULT_DEC_SEP).toString();
    AppSettings::mOpenLastProjectAtLaunch = settings.value(APP_SETTINGS_STR_OPEN_PROJ, APP_SETTINGS_DEFAULT_OPEN_PROJ).toBool();
    AppSettings::mPixelRatio = settings.value(APP_SETTINGS_STR_PIXELRATIO, APP_SETTINGS_DEFAULT_PIXELRATIO).toInt();
    AppSettings::mDpm = settings.value(APP_SETTINGS_STR_DPM, APP_SETTINGS_DEFAULT_DPM).toInt();
    AppSettings::mImageQuality = settings.value(APP_SETTINGS_STR_IMAGE_QUALITY, APP_SETTINGS_DEFAULT_IMAGE_QUALITY).toInt();
    AppSettings::mFormatDate = (DateUtils::FormatDate)settings.value(APP_SETTINGS_STR_FORMATDATE, APP_SETTINGS_DEFAULT_FORMATDATE).toInt();
    AppSettings::mPrecision = settings.value(APP_SETTINGS_STR_PRECISION, APP_SETTINGS_DEFAULT_PRECISION).toInt();
    AppSettings::mNbSheet = settings.value(APP_SETTINGS_STR_SHEET, APP_SETTINGS_DEFAULT_SHEET).toInt();

    AppSettings::mLastDir = settings.value("last_project_dir", "").toString();
    AppSettings::mLastFile = settings.value("last_project_filename", "").toString();

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
    settings.setValue(APP_SETTINGS_STR_FONT_FAMILY, AppSettings::font().family());
    settings.setValue(APP_SETTINGS_STR_FONT_SIZE, AppSettings::font().pointSize());

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

   // settings.endGroup(); // Group AppSettings

    settings.endGroup();// Group MainWindows
}

/*
void AppSettings::copyFrom(const AppSettings& s)
{
    mLanguage = s.mLanguage;
    mCountry = s.mCountry;
    mFont = s.mFont;

    mAutoSave = s.mAutoSave;
    mAutoSaveDelay = s.mAutoSaveDelay;
    mShowHelp = s.mShowHelp;
    mCSVCellSeparator = s.mCSVCellSeparator;
    mCSVDecSeparator = s.mCSVDecSeparator;
    mOpenLastProjectAtLaunch = s.mOpenLastProjectAtLaunch;
    mPixelRatio = s.mPixelRatio;
    mDpm = s.mDpm;
    mImageQuality = s.mImageQuality;
    mFormatDate = s.mFormatDate;
    mPrecision = s.mPrecision;
    mNbSheet = s.mNbSheet;
}
*/
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

 void AppSettings::setFont(const QFont& font)
 {
     AppSettings::mFont = font;
     QFontMetrics fm(font);
     const QString txt1 = " HPD (%) ";
     const QString txt2 = " Min. Cnt ";
     const int lgTxt = qMax(fm.width(txt1), fm.width(txt2) );
     AppSettings::mWidthUnit  =  fm.averageCharWidth() + fm.minRightBearing(); // for ResultsView
     AppSettings::mButtonWidth = lgTxt;//fm.rightBearing('f') + fm.width(txt) + fm.leftBearing('f');
     AppSettings::mHeigthUnit = fm.height();
     AppSettings::mFontDescent = fm.descent();
     AppSettings:: mFontFamily = font.family();
     AppSettings::mFontPointSize = font.pointSize();
 }

 int  AppSettings::fontDescent()
 {
     return AppSettings::mFontDescent;
 }

 QFont AppSettings::font()
 {
     return AppSettings::mFont;
 }
