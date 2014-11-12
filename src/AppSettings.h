#ifndef APPSETTINGS_H
#define APPSETTINGS_H


class AppSettings
{
public:
    AppSettings();
    AppSettings(const AppSettings& s);
    AppSettings& operator=(const AppSettings& s);
    void copyFrom(const AppSettings& s);
    virtual ~AppSettings();

public:
    bool mAutoSave;
    int mAutoSaveDelay;
    bool mShowHelp;
};

#endif
