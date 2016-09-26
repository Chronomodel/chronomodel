#include "StateEvent.h"


StateEvent::StateEvent(const QJsonObject& state, const QString& reason, bool notify):QEvent(QEvent::User),
mState(state),
mReason(reason),
mNotify(notify)
{
    
}

QJsonObject& StateEvent::state()
{
    return mState;
}

QString& StateEvent::reason()
{
    return mReason;
}

bool StateEvent::notify()
{
    return mNotify;
}
