#ifndef SETTINGS_H
#define SETTINGS_H


class Settings
{
public:
    Settings();
    Settings(const Settings& s);
    Settings& operator=(const Settings& s);
    void copyFrom(const Settings& s);
    virtual ~Settings();

public:
    bool mAutoSave;
    int mAutoSaveDelay;
};

#endif
