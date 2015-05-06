#include "AppSettings.h"


AppSettings::AppSettings():
mAutoSave(APP_SETTINGS_DEFAULT_AUTO_SAVE),
mAutoSaveDelay(APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC),
mShowHelp(APP_SETTINGS_DEFAULT_SHOW_HELP),
mCSVCellSeparator(APP_SETTINGS_DEFAULT_CELL_SEP),
mCSVDecSeparator(APP_SETTINGS_DEFAULT_DEC_SEP),
mOpenLastProjectAtLaunch(APP_SETTINGS_DEFAULT_OPEN_PROJ),
mPixelRatio(APP_SETTINGS_DEFAULT_PIXELRATIO)
{
    
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
    mAutoSave = s.mAutoSave;
    mAutoSaveDelay = s.mAutoSaveDelay;
    mShowHelp = s.mShowHelp;
    mCSVCellSeparator = s.mCSVCellSeparator;
    mCSVDecSeparator = s.mCSVDecSeparator;
    mOpenLastProjectAtLaunch = s.mOpenLastProjectAtLaunch;
    mPixelRatio = s.mPixelRatio;
}
AppSettings::~AppSettings()
{
    
}
