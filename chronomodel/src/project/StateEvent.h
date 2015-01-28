#ifndef StateEvent_H
#define StateEvent_H

#include <QEvent>
#include <QJsonObject>
#include <QString>

class Project;


class StateEvent: public QEvent
{
public:
    StateEvent(const QJsonObject& state, const QString& reason, bool notify);
    
    QJsonObject& state();
    QString& reason();
    bool notify();
    
private:
    QJsonObject mState;
    QString mReason;
    bool mNotify;
};


#endif