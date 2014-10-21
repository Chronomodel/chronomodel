#include "SetProjectState.h"
#include "Project.h"

SetProjectState::SetProjectState(Project* project,
                                 const QJsonObject& prevState,
                                 const QJsonObject& nextState,
                                 const QString& reason,
                                 bool notify):
mProject(project),
mPrevState(prevState),
mNextState(nextState),
mReason(reason),
mNotify(notify)
{
    setText(mReason);
}

void SetProjectState::undo()
{
    mProject->sendUpdateState(mPrevState, mReason, mNotify);
}

void SetProjectState::redo()
{
    mProject->sendUpdateState(mNextState, mReason, mNotify);
}
