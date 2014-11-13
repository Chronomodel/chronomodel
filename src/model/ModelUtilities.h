#ifndef ModelUtilities_H
#define ModelUtilities_H

#include <QIcon>
#include <QString>
#include "Date.h"
#include "Event.h"


class ModelUtilities
{
public:
    static QString getEventMethodText(Event::Method method);
    static QString getDataMethodText(Date::DataMethod method);
};

#endif