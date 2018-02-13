#include "AppSettings.h"
#include <QString>
#include <QLocale>
#include <QFontMetrics>

int AppSettings::mWidthUnit;
int AppSettings::mHeigthUnit;
QFont AppSettings::mFont;
int AppSettings::mFontDescent;

AppSettings::AppSettings():
mAutoSave(APP_SETTINGS_DEFAULT_AUTO_SAVE),
mAutoSaveDelay(APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC),
mShowHelp(APP_SETTINGS_DEFAULT_SHOW_HELP),
mOpenLastProjectAtLaunch(APP_SETTINGS_DEFAULT_OPEN_PROJ),
mPixelRatio(APP_SETTINGS_DEFAULT_PIXELRATIO),
mDpm(APP_SETTINGS_DEFAULT_DPM),
mImageQuality(APP_SETTINGS_DEFAULT_IMAGE_QUALITY),
mFormatDate(APP_SETTINGS_DEFAULT_FORMATDATE),
mPrecision(APP_SETTINGS_DEFAULT_PRECISION),
mNbSheet(APP_SETTINGS_DEFAULT_SHEET)
{
    QLocale newLoc(QLocale::system());
    mLanguage = newLoc.language();
    mCountry = newLoc.country();
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);

    AppSettings::widthUnit();
    AppSettings::heigthUnit();


    AppSettings::setFont( QFont(APP_SETTINGS_DEFAULT_FONT_FAMILY, APP_SETTINGS_DEFAULT_FONT_SIZE));
   QFontMetrics fm(mFont);
   AppSettings::mWidthUnit  = fm. maxWidth();
   AppSettings::mHeigthUnit = fm.height();

    if (newLoc.decimalPoint()==',') {
        mCSVCellSeparator=";";
        mCSVDecSeparator=",";
    } else {
        mCSVCellSeparator=",";
        mCSVDecSeparator=".";
    }
}

AppSettings::AppSettings(const AppSettings& s)
{
    copyFrom(s);
}

AppSettings& AppSettings::operator=(const AppSettings& s)
{
    copyFrom(s);
    return *this;
}

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
     AppSettings::mWidthUnit  = fm. maxWidth();
     AppSettings::mHeigthUnit = fm.height();
     AppSettings::mFontDescent = fm.descent();
 }

 int  AppSettings::fontDescent()
 {
     return AppSettings::mFontDescent;
 }

 QFont AppSettings::font()
 {
     return AppSettings::mFont;
 }
