#include "AppSettings.h"


AppSettings::AppSettings():
mAutoSave(true),
mAutoSaveDelay(300),
mShowHelp(true)
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
}
AppSettings::~AppSettings()
{
    
}
